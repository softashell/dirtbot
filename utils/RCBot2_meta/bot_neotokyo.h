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
#ifndef _BOT_NEOTOKYO_H_
#define _BOT_NEOTOKYO_H_

#define NEO_TEAM_UNASSIGNED 0
#define NEO_TEAM_SPECTATOR 1
#define NEO_TEAM_JINRAI 2
#define NEO_TEAM_NSF 3

#define NEO_CLASS_UNASSIGNED 0
#define NEO_CLASS_RECON 1
#define NEO_CLASS_ASSAULT 2
#define NEO_CLASS_SUPPORT 3


enum
{
	NEO_WEAPON_SLOT_NONE = 0,
	NEO_WEAPON_SLOT_PRIMARY = 1,
	NEO_WEAPON_SLOT_SECONDARY = 2,
	NEO_WEAPON_SLOT_MELEE = 3,
	NEO_WEAPON_SLOT_GRENADE = 4
};

enum
{
	NEO_RANK_DOG = 0,
	NEO_RANK_PVT,
	NEO_RANK_CPL,
	NEO_RANK_SGT,
	NEO_RANK_LTN
};

class CBotNeotokyo : public CBot
{
public:
	bool isNEO() override { return true; }
	void init (bool bVarInit=false) override;
	void spawnInit () override;
	void classInit();
	bool startGame() override;
	void currentlyDead() override;
	void selectTeam(int iTeam) const;
	void selectClass(int iClass);
	void selectLoadout(int iLoadout) const;
	void died ( edict_t *pKiller, const char *pszWeapon ) override;
	void modThink () override;
	void getTasks (unsigned int iIgnore=0) override;
	virtual bool executeAction(eBotAction iAction); //TODO: not implemented yet? [APG]RoboCop[CL]
	virtual float getArmorPercent() { return static_cast<int>(0.01f * m_pPlayerInfo->GetArmorValue()); }
	unsigned int maxEntityIndex() override { return gpGlobals->maxEntities; }
	bool isEnemy ( edict_t *pEdict, bool bCheckWeapons = true ) override;
	bool setVisible ( edict_t *pEntity, bool bVisible ) override;
	void touchedWpt ( CWaypoint *pWaypoint, int iNextWaypoint = -1, int iPrevWaypoint = -1 ) override;
	bool walkingTowardsWaypoint ( CWaypoint *pWaypoint, bool *bOffsetApplied, Vector &vOffset ) override;
	void reachedCoverSpot (int flags) override;
	void updateConditions () override; // Overridden due to Synergy's quirks
	void handleWeapons() override;
	bool handleAttack(CBotWeapon *pWeapon, edict_t *pEnemy) override;
	void primaryattackNEO(bool hold);
	float getNextAttackDelay();
	virtual bool needHealth();
	virtual bool needAmmo();
	virtual bool wantsToChangeCourseOfAction();
	float getInterruptionTimer() const { return m_flInterruptTime; }
	int getCurrentClass();
	virtual CBotWeapon* getPrimaryWeapon();
	virtual bool IsSniper();
protected:
	MyEHandle m_pNearbyWeapon; // weapons
	MyEHandle m_pNearbyHealthKit; // Healthkit
	MyEHandle m_pNearbyBattery; // Armor battery
	MyEHandle m_pNearbyAmmo; // ammo pickups
	MyEHandle m_pNearbyCrate; // ammo crate
	MyEHandle m_pNearbyGrenade; // grenades
	MyEHandle m_pNearbyMine; // combine mine
	MyEHandle m_pNearbyItemCrate; // breakable item crate
	MyEHandle m_pNearbyHealthCharger; // Health charger
	MyEHandle m_pNearbyArmorCharger; // Armor/Suit charger
	edict_t * m_pCurrentWeapon = nullptr; // The bot current weapon
	float m_flSuitPower = 0.0f; // HEV suit power level, range: 100-0
	float m_flNextSprintTime = 0.0f; // Used to control the bot's sprinting
	float m_flUseCrateTime = 0.0f; // Use ammo crate time delay
	float m_flPickUpTime = 0.0f; // Pick ammo delay
	float m_flInterruptTime = 0.0f; // Time delay for general interruptions
	float m_fNextAttackTime = 0.0f;	  		// Control timer for bot primary attack
	float m_fNextTeamChange = 0.0f;
	float m_fChangeClassTime = 0.0f;
	int m_iXP;
	int m_iRank;
	int m_iClass;
	int m_iLoadout;
	bool m_bCheckClass;
	bool m_bCheckWeapon;
};

#endif