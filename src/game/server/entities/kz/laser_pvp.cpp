/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <game/server/entities/character.h>

#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/mapitems.h>

#include <game/server/gamecontext.h>
#include <game/server/gamemodes/DDRace.h>
#include "laser_pvp.h"

CLaserPvP::CLaserPvP(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Type, int Bounces) :
CLaserKZ(pGameWorld, Pos, Direction, StartEnergy, Owner, Type, CGameWorld::KZ_ENTTYPE_LASER_PVP, Bounces, false, false)
{
}

void CLaserPvP::OnCharacterCollide(CCharacter *pChar, vec2 CollidePos)
{
	pChar->TakeDamage(vec2(0,0), 5, m_Owner, m_Type);
	m_ResetNextTick = true;
}

void CLaserPvP::CreateNewLaser()
{
	CLaserPvP *p = new CLaserPvP(GameWorld(), m_NewLaserPos, m_Dir, m_Energy, m_Owner, m_Type, m_Bounces);
	if(p && !p->m_MarkedForDestroy)
	{
		p->Tick();
	}
	m_ResetNextTick = true;
}
