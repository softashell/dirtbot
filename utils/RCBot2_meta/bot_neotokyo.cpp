/*
 *    This file is part of RCBot.
 *
 *    RCBot by Paul Murphy adapted from Botman's HPB Bot 2 template.
 *
 *    RCBot is free software; you can redistribute it and/or modify it
 *    under the terms of the GNU General Public License as published by the
 *    Free Software Foundation; either version 2 of the License, or (at
 *    your option) any later version.
 *
 *    RCBot is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with RCBot; if not, write to the Free Software Foundation,
 *    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *    In addition, as a special exception, the author gives permission to
 *    link the code of this program with the Half-Life Game Engine ("HL
 *    Engine") and Modified Game Libraries ("MODs") developed by Valve,
 *    L.L.C ("Valve").  You must obey the GNU General Public License in all
 *    respects for all of the code used other than the HL Engine and MODs
 *    from Valve.  If you modify this file, you may extend this exception
 *    to your version of the file, but you are not obligated to do so.  If
 *    you do not wish to do so, delete this exception statement from your
 *    version.
 *
 */
#include "engine_wrappers.h"

#include "bot.h"
#include "bot_cvars.h"
#include "ndebugoverlay.h"
#include "bot_squads.h"
#include "bot_neotokyo.h"
#include "in_buttons.h"
#include "bot_buttons.h"
#include "bot_globals.h"
#include "bot_profile.h"
#include "bot_getprop.h"
#include "bot_mtrand.h"
#include "bot_mods.h"
#include "bot_task.h"
#include "bot_schedule.h"
#include "bot_weapons.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_navigator.h"
#include "bot_perceptron.h"
#include "bot_plugin_meta.h"
#include "bot_waypoint_visibility.h"

#include <cstring>

extern IVDebugOverlay* debugoverlay;
extern IServerGameEnts* servergameents; // for accessing the server game entities

void CBotNeotokyo::init(bool bVarInit)
{
	CBot::init(bVarInit); // call base first

	m_iXP = 0;
	m_iRank = 0;
	m_iClass = rand() % 2 + 1;
	m_iLoadout = 1;
}

/// <summary>
/// Reset variables after bot dies and after starting game
/// </summary>
void CBotNeotokyo::spawnInit()
{
	CBot::spawnInit();

	if (m_pWeapons) // reset weapons
		m_pWeapons->clearWeapons();

	m_CurrentUtil = BOT_UTIL_MAX;
	m_pNearbyAmmo = nullptr;
	m_pNearbyBattery = nullptr;
	m_pNearbyCrate = nullptr;
	m_pNearbyHealthKit = nullptr;
	m_pNearbyWeapon = nullptr;
	m_pNearbyMine = nullptr;
	m_pNearbyGrenade = nullptr;
	m_pNearbyItemCrate = nullptr;
	m_pCurrentWeapon = nullptr;
	m_flNextSprintTime = engine->Time();
	m_flSuitPower = 0.0f;
	m_flUseCrateTime = engine->Time();
	m_flPickUpTime = engine->Time();
	m_fNextAttackTime = engine->Time();
	m_fNextTeamChange = engine->Time();

	m_bCheckClass = true;
	m_bCheckWeapon = true;

	if (m_pEdict == nullptr)
		return;

	m_iXP = CClassInterface::getNEOPlayerXP(m_pEdict);
	m_iRank = CNeotokyoMod::getRankFromXP(m_iXP);
	//m_iClass = getCurrentClass();

	classInit();
}

int CBotNeotokyo::getCurrentClass()
{
	return CClassInterface::getNEOPlayerClass(m_pEdict);
}

bool CBotNeotokyo::startGame()
{
	const int team = m_pPlayerInfo->GetTeamIndex();

	if (team <= NEO_TEAM_SPECTATOR && m_fNextTeamChange <= engine->Time())
	{
		debugMsg(BOT_DEBUG_GAME_EVENT, "[NEO EVENT] Selecting team");

		if (CBotGlobals::numPlayersOnTeam(NEO_TEAM_JINRAI, false) <= CBotGlobals::numPlayersOnTeam(NEO_TEAM_NSF, false))
			selectTeam(NEO_TEAM_JINRAI);
		else
			selectTeam(NEO_TEAM_NSF);

		selectClass(m_iClass);
		selectLoadout(m_iLoadout);

		m_fNextTeamChange = engine->Time() + 1000;
	}

	return true;
}

