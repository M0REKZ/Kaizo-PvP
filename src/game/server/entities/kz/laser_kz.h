/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_LASER_KZ_H
#define GAME_SERVER_ENTITIES_LASER_KZ_H

#include <game/server/entity.h>

class CLaserKZ : public CEntity
{
public:
	CLaserKZ(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Type, int EntType = CGameWorld::KZ_ENTTYPE_LASER_KZ,int Bounces = 0, bool IsBlueTeleport = false, bool TeleportCancelled = false);

	void Reset() override;
	void Tick() override;
	void TickPaused() override;
	void Snap(int SnappingClient) override;
	void SwapClients(int Client1, int Client2) override;

	int GetOwnerId() const override { return m_Owner; }

protected:
	CCharacter *HitCharacter(vec2 From, vec2 To);
	void DoBounce();

	virtual void OnCharacterCollide(CCharacter *pChar, vec2 CollidePos);
	virtual void CreateNewLaser();

	vec2 m_Dir;
	float m_Energy;
	int m_Bounces;
	bool m_ResetNextTick = false;

	int m_EvalTick;
	int m_SnapTick;
	int m_Owner;
	CClientMask m_TeamMask;
	bool m_ZeroEnergyBounceInLastTick;

	// DDRace

	vec2 m_PrevPos;
	int m_Type;
	int m_TuneZone;
	bool m_TeleportCancelled;
	bool m_IsBlueTeleport;

	//+KZ
	int m_PrevTuneZone;
	float m_MaxEnergy;
	vec2 m_NewLaserPos;
	bool m_CreateNewLaser;
};

#endif
