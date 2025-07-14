#include "projectile_pvp.h"
#include <game/server/entities/character.h>
#include <game/collision.h>
#include <game/mapitems.h>
#include <game/server/gamecontext.h>

CProjectilePvP::CProjectilePvP(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Vel, int Type) :
CProjectileKZ(pGameWorld, Owner, Pos, Vel, Type, CGameWorld::KZ_ENTTYPE_PROJECTILE_PVP)
{
	m_Dir = normalize(Vel);

    //GameWorld()->InsertEntity(this); Dont, this is already done is base class
}

void CProjectilePvP::Tick()
{

    if(GameWorld()->m_Paused)
        return;
    
    if(Move())
        Reset();
}

bool CProjectilePvP::Move()
{
    return CProjectileKZ::Move();
}