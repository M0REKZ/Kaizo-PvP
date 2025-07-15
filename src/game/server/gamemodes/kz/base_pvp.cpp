/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
/* Based on Race mod stuff and tweaked by GreYFoX@GTi and others to fit our DDRace needs. */
#include "base_pvp.h"

#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#include <game/server/entities/kz/projectile_pvp.h>

#define GAME_TYPE_NAME "PVP"
#define TEST_TYPE_NAME "TestPVP"

CGameControllerBasePvP::CGameControllerBasePvP(class CGameContext *pGameServer) :
	CGameControllerBaseKZ(pGameServer)
{
	m_pGameType = g_Config.m_SvTestingCommands ? TEST_TYPE_NAME : GAME_TYPE_NAME;
	m_GameFlags = 0;
}

CGameControllerBasePvP::~CGameControllerBasePvP() = default;

bool CGameControllerBasePvP::OnCharacterTakeDamage(CCharacter *pChar, vec2 Force, int Dmg, int From, int Weapon)
{
	if(!pChar)
		return false;

	pChar->GetCoreKZ().m_Vel += Force;

	if(GameServer()->m_pController->IsFriendlyFire(pChar->GetPlayer()->GetCid(), From) && !g_Config.m_SvTeamdamage)
		return false;

	// m_pPlayer only inflicts half damage on self
	if(From == pChar->GetPlayer()->GetCid())
		Dmg = std::max(1, Dmg/2);

	pChar->GetDamageTakenKZ()++;

	// create healthmod indicator
	if(Server()->Tick() < pChar->GetDamageTakenTickKZ()+25)
	{
		// make sure that the damage indicators doesn't group together
		GameServer()->CreateDamageInd(pChar->m_Pos, pChar->GetDamageTakenKZ()*0.25f, Dmg);
	}
	else
	{
		pChar->GetDamageTakenKZ() = 0;
		GameServer()->CreateDamageInd(pChar->m_Pos, 0, Dmg);
	}

	if(Dmg)
	{
		if(pChar->GetArmor())
		{
			if(Dmg > 1)
			{
				pChar->GetHealthKZ()--;
				Dmg--;
			}

			if(Dmg > pChar->GetArmor())
			{
				Dmg -= pChar->GetArmor();
				pChar->SetArmor(0);
			}
			else
			{
				pChar->SetArmor(pChar->GetArmor() - Dmg);
				Dmg = 0;
			}
		}

		pChar->GetHealthKZ() -= Dmg;
	}

	pChar->GetDamageTakenTickKZ() = Server()->Tick();

	// do damage Hit sound
	if(From >= 0 && From != pChar->GetPlayer()->GetCid() && GameServer()->m_apPlayers[From] && GameServer()->m_apPlayers[From]->GetCharacter())
	{
		GameServer()->CreateSound(GameServer()->m_apPlayers[From]->m_ViewPos, SOUND_HIT, GameServer()->m_apPlayers[From]->GetCharacter()->TeamMask());
	}

	// check for death
	if(pChar->GetHealthKZ() <= 0)
	{
		pChar->Die(From, Weapon);

		// set attacker's face to happy (taunt!)
		if (From >= 0 && From != pChar->GetPlayer()->GetCid() && GameServer()->m_apPlayers[From])
		{
			CCharacter *pChr = GameServer()->m_apPlayers[From]->GetCharacter();
			if (pChr)
			{
				pChar->SetEmote(EMOTE_HAPPY, Server()->Tick() + Server()->TickSpeed());
			}
		}

		return false;
	}

	if (Dmg > 2)
		GameServer()->CreateSound(pChar->m_Pos, SOUND_PLAYER_PAIN_LONG);
	else
		GameServer()->CreateSound(pChar->m_Pos, SOUND_PLAYER_PAIN_SHORT);

	pChar->SetEmote(EMOTE_PAIN, Server()->Tick() + 500 * Server()->TickSpeed() / 1000);

	return true;
}

