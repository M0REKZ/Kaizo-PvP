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
