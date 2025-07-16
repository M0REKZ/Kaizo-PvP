/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_SERVER_ENTITIES_LASER_PVP_H
#define GAME_SERVER_ENTITIES_LASER_PVP_H

#include "laser_kz.h"

class CLaserPvP : public CLaserKZ
{
public:
	CLaserPvP(CGameWorld *pGameWorld, vec2 Pos, vec2 Direction, float StartEnergy, int Owner, int Type, int Bounces = 0);


protected:

	virtual void OnCharacterCollide(CCharacter *pChar, vec2 CollidePos) override;
	virtual void CreateNewLaser() override;

};

#endif
