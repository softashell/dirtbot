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
#include "server_class.h"

#include "bot.h"

#include "in_buttons.h"

#include "bot_mods.h"
#include "bot_globals.h"
#include "bot_weapons.h"
#include "bot_configfile.h"
#include "bot_getprop.h"
#include "bot_neotokyo.h"
#include "bot_navigator.h"
#include "bot_waypoint.h"
#include "bot_waypoint_locations.h"
#include "bot_perceptron.h"

#include "rcbot/logging.h"

void CNeotokyoMod::initMod()
{
	//Load weapons
	CWeapons::loadWeapons(m_szWeaponListName == nullptr ? "NEOTOKYO" : m_szWeaponListName, NEOTOKYOWeaps);
}

void CNeotokyoMod::mapInit()
{
	logger->Log(LogLevel::DEBUG, "[NEOTOKYO] map Init.");
}

int CNeotokyoMod::numClassOnTeam(int iTeam, int iClass)
{
	int num = 0;

	for (int i = 1; i <= CBotGlobals::numClients(); i++)
	{
		edict_t* pEdict = INDEXENT(i);

		if (CBotGlobals::entityIsValid(pEdict))
		{
			if (CBotGlobals::getTeam(pEdict) == iTeam)
			{
				if (CClassInterface::getNEOPlayerClass(pEdict) == iClass)
					num++;
			}
		}
	}

	return num;
}

int CNeotokyoMod::preferredClassOnTeam(int iTeam)
{
	int num = -1;
	int classNum = 1;

	for (int iClass = 1; iClass <= 3; iClass++)
	{
		int tempNum = CNeotokyoMod::numClassOnTeam(iTeam, iClass);
		if (tempNum < num || num == -1)
		{
			num = tempNum;
			classNum = iClass;
		}
	}

	return classNum;
}

int CNeotokyoMod::getRankFromXP(int xp)
{
	if (xp < 0)
	{
		return NEO_RANK_DOG;
	}
	else if (xp < 4)
	{
		return NEO_RANK_PVT;
	}
	else if (xp < 10)
	{
		return NEO_RANK_CPL;
	}
	else if (xp < 20)
	{
		return NEO_RANK_SGT;
	}
	else
	{
		return NEO_RANK_LTN;
	}
}

