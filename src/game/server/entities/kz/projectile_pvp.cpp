#include "projectile_pvp.h"
#include <game/server/entities/character.h>
#include <game/collision.h>
#include <game/mapitems.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>

CProjectilePvP::CProjectilePvP(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Dir, int Type) :
CProjectileKZ(pGameWorld, Owner, Pos, Dir, Type, CGameWorld::KZ_ENTTYPE_PROJECTILE_PVP)
{
	m_OrigStartTick = Server()->Tick();
	m_pInsideChar = GameServer()->GetPlayerChar(Owner);

	if(m_pInsideChar)
		m_DDRaceTeam = m_pInsideChar->Team();
	else
		m_DDRaceTeam = -1;

    //GameWorld()->InsertEntity(this); Dont, this is already done is base class
}

void CProjectilePvP::Tick()
{
	vec2 PrevPos = m_Pos;

	CProjectileKZ::Tick();

	if(m_pInsideChar && GameWorld()->ClosestCharacter(m_Pos,28.0f,nullptr) != m_pInsideChar)
		m_pInsideChar = nullptr;
	
	if(m_LifeSpan == -1 && m_Type == WEAPON_GRENADE)
	{
		GameServer()->CreateExplosion(m_Pos, m_Owner, m_Type, false, -1);
		GameServer()->CreateSound(m_Pos, SOUND_GRENADE_EXPLODE);
	}
}

void CProjectilePvP::OnCollide(vec2 PrevPos, int TileIndex, vec2 *pPreIntersectPos, vec2 *pIntersectPos, int *pTeleNr)
{
	CProjectileKZ::OnCollide(PrevPos, TileIndex, pPreIntersectPos, pIntersectPos, pTeleNr);

	if(m_MarkedForDestroy) //Reset was called on base class
	{
		if(m_Type == WEAPON_GRENADE)
		{
			GameServer()->CreateExplosion(*pPreIntersectPos, m_Owner, m_Type, false, -1);
			GameServer()->CreateSound(*pPreIntersectPos, SOUND_GRENADE_EXPLODE);
		}

		if(TileIndex == TILE_NOHOOK) //make metallic sound because yes
		{
			GameServer()->CreateSound(*pPreIntersectPos, SOUND_HOOK_NOATTACH);
		}
	}
}

void CProjectilePvP::OnCharacterCollide(vec2 PrevPos, CCharacter *pChar, vec2 *pIntersectPos)
{
	if(m_pInsideChar)
		return;

	if(m_DDRaceTeam != -1 && m_DDRaceTeam != pChar->Team())
		return;

	switch (m_Type)
	{
		case WEAPON_GRENADE:
			GameServer()->CreateExplosion(*pIntersectPos, m_Owner, m_Type, false, -1);
			GameServer()->CreateSound(*pIntersectPos, SOUND_GRENADE_EXPLODE);
			break;
		case WEAPON_SHOTGUN:
			pChar->TakeDamage(normalize(*pIntersectPos - PrevPos),1,m_Owner,WEAPON_SHOTGUN);
			break;
		case WEAPON_GUN:
			pChar->TakeDamage(vec2(0,0),1,m_Owner,WEAPON_GUN);
			break;
	}

	GameServer()->CreateSound(*pIntersectPos, SOUND_HOOK_ATTACH_PLAYER); //some weird realistic sound

	Reset();
}