void  CBotNeotokyo::classInit()
{
	if (m_pEdict == nullptr)
		return;

	//helpers->ClientCommand(m_pEdict, "vguicancel\n");

	// TODO: Check if class needs changing
	/*f(getCurrentClass() != m_iClass && CClassInterface::getNEOPlayerNextClass(m_pEdict) != m_iClass)
	{
		debugMsg(BOT_DEBUG_GAME_EVENT, "[NEO EVENT] Selecting class");
		selectClass(m_iClass);
	}*/

	selectClass(m_iClass);

	if (!CClassInterface::getNEOPlayerStar(m_pEdict))
	{
		helpers->ClientCommand(m_pEdict, "joinstar 1\n");
	}

	// TODO: Reuse weapon or pick something else
	debugMsg(BOT_DEBUG_GAME_EVENT, "[NEO EVENT] Selecting loadout");

	switch (m_iRank)
	{
	case NEO_RANK_PVT:
		switch (m_iClass)
		{
		case NEO_CLASS_RECON:
			m_iLoadout = rand() % 2; // Any weapon
			break;
		case NEO_CLASS_ASSAULT:
			m_iLoadout = rand() % 1 ? 1 : 4; // SRM or ZR68S
			break;
		case NEO_CLASS_SUPPORT:
			m_iLoadout = 1 + rand() % 3; // Everything but MPN
			break;
		}
		break;
	case NEO_RANK_CPL:
		switch (m_iClass)
		{
		case NEO_CLASS_RECON:
			m_iLoadout = 1 + rand() % 4; // Any weapon, except MPN
			break;
		case NEO_CLASS_ASSAULT:
			m_iLoadout = rand() % 1 ? 1 : 4 + rand() % 1; // SRM or ZR68S, SUPA
			break;
		case NEO_CLASS_SUPPORT:
			m_iLoadout = 5; // MX
			break;
		}
		break;
	case NEO_RANK_SGT:
		switch (m_iClass)
		{
		case NEO_CLASS_RECON:
			m_iLoadout = 1 + rand() % 5; // Any weapon, except MPN
			break;
		case NEO_CLASS_ASSAULT:
			m_iLoadout = 8 + rand() % 1; // MX or MXS
			break;
		case NEO_CLASS_SUPPORT:
			m_iLoadout = rand() % 1 ? 5 : 7; // MX or MXS
			break;
		}
		break;
	case NEO_RANK_LTN:
		switch (m_iClass)
		{
		case NEO_CLASS_RECON:
			m_iLoadout = 1 + rand() % 7; // Any weapon, except MPN
			break;
		case NEO_CLASS_ASSAULT:
			m_iLoadout = 8 + rand() % 1; // MX or MXS
			break;
		case NEO_CLASS_SUPPORT:
			m_iLoadout = rand() % 1 ? 5 : 7; // MX or MXS
			break;
		}
		break;

	default:
		m_iLoadout = 0;
	}

	selectLoadout(m_iLoadout);
}

void CBotNeotokyo::currentlyDead()
{
	tapButton(IN_ATTACK);

	// keep updating until alive
	m_fSpawnTime = engine->Time();
}

void CBotNeotokyo::selectTeam(int iTeam) const
{
	char cmd[32];

	// jointeam a
	sprintf(cmd, "jointeam %d\n", iTeam);

	helpers->ClientCommand(m_pEdict, cmd);
}

void CBotNeotokyo::selectClass(int iClass)
{
	char cmd[32];

	if (iClass < NEO_CLASS_RECON || iClass > NEO_CLASS_SUPPORT)
		return;

	sprintf(cmd, "setclass %d\n", iClass);
	helpers->ClientCommand(m_pEdict, cmd);

	sprintf(cmd, "SetVariant %d\n", rand() % 2);
	helpers->ClientCommand(m_pEdict, cmd);

	// TODO: Validate that it worked?

	m_fChangeClassTime = engine->Time() + randomFloat(bot_min_cc_time.GetFloat(), bot_max_cc_time.GetFloat());
}

