#ifndef GAME_SERVER_ENTITIES_KZ_PROJECTILE_PVP_H
#define GAME_SERVER_ENTITIES_KZ_PROJECTILE_PVP_H

#include "projectile_kz.h"

class CProjectilePvP : public CProjectileKZ
{
public:
	CProjectilePvP(
		CGameWorld *pGameWorld,
		int Owner,
		vec2 Pos,
		vec2 Vel,
        int Type);

	void Tick() override;

protected:

    virtual bool Move() override;

};

#endif