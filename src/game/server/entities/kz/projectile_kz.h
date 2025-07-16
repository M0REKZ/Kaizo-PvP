#ifndef GAME_SERVER_ENTITIES_KZ_PROJECTILE_KZ_H
#define GAME_SERVER_ENTITIES_KZ_PROJECTILE_KZ_H

#include <game/server/entity.h>

class CProjectileKZ : public CEntity
{
public:
	CProjectileKZ(
		CGameWorld *pGameWorld,
		int Owner,
		vec2 Pos,
		vec2 Dir,
        int Type,
        int EntType = CGameWorld::KZ_ENTTYPE_PROJECTILE_KZ);

	void Reset() override;
	void Tick() override;
	void TickPaused() override;
	void Snap(int SnappingClient) override;

protected:

    vec2 m_Dir;
	int m_LifeSpan;
	int m_Owner;
	int m_StartTick;
    int m_SnapTick;
    int m_Type;
    int m_PrevTuneZone;
    float m_Curvature = 0.f;
    float m_Speed = 0.f;
    CCharacter * m_pInsideChar;

	// DDRace

    int m_Bouncing; //TODO +KZ
	int m_TuneZone;
    CCharacter *m_pSoloChar;

    virtual void Move();
    virtual int Collide(vec2 PrevPos, vec2 *pPreIntersectPos = nullptr, vec2 *pIntersectPos = nullptr, int *pTeleNr = nullptr);
    virtual void OnCollide(vec2 PrevPos, int TileIndex = 0, vec2 *pPreIntersectPos = nullptr, vec2 *pIntersectPos = nullptr, int *pTeleNr = nullptr);
    virtual void OnCharacterCollide(vec2 PrevPos, CCharacter* pChar, vec2 *pIntersectPos = nullptr);

public:

    vec2 m_SnapPos;
    vec2 m_SnapVel;
    int m_SoundImpact;

    //keep those functions for CProjectile compat:

    void SwapClients(int Client1, int Client2) override;
    void SetBouncing(int Value);
    vec2 GetPos(float Time);
	void FillInfo(CNetObj_Projectile *pProj);
	bool FillExtraInfoLegacy(CNetObj_DDRaceProjectile *pProj);
	void FillExtraInfo(CNetObj_DDNetProjectile *pProj);
	int GetOwnerId() const override { return m_Owner; }
    
};

#endif