void CBotNeotokyo::selectLoadout(int iLoadout) const
{
	char cmd[32];

	sprintf(cmd, "loadout %d\n", iLoadout);

	helpers->ClientCommand(m_pEdict, cmd);
}

void CBotNeotokyo::died(edict_t* pKiller, const char* pszWeapon)
{
	CBot::died(pKiller, pszWeapon);

	if (pKiller)
	{
		if (CBotGlobals::entityIsValid(pKiller))
		{
			m_pNavigator->belief(CBotGlobals::entityOrigin(pKiller), getEyePosition(), bot_beliefmulti.GetFloat(), distanceFrom(pKiller), BELIEF_DANGER);
		}
	}
}

/**
 * Determines if the bot needs health
 *
 * @return          True if the bot needs health
 **/
bool CBotNeotokyo::needHealth()
{
	return getHealthPercent() <= 0.7f;
}

/**
 * Determines if the bot needs ammo
 *
 * @return          True if the bot needs ammo
 **/
bool CBotNeotokyo::needAmmo()
{
	if (m_pCurrentWeapon == nullptr)
	{
		return false;
	}

	const CBotWeapon* weapon = getPrimaryWeapon();
	if (weapon == nullptr)
		return false;

	//const CBotWeapon* weapon = m_pWeapons->getWeapon(CWeapons::getWeapon(m_pCurrentWeapon->GetClassName()));
		//const int iAmmo = weapon->getAmmo(this); // Current weapon reserve ammo
	if (weapon && weapon->getClip1(this) == 0 && !weapon->isMelee() && weapon->getID() != NEO_WEAPON_GHOST)
	{
		return true;
	}

	return false;
}

void CBotNeotokyo::modThink()
{
	m_fIdealMoveSpeed = CClassInterface::getMaxSpeed(m_pEdict);
	m_pCurrentWeapon = CClassInterface::getCurrentWeapon(m_pEdict);

	// when respawned -- check if I should change class
	if (m_bCheckClass && !m_pPlayerInfo->IsDead())
	{
		m_bCheckClass = false;

		if (bot_change_class.GetBool() && (m_fChangeClassTime < engine->Time()))
		{
			m_iClass = rand() % 2 + 1;
		}

		classInit();
	}

	if (needHealth())
		updateCondition(CONDITION_NEED_HEALTH);
	else
		removeCondition(CONDITION_NEED_HEALTH);

	if (needAmmo())
	{
		letGoOfButton(IN_ATTACK);
		tapButton(IN_RELOAD);
		updateCondition(CONDITION_OUT_OF_AMMO);
		updateCondition(CONDITION_NEED_AMMO);
	}
	else
	{
		removeCondition(CONDITION_NEED_AMMO);
		removeCondition(CONDITION_OUT_OF_AMMO);
	}

	if (onLadder())
	{
		setMoveLookPriority(MOVELOOK_OVERRIDE);
		setLookAtTask(LOOK_WAYPOINT);
		m_pButtons->holdButton(IN_FORWARD, 0, 1, 0);
		setMoveLookPriority(MOVELOOK_MODTHINK);
	}

	if (m_pNearbyGrenade && distanceFrom(m_pNearbyGrenade.get()) <= 200.0f) // Nearby grenade, RUN for cover!
	{
		updateCondition(CONDITION_RUN);
		if (!m_pSchedules->isCurrentSchedule(SCHED_GOOD_HIDE_SPOT))
		{
			m_pSchedules->removeSchedule(SCHED_GOOD_HIDE_SPOT);
			m_pSchedules->addFront(new CGotoHideSpotSched(this, m_pNearbyGrenade, false)); // bIsGrenade is false because when true the bot will do a DoD specific task
			debugMsg(BOT_DEBUG_THINK, "[MOD THINK] Taking cover from grenade");
		}
	}

	// Pick nearby weapons that the bot doesn't already have
	if (m_pNearbyWeapon && distanceFrom(m_pNearbyWeapon.get()) <= 400.0f && m_flPickUpTime <= engine->Time())
	{
		const edict_t* pOwner = CClassInterface::getOwner(m_pNearbyWeapon);

		if (pOwner != nullptr) // Someone already owns this weapon
		{
			m_pNearbyWeapon = nullptr;
		}
		else
		{
			if (!m_pSchedules->isCurrentSchedule(SCHED_PICKUP))
			{
				m_pSchedules->removeSchedule(SCHED_PICKUP);
				m_pSchedules->addFront(new CBotPickupSched(m_pNearbyWeapon.get()));
				debugMsg(BOT_DEBUG_THINK, "[MOD THINK] Picking up weapon");
				m_flPickUpTime = engine->Time() + randomFloat(5.0f, 10.0f);
			}
		}
	}

	/**
	 * Bot sprinting logic
	 **/
	if (hasSomeConditions(CONDITION_RUN) && m_flSuitPower > 1.0f && m_flNextSprintTime <= engine->Time()) // The bot wants to sprint
	{
		m_pButtons->holdButton(IN_SPEED, 0.0f, 1.0f, 0.0f);
	}
	else if (m_fCurrentDanger >= 75.0f && m_flSuitPower > 1.0f && !isUnderWater()) // dangerous area, sprint
	{
		m_pButtons->holdButton(IN_SPEED, 0.0f, 1.0f, 0.0f);
	}
	else if (m_flSuitPower < 1.0f) // Low on suit power, don't sprint for a while
	{
		m_flNextSprintTime = engine->Time() + randomFloat(8.0f, 10.0f);
		removeCondition(CONDITION_RUN);
	}
	else if (isUnderWater()) // In Synergy/HL2 suit power is also used for oxygen
	{
		m_flNextSprintTime = engine->Time() + 0.5f;
	}
}

