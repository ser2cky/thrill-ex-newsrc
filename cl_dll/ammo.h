/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/

#ifndef __AMMO_H__
#define __AMMO_H__

#define MAX_WEAPON_NAME 128


#define WEAPON_FLAGS_SELECTONEMPTY	1

#define WEAPON_IS_ONTARGET 0x40

// ThrillEX Addition/Edit Start
struct WEAPON
{
	char	szName[MAX_WEAPON_NAME];
	int		iAmmoType;
	int		iAmmo2Type;
	int		iMax1;
	int		iMax2;
	int		iSlot;
	int		iSlotPos;
	int		iFlags;
	int		iId;
	int		iClip;
	int		iMaxClip;

	int		iCount;		// # of itesm in plist

	HL_SPRITE hActive;
	wrect_t rcActive;

	HL_SPRITE hInactive;
	wrect_t rcInactive;

	HL_SPRITE hAlphaIcon;		// NEW - serecky 1.2.26
	wrect_t rcAlphaIcon;

	HL_SPRITE hQuakeAmmo1;	// SERECKY JAN-18-26: NEW!!!
	wrect_t rcQuakeAmmo1;

	HL_SPRITE hQuakeAmmo2;	// SERECKY JAN-18-26: NEW!!!
	wrect_t rcQuakeAmmo2;
};

typedef int AMMO;

#define ITEM_ALWAYS_DRAW	1
#define ITEM_BAR_COUNT		2
#define ITEM_NUMBER_COUNT	4

typedef struct hud_item_s
{
	char		szName[MAX_WEAPON_NAME];
	int			iCount;
	int			iMax;
	int			iFlags;

	HL_SPRITE		hIcon;
	wrect_t		rcIcon;
	HL_SPRITE		hAlphaIcon;
	wrect_t		rcAlphaIcon;
} HUD_ITEM;
// ThrillEX Addition/Edit End

#endif