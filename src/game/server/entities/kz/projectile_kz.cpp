#include "projectile_kz.h"
#include <game/collision.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>

CProjectileKZ::CProjectileKZ(CGameWorld *pGameWorld, int Owner, vec2 Pos, vec2 Vel, int Type, int EntType) :
	CEntity(pGameWorld, EntType)
{
	m_SnapVel = m_Vel = Vel;
	m_SnapPos = m_Pos = Pos;
	m_Owner = Owner;
	m_Type = Type;

	m_Dir = normalize(m_Vel);

	m_PrevTuneZone = -1;
	m_TuneZone = GameServer()->Collision()->IsTune(GameServer()->Collision()->GetMapIndex(m_Pos));

	switch(Type)
	{
	case WEAPON_GUN:
		m_LifeSpan = (int)(Server()->TickSpeed() * GetTuning(m_TuneZone)->m_GunLifetime);
		break;
	case WEAPON_SHOTGUN:
		m_LifeSpan = (int)(Server()->TickSpeed() * GetTuning(m_TuneZone)->m_GrenadeLifetime);
		break;
	case WEAPON_GRENADE:
		m_LifeSpan = (int)(Server()->TickSpeed() * GetTuning(m_TuneZone)->m_ShotgunLifetime);
		break;
	default: // unknown weapon
		m_LifeSpan = 0;
		break;
	}

	m_SnapTick = Server()->Tick();
	m_StartTick = Server()->Tick();

	GameWorld()->InsertEntity(this);
}

void CProjectileKZ::Reset()
{
	printf("RESET\n");
	m_MarkedForDestroy = true;
}

void CProjectileKZ::Tick()
{
	if(GameWorld()->m_Paused)
		return;

	if(Move())
		Reset();
}

void CProjectileKZ::TickPaused()
{
	m_SnapTick++;
}

void CProjectileKZ::Snap(int SnappingClient)
{
	if(NetworkClipped(SnappingClient, m_Pos))
		return;

	int SnappingClientVersion = GameServer()->GetClientVersion(SnappingClient);
	if(SnappingClientVersion < VERSION_DDNET_ENTITY_NETOBJS)
	{
		CCharacter *pSnapChar = GameServer()->GetPlayerChar(SnappingClient);
		int Tick = (Server()->Tick() % Server()->TickSpeed()) % 20;
		if(pSnapChar && pSnapChar->IsAlive() && (m_Layer == LAYER_SWITCH && m_Number > 0 && !Switchers()[m_Number].m_aStatus[pSnapChar->Team()] && (!Tick)))
			return;
	}

	CCharacter *pOwnerChar = nullptr;
	CClientMask TeamMask = CClientMask().set();

	if(m_Owner >= 0)
		pOwnerChar = GameServer()->GetPlayerChar(m_Owner);

	if(pOwnerChar && pOwnerChar->IsAlive())
		TeamMask = pOwnerChar->TeamMask();

	if(SnappingClient != SERVER_DEMO_CLIENT && m_Owner != -1 && !TeamMask.test(SnappingClient))
		return;

	CNetObj_DDRaceProjectile DDRaceProjectile;

	if(SnappingClientVersion >= VERSION_DDNET_ENTITY_NETOBJS)
	{
		CNetObj_DDNetProjectile *pDDNetProjectile = static_cast<CNetObj_DDNetProjectile *>(Server()->SnapNewItem(NETOBJTYPE_DDNETPROJECTILE, GetId(), sizeof(CNetObj_DDNetProjectile)));
		if(!pDDNetProjectile)
		{
			return;
		}
		FillExtraInfo(pDDNetProjectile);
	}
	else if(SnappingClientVersion >= VERSION_DDNET_ANTIPING_PROJECTILE && FillExtraInfoLegacy(&DDRaceProjectile))
	{
		int Type = SnappingClientVersion < VERSION_DDNET_MSG_LEGACY ? (int)NETOBJTYPE_PROJECTILE : NETOBJTYPE_DDRACEPROJECTILE;
		void *pProj = Server()->SnapNewItem(Type, GetId(), sizeof(DDRaceProjectile));
		if(!pProj)
		{
			return;
		}
		mem_copy(pProj, &DDRaceProjectile, sizeof(DDRaceProjectile));
	}
	else
	{
		CNetObj_Projectile *pProj = Server()->SnapNewItem<CNetObj_Projectile>(GetId());
		if(!pProj)
		{
			return;
		}
		FillInfo(pProj);
	}
}