void CBotNeotokyo::updateConditions()
{
	CBot::updateConditions();

	if (m_pEnemy.get() != nullptr)
	{
		if (CDataInterface::GetEntityHealth(m_pEnemy.get()->GetNetworkable()->GetBaseEntity()) <= 0)
		{
			updateCondition(CONDITION_ENEMY_DEAD);
			m_pNavigator->belief(getOrigin(), CBotGlobals::entityOrigin(m_pEnemy), bot_belief_fade.GetFloat(), distanceFrom(m_pEnemy), BELIEF_SAFETY);
			enemyDown(m_pEnemy);
			m_pEnemy = nullptr;
		}
	}
}

bool CBotNeotokyo::isEnemy(edict_t* pEdict, bool bCheckWeapons)
{
	if (m_pEdict == pEdict) // Not self
		return false;

	if (ENTINDEX(pEdict) >= CBotGlobals::maxClients()) 
		return false;

	if (rcbot_notarget.GetBool())
		return false;

	if (!CBotGlobals::isAlivePlayer(pEdict) || !isVisible(pEdict))
		return false;

	if (CBotGlobals::getTeam(pEdict) >= NEO_TEAM_JINRAI && CBotGlobals::getTeam(pEdict) <= NEO_TEAM_NSF && getTeam() >= NEO_TEAM_JINRAI)
	{
		if (CBotGlobals::getTeam(pEdict) != getTeam())
		{
			return true;
		}
		else if (rcbot_ffa.GetBool())
		{
			return true;
		}
	}

	return false;
}