bool CGameControllerBasePvP::CharacterFireWeapon(CCharacter *pChar)
{
	if(pChar->GetReloadTimerKZ() != 0)
		return true;

	pChar->DoWeaponSwitch();
	vec2 Direction = normalize(vec2(pChar->GetLatestInputKZ().m_TargetX, pChar->GetLatestInputKZ().m_TargetY));

	bool FullAuto = false;
	if(pChar->GetActiveWeapon() == WEAPON_GRENADE || pChar->GetActiveWeapon() == WEAPON_SHOTGUN || pChar->GetActiveWeapon() == WEAPON_LASER)
		FullAuto = true;


	// check if we gonna fire
	bool WillFire = false;
	if(CountInput(pChar->GetLatestPrevInputKZ().m_Fire, pChar->GetLatestInputKZ().m_Fire).m_Presses)
		WillFire = true;

	if(FullAuto && (pChar->GetLatestInputKZ().m_Fire&1) && pChar->Core()->m_aWeapons[pChar->GetActiveWeapon()].m_Ammo)
		WillFire = true;

	if(!WillFire)
		return true;

	// check for ammo
	if(!pChar->Core()->m_aWeapons[pChar->GetActiveWeapon()].m_Ammo)
	{
		// 125ms is a magical limit of how fast a human can click
		pChar->GetReloadTimerKZ() = 125 * Server()->TickSpeed() / 1000;
		if(pChar->GetLastNoAmmoSoundKZ()+Server()->TickSpeed() <= Server()->Tick())
		{
			GameServer()->CreateSound(pChar->m_Pos, SOUND_WEAPON_NOAMMO);
			pChar->GetLastNoAmmoSoundKZ() = Server()->Tick();
		}
		return true;
	}

	vec2 ProjStartPos = pChar->m_Pos+Direction*pChar->GetProximityRadius()*0.75f;

	if(Config()->m_Debug)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "shot player='%d:%s' team=%d weapon=%d", pChar->GetPlayer()->GetCid(), Server()->ClientName(pChar->GetPlayer()->GetCid()), pChar->GetPlayer()->GetTeam(), pChar->GetActiveWeapon());
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	switch(pChar->GetActiveWeapon())
	{
		case WEAPON_HAMMER:
		{
			GameServer()->CreateSound(pChar->m_Pos, SOUND_HAMMER_FIRE);

			CCharacter *apEnts[MAX_CLIENTS];
			int Hits = 0;
			int Num = pChar->GameWorld()->FindEntities(ProjStartPos, pChar->GetProximityRadius()*0.5f, (CEntity**)apEnts,
														MAX_CLIENTS, CGameWorld::ENTTYPE_CHARACTER);

			for(int i = 0; i < Num; ++i)
			{
				CCharacter *pTarget = apEnts[i];

				if((pTarget == pChar) || GameServer()->Collision()->IntersectLine(ProjStartPos, pTarget->m_Pos, NULL, NULL))
					continue;

				// set his velocity to fast upward (for now)
				if(length(pTarget->m_Pos-ProjStartPos) > 0.0f)
					GameServer()->CreateHammerHit(pTarget->m_Pos-normalize(pTarget->m_Pos-ProjStartPos)*pChar->GetProximityRadius()*0.5f);
				else
					GameServer()->CreateHammerHit(ProjStartPos);

				vec2 Dir;
				if(length(pTarget->m_Pos - pChar->m_Pos) > 0.0f)
					Dir = normalize(pTarget->m_Pos - pChar->m_Pos);
				else
					Dir = vec2(0.f, -1.f);

				pTarget->TakeDamage(vec2(0.f, -1.f) + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f, 3,
					pChar->GetPlayer()->GetCid(), pChar->GetActiveWeapon());
				Hits++;
			}

			// if we Hit anything, we have to wait for the reload
			if(Hits)
				pChar->GetReloadTimerKZ() = Server()->TickSpeed()/3;

		} break;

		case WEAPON_GUN:
		{
			new CProjectilePvP(&GameServer()->m_World, pChar->GetPlayer()->GetCid(), ProjStartPos, Direction, WEAPON_GUN);

			GameServer()->CreateSound(pChar->m_Pos, SOUND_GUN_FIRE);
		} break;

		case WEAPON_SHOTGUN:
		{
			int ShotSpread = 2;

			for(int i = -ShotSpread; i <= ShotSpread; ++i)
			{
				float Spreading[] = {-0.185f, -0.070f, 0, 0.070f, 0.185f};
				float a = angle(Direction);
				a += Spreading[i+2];
				float v = 1-(absolute(i)/(float)ShotSpread);
				float Speed = mix((float)GameServer()->Tuning()->m_ShotgunSpeeddiff, 1.0f, v);
				/*new CProjectile(GameWorld(), WEAPON_SHOTGUN,
					pChar->GetPlayer()->GetCid(),
					ProjStartPos,
					vec2(cosf(a), sinf(a))*Speed,
					(int)(Server()->TickSpeed()*GameServer()->Tuning()->m_ShotgunLifetime),
					g_pData->m_Weapons.m_Shotgun.m_pBase->m_Damage, false, 0, -1, WEAPON_SHOTGUN);*/
			}

			GameServer()->CreateSound(pChar->m_Pos, SOUND_SHOTGUN_FIRE);
		} break;

		case WEAPON_GRENADE:
		{
			new CProjectileKZ(&GameServer()->m_World, pChar->GetPlayer()->GetCid(), ProjStartPos, Direction, WEAPON_GRENADE);

			GameServer()->CreateSound(pChar->m_Pos, SOUND_GRENADE_FIRE);
		} break;

		case WEAPON_LASER:
		{
			//new CLaser(GameWorld(), pChar->m_Pos, Direction, GameServer()->Tuning()->m_LaserReach, pChar->GetPlayer()->GetCid());
			GameServer()->CreateSound(pChar->m_Pos, SOUND_LASER_FIRE);
		} break;

		case WEAPON_NINJA:
		{
			//pChar->m_NumObjectsHit = 0;

			pChar->GetCoreKZ().m_Ninja.m_ActivationDir = Direction;
			//pChar->GetCoreKZ().m_Ninja.m_CurrentMoveTime = g_pData->m_Weapons.m_Ninja.m_Movetime * Server()->TickSpeed() / 1000;
			pChar->GetCoreKZ().m_Ninja.m_OldVelAmount = length(pChar->GetCoreKZ().m_Vel);

			GameServer()->CreateSound(pChar->m_Pos, SOUND_NINJA_FIRE);
		} break;

	}

	pChar->GetAttackTickKZ() = Server()->Tick();

	if(pChar->GetCoreKZ().m_aWeapons[pChar->GetActiveWeapon()].m_Ammo > 0) // -1 == unlimited
		pChar->GetCoreKZ().m_aWeapons[pChar->GetActiveWeapon()].m_Ammo--;

	if(!pChar->GetReloadTimerKZ())
	{
		float FireDelay;
		pChar->GetTuning(pChar->m_TuneZone)->Get(offsetof(CTuningParams, m_HammerFireDelay) / sizeof(CTuneParam) + pChar->GetCoreKZ().m_ActiveWeapon, &FireDelay);
		pChar->GetReloadTimerKZ() = FireDelay * Server()->TickSpeed() / 1000;
	}

	return true;
}