bool CProjectileKZ::Move()
{
	// Set m_Vel to tunezone speed and curvature
	bool UpdateSpeed = false;
    vec2 PrevPos = m_Pos;

	m_TuneZone = GameServer()->Collision()->IsTune(GameServer()->Collision()->GetMapIndex(m_Pos));

	if(m_PrevTuneZone != m_TuneZone)
	{
		switch(m_Type)
		{
		case WEAPON_GUN:
			m_Speed = (GetTuning(m_TuneZone)->m_GunSpeed);
			UpdateSpeed = true;
			break;
		case WEAPON_SHOTGUN:
			m_Speed = (GetTuning(m_TuneZone)->m_GrenadeSpeed);
			UpdateSpeed = true;
			break;
		case WEAPON_GRENADE:
			m_Speed = (GetTuning(m_TuneZone)->m_ShotgunSpeed);
			UpdateSpeed = true;
			break;
		}

        switch(m_Type)
        {
        case WEAPON_GUN:
            m_Curvature = Tuning()->m_GunCurvature;
            break;
        case WEAPON_SHOTGUN:
            m_Curvature = GetTuning(m_TuneZone)->m_GrenadeCurvature;
            break;
        case WEAPON_GRENADE:
            m_Curvature = GetTuning(m_TuneZone)->m_ShotgunCurvature;
            break;
        }

		m_PrevTuneZone = m_TuneZone;
	}

	if(UpdateSpeed)
	{
		printf("SETSPEED %.5f, %.5f\n", m_Speed, (m_Speed * (1 / (float)Server()->TickSpeed())));
		m_Vel = normalize(m_Vel) * (m_Speed * (1 / (float)Server()->TickSpeed()));
		m_SnapPos = m_Pos;
		m_SnapVel = m_Vel;
		m_SnapTick = Server()->Tick();
		m_Dir = normalize(m_Vel);
        UpdateSpeed = false;
	}

    //Apply curvature
    float Time = ((Server()->Tick() - (float)m_StartTick)/(float)Server()->TickSpeed());
    float Result = (m_Curvature / 10000 * (Time * Time)) * Time * 100000;
    printf("Result %f\n",Result);
    m_Vel.y = m_SnapVel.y + Result;

	vec2 CollidePos;

	m_Pos += m_Vel;
	int TeleNr = 0;

	int Collide = Collision()->FastIntersectLineProjectile(PrevPos, m_Pos, &CollidePos, &m_Pos, &TeleNr);

	if(Collide)
	{
		if(Collide == TILE_TELEINWEAPON && TeleNr)
		{
			int TeleOut = GameWorld()->m_Core.RandomOr0(Collision()->TeleOuts(TeleNr - 1).size());
			m_Pos = Collision()->TeleOuts(TeleNr - 1)[TeleOut];
		}
		else
		{
			return true;
		}
	}

	return false;
}

void CProjectileKZ::FillInfo(CNetObj_Projectile *pProj)
{
	pProj->m_X = (int)m_SnapPos.x;
	pProj->m_Y = (int)m_SnapPos.y;
	pProj->m_VelX = (int)(m_SnapVel.x * 100.0f);
	pProj->m_VelY = (int)(m_SnapVel.y * 100.0f);
	pProj->m_StartTick = m_SnapTick;
	pProj->m_Type = m_Type;
}