bool CBotNeotokyo::setVisible(edict_t* pEntity, bool bVisible)
{
	const bool bValid = CBot::setVisible(pEntity, bVisible);

	static float fDist = distanceFrom(pEntity);
	const Vector entityorigin = CBotGlobals::entityOrigin(pEntity);
	const char* szclassname = pEntity->GetClassName();

	// Is valid and NOT invisible
	if (bValid && bVisible && !(CClassInterface::getEffects(pEntity) & EF_NODRAW))
	{
		if (std::strncmp(szclassname, "weapon_", 7) == 0 && (!m_pNearbyWeapon.get() || fDist < distanceFrom(m_pNearbyWeapon.get())))
		{
			const CBotWeapon* pWeapon = m_pWeapons->getWeapon(CWeapons::getWeapon(szclassname));
			if (pWeapon && pWeapon->hasWeapon())
			{
				m_pNearbyWeapon = nullptr; // bot already has this weapon
			}
			else
			{
				const edict_t* pOwner = CClassInterface::getOwner(pEntity);
				if (pOwner == nullptr) // Don't pick weapons owned by someone
				{
					m_pNearbyWeapon = pEntity;
				}
			}
		}
		else if (std::strncmp(szclassname, "weapon_grenade", 16) == 0 && (!m_pNearbyGrenade.get() || fDist < distanceFrom(m_pNearbyGrenade.get())))
		{
			const edict_t* pOwner = CClassInterface::getOwner(pEntity);
			const IPlayerInfo* p = playerinfomanager->GetPlayerInfo(pEntity);
			if (pOwner == nullptr || p == nullptr) // Only care about grenades that doesn't have an owner or isn't owned by a player
			{
				m_pNearbyGrenade = pEntity;
				const int iWaypoint = CWaypointLocations::NearestWaypoint(entityorigin, 512.0f, -1);
				if (iWaypoint != -1)
				{
					m_pNavigator->beliefOne(iWaypoint, BELIEF_DANGER, distanceFrom(pEntity));
				}
			}
		}
		else if (std::strncmp(szclassname, "weapon_remotedet", 12) == 0 && (!m_pNearbyMine.get() || fDist < distanceFrom(m_pNearbyMine.get())))
		{
			if (!CSynergyMod::IsCombineMinePlayerPlaced(pEntity)) // Ignore player placed (friendly) mines
			{
				m_pNearbyMine = pEntity;
				const int iWaypoint = CWaypointLocations::NearestWaypoint(entityorigin, 512.0f, -1);
				if (iWaypoint != -1)
				{
					m_pNavigator->beliefOne(iWaypoint, BELIEF_DANGER, distanceFrom(pEntity));
				}
			}
		}
	}
	else
	{
		if (pEntity == m_pNearbyAmmo.get_old())
			m_pNearbyAmmo = nullptr;
		else if (pEntity == m_pNearbyCrate.get_old())
			m_pNearbyCrate = nullptr;
		else if (pEntity == m_pNearbyHealthKit.get_old())
			m_pNearbyHealthKit = nullptr;
		else if (pEntity == m_pNearbyBattery.get_old())
			m_pNearbyBattery = nullptr;
		else if (pEntity == m_pNearbyWeapon.get_old())
			m_pNearbyWeapon = nullptr;
		else if (pEntity == m_pNearbyMine.get_old())
			m_pNearbyMine = nullptr;
		else if (pEntity == m_pNearbyGrenade.get_old())
			m_pNearbyGrenade = nullptr;
		else if (pEntity == m_pNearbyItemCrate.get_old())
			m_pNearbyItemCrate = nullptr;
		else if (pEntity == m_pNearbyHealthCharger.get_old())
			m_pNearbyHealthCharger = nullptr;
		else if (pEntity == m_pNearbyArmorCharger.get_old())
			m_pNearbyArmorCharger = nullptr;
	}

	return bValid;
}

