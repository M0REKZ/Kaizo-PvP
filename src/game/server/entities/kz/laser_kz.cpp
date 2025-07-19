/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "laser_kz.h"
#include <game/server/entities/character.h>

#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/mapitems.h>

#include <game/server/gamecontext.h>
#include <game/server/gamemodes/DDRace.h>

CLaserKZ::CLaserKZ(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Type ,int EntType, int Bounces ,bool IsBlueTeleport, bool TeleportCancelled) :
	CEntity(pGameWorld, EntType)
{
	m_Pos = Pos;
	m_Owner = Owner;
	m_Energy = StartEnergy;
	m_Dir = Direction;
	m_EvalTick = 0;
	m_Type = Type;
	m_TeleportCancelled = TeleportCancelled;
	m_IsBlueTeleport = IsBlueTeleport;
	m_Bounces = Bounces;
	m_CreateNewLaser = false;
	m_SnapTick = Server()->Tick();

	m_ZeroEnergyBounceInLastTick = false;
	m_PrevTuneZone = m_TuneZone = GameServer()->Collision()->IsTune(GameServer()->Collision()->GetMapIndex(m_Pos));
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	m_TeamMask = pOwnerChar ? pOwnerChar->TeamMask() : CClientMask();

	switch (m_Type)
	{
	case WEAPON_LASER:
	case WEAPON_SHOTGUN:
		if(m_TuneZone)
			m_MaxEnergy = GetTuning(m_TuneZone)->m_LaserReach;
		else
			m_MaxEnergy = Tuning()->m_LaserReach;
		break;
	default: //unknown type
		m_MaxEnergy = 0.f;
		break;
	}

	GameWorld()->InsertEntity(this);
}

CCharacter *CLaserKZ::HitCharacter(vec2 From, vec2 To)
{
	static const vec2 StackedLaserShotgunBugSpeed = vec2(-2147483648.0f, -2147483648.0f);
	vec2 At;
	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	CCharacter *pHit;
	bool pDontHitSelf = g_Config.m_SvOldLaser || m_Bounces == 0;

	if(pOwnerChar ? (!pOwnerChar->LaserHitDisabled() && m_Type == WEAPON_LASER) || (!pOwnerChar->ShotgunHitDisabled() && m_Type == WEAPON_SHOTGUN) : g_Config.m_SvHit)
		pHit = GameWorld()->IntersectCharacter(m_Pos, To, 0.f, At, pDontHitSelf ? pOwnerChar : nullptr, m_Owner);
	else
		pHit = GameWorld()->IntersectCharacter(m_Pos, To, 0.f, At, pDontHitSelf ? pOwnerChar : nullptr, m_Owner, pOwnerChar);

	if(!pHit || (pHit == pOwnerChar && g_Config.m_SvOldLaser) || (pHit != pOwnerChar && pOwnerChar ? (pOwnerChar->LaserHitDisabled() && m_Type == WEAPON_LASER) || (pOwnerChar->ShotgunHitDisabled() && m_Type == WEAPON_SHOTGUN) : !g_Config.m_SvHit))
		return nullptr;
	m_PrevPos = From;
	m_Pos = At;
	
	return pHit;
}

