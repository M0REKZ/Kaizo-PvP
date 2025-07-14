/* (c) Shereef Marzouk. See "licence DDRace.txt" and the readme.txt in the root of the distribution for more information. */
/* Based on Race mod stuff and tweaked by GreYFoX@GTi and others to fit our DDRace needs. */
#include "base_kz.h"

#include <engine/server.h>
#include <engine/shared/config.h>
#include <game/mapitems.h>
#include <game/server/entities/character.h>
#include <game/server/gamecontext.h>
#include <game/server/player.h>
#include <game/server/score.h>
#include <game/version.h>

#define GAME_TYPE_NAME "None"
#define TEST_TYPE_NAME "TestNone"

CGameControllerBaseKZ::CGameControllerBaseKZ(class CGameContext *pGameServer) :
	CGameControllerDDRace(pGameServer)
{
	m_pGameType = g_Config.m_SvTestingCommands ? TEST_TYPE_NAME : GAME_TYPE_NAME;
	m_GameFlags = 0;
}

CGameControllerBaseKZ::~CGameControllerBaseKZ() = default;

void CGameControllerBaseKZ::HandleCharacterTiles(CCharacter *pChr, int MapIndex)
{
	CGameControllerDDRace::HandleCharacterTiles(pChr, MapIndex);
}

void CGameControllerBaseKZ::SetArmorProgress(CCharacter *pCharacter, int Progress)
{
	//Dont
}

void CGameControllerBaseKZ::OnPlayerConnect(CPlayer *pPlayer)
{
	int ClientId = pPlayer->GetCid();
	pPlayer->Respawn();

	if(!Server()->ClientPrevIngame(ClientId))
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "team_join player='%d:%s' team=%d", ClientId, Server()->ClientName(ClientId), pPlayer->GetTeam());
		GameServer()->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
	}

	if(Server()->IsSixup(ClientId))
	{
		{
			protocol7::CNetMsg_Sv_GameInfo Msg;
			Msg.m_GameFlags = m_GameFlags;
			Msg.m_MatchCurrent = 1;
			Msg.m_MatchNum = 0;
			Msg.m_ScoreLimit = g_Config.m_SvScoreLimit;
			Msg.m_TimeLimit = g_Config.m_SvTimeLimit;
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, ClientId);
		}

		// /team is essential
		{
			protocol7::CNetMsg_Sv_CommandInfoRemove Msg;
			Msg.m_pName = "team";
			Server()->SendPackMsg(&Msg, MSGFLAG_VITAL | MSGFLAG_NORECORD, ClientId);
		}
	}

	// init the player
	Score()->PlayerData(ClientId)->Reset();

	// Can't set score here as LoadScore() is threaded, run it in
	// LoadScoreThreaded() instead
	Score()->LoadPlayerData(ClientId);

	if(!Server()->ClientPrevIngame(ClientId))
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "'%s' entered and joined the %s", Server()->ClientName(ClientId), GetTeamName(pPlayer->GetTeam()));
		GameServer()->SendChat(-1, TEAM_ALL, aBuf, -1, CGameContext::FLAG_SIX);

		GameServer()->SendChatTarget(ClientId, "Kaizo-PvP Mod. DDNet Version: " GAME_VERSION);
		GameServer()->SendChatTarget(ClientId, "please visit DDNet.org or say /info and make sure to read our /rules");
	}
}

void CGameControllerBaseKZ::OnPlayerDisconnect(CPlayer *pPlayer, const char *pReason)
{
	CGameControllerDDRace::OnPlayerDisconnect(pPlayer, pReason);
}

void CGameControllerBaseKZ::OnReset()
{
	CGameControllerDDRace::OnReset();
}

void CGameControllerBaseKZ::Tick()
{
	if(m_PausedTicks > 0)
	{
		return;
	}
	else if(m_PausedTicks == 0)
	{
		m_PausedTicks = -1;
		return;
	}

	CGameControllerDDRace::Tick();

	if(!m_Warmup)
	{
		int Ticks = DoWinCheck();

		if(Ticks)
			EndMatch(Ticks);
	}
}

void CGameControllerBaseKZ::DoTeamChange(class CPlayer *pPlayer, int Team, bool DoChatMsg)
{
	Team = ClampTeam(Team);
	if(Team == pPlayer->GetTeam())
		return;

	CCharacter *pCharacter = pPlayer->GetCharacter();

	if(Team == TEAM_SPECTATORS)
	{
		if(g_Config.m_SvTeam != SV_TEAM_FORCED_SOLO && pCharacter)
		{
			// Joining spectators should not kill a locked team, but should still
			// check if the team finished by you leaving it.
			int DDRTeam = pCharacter->Team();
			Teams().SetForceCharacterTeam(pPlayer->GetCid(), TEAM_FLOCK);
			Teams().CheckTeamFinished(DDRTeam);
		}
	}

	IGameController::DoTeamChange(pPlayer, Team, DoChatMsg);
}

int CGameControllerBaseKZ::DoWinCheck()
{
	return 0;
}

void CGameControllerBaseKZ::EndMatch(int Ticks)
{
	GameServer()->m_World.m_Paused = true;
	m_PausedTicks = Ticks;
}

bool CGameControllerBaseKZ::IsFriendlyFire(int ClientID1, int ClientID2)
{
	if(ClientID1 == ClientID2)
		return false;

	if(IsTeamPlay())
	{
		if(!GameServer()->m_apPlayers[ClientID1] || !GameServer()->m_apPlayers[ClientID2])
			return false;

		if(GameServer()->m_apPlayers[ClientID1]->GetTeam() == GameServer()->m_apPlayers[ClientID2]->GetTeam())
			return true;
	}

	return false;
}