void CBotNeotokyo::getTasks(unsigned int iIgnore)
{
	static CBotUtilities utils;
	static CBotUtility* next;
	static bool bCheckCurrent;

	if (!hasSomeConditions(CONDITION_CHANGED) && !m_pSchedules->isEmpty())
		return;

	removeCondition(CONDITION_CHANGED);
	bCheckCurrent = true; // important for checking current schedule
	setMoveSpeed(CClassInterface::getMaxSpeed(m_pEdict)); // Some tasks changes the bot move speed, reset it back.

	// Utilities
	ADD_UTILITY(BOT_UTIL_SNIPE, IsSniper(), randomFloat(0.7900f, 0.8200f))

	// Combat Utilities
	ADD_UTILITY(BOT_UTIL_ENGAGE_ENEMY, hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && !hasSomeConditions(CONDITION_OUT_OF_AMMO), 1.00f)
	ADD_UTILITY(BOT_UTIL_WAIT_LAST_ENEMY, hasSomeConditions(CONDITION_ENEMY_OBSCURED), 0.95f)
	ADD_UTILITY(BOT_UTIL_HIDE_FROM_ENEMY, hasSomeConditions(CONDITION_SEE_CUR_ENEMY) && hasSomeConditions(CONDITION_OUT_OF_AMMO), 0.98f)

	// Generic Utilities
	ADD_UTILITY(BOT_UTIL_ATTACK_POINT, true, 0.01f); // Go to waypoints with 'goal' flag
	ADD_UTILITY(BOT_UTIL_ROAM, true, 0.001f) // Roam around

	ADD_UTILITY(BOT_UTIL_FIND_LAST_ENEMY,
		wantToFollowEnemy() && !m_bLookedForEnemyLast && m_pLastEnemy && CBotGlobals::entityIsValid(m_pLastEnemy
		) && CBotGlobals::entityIsAlive(m_pLastEnemy), getHealthPercent() * (getArmorPercent() + 0.1f))

	utils.execute();

	while ((next = utils.nextBest()) != nullptr)
	{
		if (!m_pSchedules->isEmpty() && bCheckCurrent)
		{
			if (m_CurrentUtil != next->getId())
				m_pSchedules->freeMemory();
			else
				break;
		}

		bCheckCurrent = false;

		if (executeAction(next->getId()))
		{
			m_CurrentUtil = next->getId();
			m_flInterruptTime = engine->Time() + randomFloat(30.0f, 45.0f);

			if (m_fUtilTimes[next->getId()] < engine->Time())
				m_fUtilTimes[next->getId()] = engine->Time() + randomFloat(0.1f, 2.0f); // saves problems with consistent failing

			if (CClients::clientsDebugging(BOT_DEBUG_UTIL))
			{
				CClients::clientDebugMsg(BOT_DEBUG_UTIL, g_szUtils[next->getId()], this);
			}
			break;
		}
	}

	utils.freeMemory();
}

