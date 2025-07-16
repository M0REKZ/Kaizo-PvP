/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
/* Based on Race mod stuff and tweaked by GreYFoX@GTi and others to fit our DDRace needs. */
#include "dm.h"

#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#define GAME_TYPE_NAME "DM-rw"
#define TEST_TYPE_NAME "TestDM"

CGameControllerDM::CGameControllerDM(class CGameContext *pGameServer) :
	CGameControllerBasePvP(pGameServer)
{
	m_pGameType = g_Config.m_SvTestingCommands ? TEST_TYPE_NAME : GAME_TYPE_NAME;
	m_GameFlags = 0;
}

CGameControllerDM::~CGameControllerDM() = default;

int CGameControllerDM::DoWinCheck()
{

	int HighScore = -1;
	int BestPlayer = -1;

	for(auto pPlayer : GameServer()->m_apPlayers)
	{
		if(!pPlayer)
			continue;

		if(pPlayer->m_ScoreKZ > HighScore)
		{
			BestPlayer = pPlayer->GetCid();
			HighScore = pPlayer->m_ScoreKZ;
		}
	}

	if(HighScore >= g_Config.m_SvScoreLimit)
	{
		return 5 * Server()->TickSpeed();
	}

	return 0;
}