void CLaserKZ::DoBounce()
{
	if(m_EvalTick)
	{
		Reset();
	}

	m_EvalTick = Server()->Tick();

	if(m_Energy < 0)
	{
		m_MarkedForDestroy = true;
		return;
	}
	m_PrevPos = m_Pos;
	vec2 Coltile;

	int Res;
	int z;

	vec2 To = m_Pos + m_Dir * m_Energy;

	Res = GameServer()->Collision()->FastIntersectLineProjectile(m_Pos, To, &Coltile, &To, &z);

	CCharacter *pHit = HitCharacter(m_Pos, To);

	if(Res && !pHit)
	{
		
			// intersected
			m_PrevPos = m_Pos;
			m_Pos = To;

			vec2 TempPos = m_Pos;
			vec2 TempDir = m_Dir * 4.0f;

			GameServer()->Collision()->MovePoint(&TempPos, &TempDir, 1.0f, nullptr);
			
			m_NewLaserPos = TempPos;
			m_Dir = normalize(TempDir);

			const float Distance = distance(m_PrevPos, m_Pos);
			// Prevent infinite bounces
			if(Distance == 0.0f && m_ZeroEnergyBounceInLastTick)
			{
				m_Energy = -1;
			}
			else if(!m_TuneZone)
			{
				m_Energy -= Distance + Tuning()->m_LaserBounceCost;
			}
			else
			{
				m_Energy -= distance(m_PrevPos, m_Pos) + GameServer()->TuningList()[m_TuneZone].m_LaserBounceCost;
			}
			m_ZeroEnergyBounceInLastTick = Distance == 0.0f;

			if(Res == TILE_TELEINWEAPON && !GameServer()->Collision()->TeleOuts(z - 1).empty())
			{
				int TeleOut = GameServer()->m_World.m_Core.RandomOr0(GameServer()->Collision()->TeleOuts(z - 1).size());
				m_NewLaserPos = GameServer()->Collision()->TeleOuts(z - 1)[TeleOut];
			}
			else
			{
				m_Bounces++;
				m_NewLaserPos = m_Pos;
			}
			m_CreateNewLaser = true;

			int BounceNum = Tuning()->m_LaserBounceNum;
			if(m_TuneZone)
				BounceNum = TuningList()[m_TuneZone].m_LaserBounceNum;

			if(m_Bounces > BounceNum)
				m_Energy = -1;

			if(m_Bounces == 0)
				GameServer()->CreateSound(m_Pos, SOUND_LASER_BOUNCE, m_TeamMask);
		
	}
	else if(!pHit)
	{
		
		
			m_PrevPos = m_Pos;
			m_Pos = To;
			m_Energy = -1;
		
	}
	else if(pHit)
	{
		OnCharacterCollide(pHit, m_Pos);
	}

	CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	if(m_Owner >= 0 && m_Energy <= 0 && !m_TeleportCancelled && pOwnerChar &&
		pOwnerChar->IsAlive() && pOwnerChar->HasTelegunLaser() && m_Type == WEAPON_LASER)
	{
		vec2 PossiblePos;
		bool Found = false;

		// Check if the laser hits a player.
		bool pDontHitSelf = g_Config.m_SvOldLaser || m_Bounces == 0;
		vec2 At;

		if(pHit)
			Found = GetNearestAirPosPlayer(pHit->m_Pos, &PossiblePos);
		else
			Found = GetNearestAirPos(m_Pos, m_PrevPos, &PossiblePos);

		if(Found)
		{
			pOwnerChar->m_TeleGunPos = PossiblePos;
			pOwnerChar->m_TeleGunTeleport = true;
			pOwnerChar->m_IsBlueTeleGunTeleport = m_IsBlueTeleport;
		}
	}
	else if(m_Owner >= 0)
	{
		int MapIndex = GameServer()->Collision()->GetPureMapIndex(Coltile);
		int TileFIndex = GameServer()->Collision()->GetFrontTileIndex(MapIndex);
		bool IsSwitchTeleGun = GameServer()->Collision()->GetSwitchType(MapIndex) == TILE_ALLOW_TELE_GUN;
		bool IsBlueSwitchTeleGun = GameServer()->Collision()->GetSwitchType(MapIndex) == TILE_ALLOW_BLUE_TELE_GUN;
		int IsTeleInWeapon = GameServer()->Collision()->IsTeleportWeapon(MapIndex);

		if(!IsTeleInWeapon)
		{
			if(IsSwitchTeleGun || IsBlueSwitchTeleGun)
			{
				// Delay specifies which weapon the tile should work for.
				// Delay = 0 means all.
				int delay = GameServer()->Collision()->GetSwitchDelay(MapIndex);

				if((delay != 3 && delay != 0) && m_Type == WEAPON_LASER)
				{
					IsSwitchTeleGun = IsBlueSwitchTeleGun = false;
				}
			}

			m_IsBlueTeleport = TileFIndex == TILE_ALLOW_BLUE_TELE_GUN || IsBlueSwitchTeleGun;

			// Teleport is canceled if the last bounce tile is not a TILE_ALLOW_TELE_GUN.
			// Teleport also works if laser didn't bounce.
			m_TeleportCancelled =
				m_Type == WEAPON_LASER && (TileFIndex != TILE_ALLOW_TELE_GUN && TileFIndex != TILE_ALLOW_BLUE_TELE_GUN && !IsSwitchTeleGun && !IsBlueSwitchTeleGun);
		}
	}
}

void CLaserKZ::OnCharacterCollide(CCharacter *pChar, vec2 CollidePos)
{
	Reset();
}

void CLaserKZ::CreateNewLaser()
{
	CLaserKZ *p = new CLaserKZ(GameWorld(), m_NewLaserPos, m_Dir, m_Energy, m_Owner, m_Type, CGameWorld::KZ_ENTTYPE_LASER_KZ, m_Bounces, m_IsBlueTeleport, m_TeleportCancelled);

	if(p && !p->m_MarkedForDestroy)
	{
		p->m_ZeroEnergyBounceInLastTick = m_ZeroEnergyBounceInLastTick;
		p->Tick();
	}
	m_ResetNextTick = true;
}

void CLaserKZ::Reset()
{
	m_MarkedForDestroy = true;
}

void CLaserKZ::Tick()
{
	if((g_Config.m_SvDestroyLasersOnDeath) && m_Owner >= 0)
	{
		CCharacter *pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
		if(!(pOwnerChar && pOwnerChar->IsAlive()))
		{
			Reset();
			return;
		}
	}

	float Delay;
	if(m_TuneZone)
		Delay = TuningList()[m_TuneZone].m_LaserBounceDelay;
	else
		Delay = Tuning()->m_LaserBounceDelay;

	if((Server()->Tick() - m_EvalTick) > (Server()->TickSpeed() * Delay / 1000.0f))
	{
		m_SnapTick = Server()->Tick();
		if(m_CreateNewLaser && m_Energy >= 0)
		{
			CreateNewLaser();
			m_CreateNewLaser = false;
		}
		if(m_ResetNextTick)
		{
			Reset();
			return;
		}
		
		DoBounce();
	}
	
}

void CLaserKZ::TickPaused()
{
	++m_EvalTick;
}

void CLaserKZ::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient) && NetworkClipped(SnappingClient, m_PrevPos))
		return;
	CCharacter *pOwnerChar = nullptr;
	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);
	if(!pOwnerChar)
		return;

	pOwnerChar = nullptr;
	CClientMask TeamMask = CClientMask().set();

	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(pOwnerChar && pOwnerChar->IsAlive())
		TeamMask = pOwnerChar->TeamMask();

	if(SnappingClient != SERVER_DEMO_CLIENT && !TeamMask.test(SnappingClient))
		return;

	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	int LaserType = m_Type == WEAPON_LASER ? LASERTYPE_RIFLE : m_Type == WEAPON_SHOTGUN ? LASERTYPE_SHOTGUN : -1;

	GameServer()->SnapLaserObject(CSnapContext(SnappingClientVersion), GetId(),
		m_Pos, m_PrevPos, m_SnapTick, m_Owner, LaserType, 0, m_Number);
}

void CLaserKZ::SwapClients(int Client1, int Client2)
{
	m_Owner = m_Owner == Client1 ? Client2 : m_Owner == Client2 ? Client1 : m_Owner;
}