bool CBotNeotokyo::executeAction(eBotAction iAction)
{
	switch (iAction)
	{
	case BOT_UTIL_PICKUP_WEAPON:
		m_pSchedules->add(new CBotPickupSched(m_pNearbyWeapon.get()));
		m_fUtilTimes[BOT_UTIL_PICKUP_WEAPON] = engine->Time() + randomFloat(5.0f, 10.0f);
		return true;
	case BOT_UTIL_HIDE_FROM_ENEMY:
	{
		CBotSchedule* pSched = new CBotSchedule();
		pSched->setID(SCHED_RUN_FOR_COVER);
		const int cover = CWaypointLocations::GetCoverWaypoint(getOrigin(), CBotGlobals::entityOrigin(m_pEnemy.get()), nullptr, nullptr, 0, 8.0f, 1024.0f);
		if (cover != -1)
		{
			CBotTask* pTask = new CFindPathTask(cover);
			pTask->setCompleteInterrupt(CONDITION_ENEMY_DEAD, CONDITION_OUT_OF_AMMO);
			pSched->addTask(pTask);
			m_pSchedules->add(pSched);
			return true;
		}
		break;
	}
	case BOT_UTIL_ATTACK_POINT:
	{
		// roam
		CBotSchedule* pSched = new CBotSchedule();
		m_fUtilTimes[BOT_UTIL_ATTACK_POINT] = engine->Time() + randomFloat(60.0f, 180.0f);

		pSched->setID(SCHED_ATTACKPOINT);

		// Make the bot more likely to use alternate paths based on their braveness and current health
		if (getHealthPercent() + m_pProfile->m_fBraveness <= 1.0f)
			updateCondition(CONDITION_COVERT);
		else
			removeCondition(CONDITION_COVERT);

		CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_GOAL);

		if (pWaypoint)
		{
			CBotTask* pFindPath;
			CWaypoint* pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), 0, 0);
			if ((m_fUseRouteTime <= engine->Time()))
			{
				if (pRoute)
				{
					const int iRoute = CWaypoints::getWaypointIndex(pRoute); // Route waypoint
					pFindPath = new CFindPathTask(iRoute, LOOK_WAYPOINT);
					pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
					pSched->addTask(pFindPath);
					pSched->addTask(new CMoveToTask(pRoute->getOrigin()));
					m_pSchedules->add(pSched);
					m_fUseRouteTime = engine->Time() + 30.0f;
				}
			}

			const int iWaypoint = CWaypoints::getWaypointIndex(pWaypoint);
			pFindPath = new CFindPathTask(iWaypoint, LOOK_WAYPOINT);
			pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
			pSched->addTask(pFindPath);
			pSched->addTask(new CMoveToTask(pWaypoint->getOrigin()));
			m_pSchedules->add(pSched);

			return true;
		}

		break;
	}
	case BOT_UTIL_SNIPE:
	{
		CBotSchedule* pSched = new CBotSchedule();
		pSched->setID(SCHED_SNIPE);

		CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(CWaypointTypes::W_FL_SNIPER, getTeam(), 0, false);
		if (pWaypoint)
		{
			CFindPathTask* pFindPath = new CFindPathTask(CWaypoints::getWaypointIndex(pWaypoint));
			//pFindPath->setInterruptFunction(new CBotCSSRoamInterrupt());
			CCSSGuardTask* pGuard = new CCSSGuardTask(getPrimaryWeapon(), pWaypoint->getOrigin(), pWaypoint->getAimYaw(), false, 0.0f, pWaypoint->getFlags());
			pSched->addTask(pFindPath);
			pSched->addTask(pGuard);
			m_pSchedules->add(pSched);
			return true;
		}
		break;
	}
	case BOT_UTIL_ROAM:
	{
		// roam
		CBotSchedule* pSched = new CBotSchedule();

		pSched->setID(SCHED_GOTO_ORIGIN);

		// Make the bot more likely to use alternate paths based on their braveness and current health
		if (getHealthPercent() + m_pProfile->m_fBraveness <= 1.0f)
			updateCondition(CONDITION_COVERT);
		else
			removeCondition(CONDITION_COVERT);

		CWaypoint* pWaypoint = CWaypoints::randomWaypointGoal(-1);

		if (pWaypoint)
		{
			CBotTask* pFindPath;
			CWaypoint* pRoute = CWaypoints::randomRouteWaypoint(this, getOrigin(), pWaypoint->getOrigin(), 0, 0);
			if ((m_fUseRouteTime <= engine->Time()))
			{
				if (pRoute)
				{
					const int iRoute = CWaypoints::getWaypointIndex(pRoute); // Route waypoint
					pFindPath = new CFindPathTask(iRoute, LOOK_WAYPOINT);
					pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
					pSched->addTask(pFindPath);
					pSched->addTask(new CMoveToTask(pRoute->getOrigin()));
					m_pSchedules->add(pSched);
					m_fUseRouteTime = engine->Time() + 30.0f;
				}
			}

			const int iWaypoint = CWaypoints::getWaypointIndex(pWaypoint);
			pFindPath = new CFindPathTask(iWaypoint, LOOK_WAYPOINT);
			pFindPath->setInterruptFunction(new CBotSYNRoamInterrupt());
			pSched->addTask(pFindPath);
			pSched->addTask(new CMoveToTask(pWaypoint->getOrigin()));
			m_pSchedules->add(pSched);

			return true;
		}

		break;
	}
	} //Not required? [APG]RoboCop[CL]

	return false;
}

void CBotNeotokyo::touchedWpt(CWaypoint* pWaypoint, int iNextWaypoint, int iPrevWaypoint)
{
	CBot::touchedWpt(pWaypoint, iNextWaypoint, iPrevWaypoint);
}

bool CBotNeotokyo::walkingTowardsWaypoint(CWaypoint* pWaypoint, bool* bOffsetApplied, Vector& vOffset)
{
	return CBot::walkingTowardsWaypoint(pWaypoint, bOffsetApplied, vOffset);
}

void CBotNeotokyo::reachedCoverSpot(int flags)
{
	removeCondition(CONDITION_RUN); // Remove when in cover
}

void CBotNeotokyo::handleWeapons()
{
	CBot::handleWeapons();
}