bool CProjectileKZ::FillExtraInfoLegacy(CNetObj_DDRaceProjectile *pProj)
{
	const int MaxPos = 0x7fffffff / 100;
	if(absolute((int)m_SnapPos.y) + 1 >= MaxPos || absolute((int)m_SnapPos.x) + 1 >= MaxPos)
	{
		// If the modified data would be too large to fit in an integer, send normal data instead
		return false;
	}
	// Send additional/modified info, by modifying the fields of the netobj
	float Angle = -std::atan2(m_SnapVel.x, m_SnapVel.y);

	int Data = 0;
	Data |= (absolute(m_Owner) & 255) << 0;
	if(m_Owner < 0)
		Data |= LEGACYPROJECTILEFLAG_NO_OWNER;
	// This bit tells the client to use the extra info
	Data |= LEGACYPROJECTILEFLAG_IS_DDNET;
	// LEGACYPROJECTILEFLAG_BOUNCE_HORIZONTAL, LEGACYPROJECTILEFLAG_BOUNCE_VERTICAL
	Data |= (m_Bouncing & 3) << 10;
	// if(m_Explosive)
	//	Data |= LEGACYPROJECTILEFLAG_EXPLOSIVE;
	// if(m_Freeze)
	//	Data |= LEGACYPROJECTILEFLAG_FREEZE;

	pProj->m_X = (int)(m_SnapPos.x * 100.0f);
	pProj->m_Y = (int)(m_SnapPos.y * 100.0f);
	pProj->m_Angle = (int)(Angle * 1000000.0f);
	pProj->m_Data = Data;
	pProj->m_StartTick = m_SnapTick;
	pProj->m_Type = m_Type;
	return true;
}

void CProjectileKZ::FillExtraInfo(CNetObj_DDNetProjectile *pProj)
{
	int Flags = 0;
	if(m_Bouncing & 1)
	{
		Flags |= PROJECTILEFLAG_BOUNCE_HORIZONTAL;
	}
	if(m_Bouncing & 2)
	{
		Flags |= PROJECTILEFLAG_BOUNCE_VERTICAL;
	}
	/*if(m_Explosive)
	{
		Flags |= PROJECTILEFLAG_EXPLOSIVE;
	}
	if(m_Freeze)
	{
		Flags |= PROJECTILEFLAG_FREEZE;
	}*/

	if(m_Owner < 0)
	{
		pProj->m_VelX = round_to_int(m_Dir.x * 1e6f);
		pProj->m_VelY = round_to_int(m_Dir.y * 1e6f);
	}
	else
	{
		pProj->m_VelX = round_to_int(m_SnapVel.x);
		pProj->m_VelY = round_to_int(m_SnapVel.y);
		Flags |= PROJECTILEFLAG_NORMALIZE_VEL;
	}

	pProj->m_X = round_to_int(m_Pos.x * 100.0f);
	pProj->m_Y = round_to_int(m_Pos.y * 100.0f);
	pProj->m_Type = m_Type;
	pProj->m_StartTick = Server()->Tick();
	pProj->m_Owner = m_Owner;
	pProj->m_Flags = Flags;
	pProj->m_SwitchNumber = m_Number;
	pProj->m_TuneZone = m_TuneZone;
}

vec2 CProjectileKZ::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
	case WEAPON_GRENADE:
		if(!m_TuneZone)
		{
			Curvature = Tuning()->m_GrenadeCurvature;
			Speed = Tuning()->m_GrenadeSpeed;
		}
		else
		{
			Curvature = TuningList()[m_TuneZone].m_GrenadeCurvature;
			Speed = TuningList()[m_TuneZone].m_GrenadeSpeed;
		}

		break;

	case WEAPON_SHOTGUN:
		if(!m_TuneZone)
		{
			Curvature = Tuning()->m_ShotgunCurvature;
			Speed = Tuning()->m_ShotgunSpeed;
		}
		else
		{
			Curvature = TuningList()[m_TuneZone].m_ShotgunCurvature;
			Speed = TuningList()[m_TuneZone].m_ShotgunSpeed;
		}

		break;

	case WEAPON_GUN:
		if(!m_TuneZone)
		{
			Curvature = Tuning()->m_GunCurvature;
			Speed = Tuning()->m_GunSpeed;
		}
		else
		{
			Curvature = TuningList()[m_TuneZone].m_GunCurvature;
			Speed = TuningList()[m_TuneZone].m_GunSpeed;
		}
		break;
	}

	return CalcPos(m_Pos, normalize(m_Vel), Curvature, Speed, Time);
}

void CProjectileKZ::SwapClients(int Client1, int Client2)
{
	m_Owner = m_Owner == Client1 ? Client2 : m_Owner == Client2 ? Client1 :
								      m_Owner;
}