bool CBotNeotokyo::handleAttack(CBotWeapon* pWeapon, edict_t* pEnemy)
{
	//const char* szclassname = pEnemy->GetClassName();

	static float fDistance;
	fDistance = distanceFrom(pEnemy);

	if ((fDistance > 128) && (DotProductFromOrigin(m_vAimVector) < rcbot_enemyshootfov.GetFloat()))
		return true; // keep enemy / don't shoot : until angle between enemy is less than 45 degrees

	if (pWeapon == nullptr)
		return false;

	clearFailedWeaponSelect();

	if ((!pWeapon->isMelee() || pWeapon->isSpecial()) && pWeapon->outOfAmmo(this))
		return false; // change weapon/enemy

	if (pWeapon->isMelee())
	{
		setMoveTo(CBotGlobals::entityOrigin(pEnemy));
		setLookAtTask(LOOK_ENEMY);
		m_fAvoidTime = engine->Time() + 1.0f;
	}

	if (pWeapon->canUseSecondary() && pWeapon->getAmmo(this, 2) && pWeapon->secondaryInRange(fDistance))
	{
		if (randomInt(0, 1))
		{
			secondaryAttack();
			return true;
		}
	}

	if (pWeapon->isMelee() && pWeapon->canAttack() && pWeapon->primaryInRange(fDistance))
	{
		primaryAttack(true);

		return true;
	}

	// can use primary
	if (!pWeapon->isMelee() && pWeapon->canAttack() && pWeapon->primaryInRange(fDistance))
	{
		if (pWeapon->mustHoldAttack())
			primaryattackNEO(true);
		else
			primaryattackNEO(false);

		return true;
	}

	return false;
}

void CBotNeotokyo::primaryattackNEO(bool hold)
{
	if (hold)
	{
		primaryAttack(hold);
	}
	else
	{
		if (m_fNextAttackTime <= engine->Time())
		{
			tapButton(IN_ATTACK);
			m_fNextAttackTime = engine->Time() + getNextAttackDelay(); // 50 ms delay between shots
		}
		else
		{
			letGoOfButton(IN_ATTACK);
		}
	}
}

float CBotNeotokyo::getNextAttackDelay()
{
	static constexpr float max = 4096.0f;
	static float dist;
	static float delay;
	delay = 0.050f; // Base delay

	dist = distanceFrom(getEnemy());
	delay = dist / max;
	clamp(delay, 0.050f, 0.300f);

	//CClients::clientDebugMsg(this, BOT_DEBUG_AIM, "[NEO ATTACK] Next Attack Delay: %2.4f", delay);

	return delay;
}

/**
 * This functions is called by task interruptions check to see if the bot should change it's current task
 *
 * @return 		TRUE if the bot should interrupt it's current task
 **/
bool CBotNeotokyo::wantsToChangeCourseOfAction()
{
	return false; // TO-DO
}


/**
 * Checks if the bot is carrying a sniper weapon
 *
 * @return		TRUE if the bot is carrying a sniper, FALSE otherwise
 **/
bool CBotNeotokyo::IsSniper()
{
	const CBotWeapon* weapon = getPrimaryWeapon();

	if (!weapon)
		return false;

	switch (weapon->getID())
	{
		case NEO_WEAPON_SRS:
		case NEO_WEAPON_M41:
		case NEO_WEAPON_M41_L:
		case NEO_WEAPON_M41_S:
		case NEO_WEAPON_ZR68_L:
		{
			return true;
		}
	}

	return false;
}

/**
 * Gets the bot primary weapon (will fallback to secondary if the bot doesn't have a primary)
 **/
CBotWeapon* CBotNeotokyo::getPrimaryWeapon()
{
	static CBotWeapon* primary;
	static CBotWeapon* secondary;
	primary = m_pWeapons->getCurrentWeaponInSlot(NEO_WEAPON_SLOT_PRIMARY);
	secondary = m_pWeapons->getCurrentWeaponInSlot(NEO_WEAPON_SLOT_SECONDARY);

	if (primary)
		return primary;

	if (secondary)
		return secondary;

	return nullptr;
}