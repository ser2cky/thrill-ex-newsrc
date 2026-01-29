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

//=======================================
//	hl_wpn_python.cpp
//
//	History:
// 
//	JAN-27-26: Started reimplementing all
//	the Thrill stuff back into here.
//	ItemPostFrame() doesn't call events anymore
//	because the client for some reason can't
//	play it's events.
//
//=======================================

#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "monsters.h"
#include "player.h"
#include "gamerules.h"

enum python_e {
	PYTHON_IDLE1 = 0,
	PYTHON_FIDGET,
	PYTHON_IDLE2,
	PYTHON_IDLE3,
	PYTHON_FIRE1,
	PYTHON_RELOAD,
	PYTHON_DRAW,
	PYTHON_HOLSTER,
	PYTHON_FIRE2
};

LINK_ENTITY_TO_CLASS( weapon_python, CPython );
LINK_WEAPON_TO_CLASS( weapon_357, CPython );

/*
======================================
Spawn
======================================
*/

int CPython::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = "357";
	p->iMaxAmmo1 = _357_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = PYTHON_MAX_CLIP;
	p->iFlags = 0;
	p->iSlot = 1;
	p->iPosition = 1;
	p->iId = m_iId = WEAPON_PYTHON;
	p->iWeight = PYTHON_WEIGHT;

	return 1;
}

/*
======================================
Spawn
======================================
*/

int CPython::AddToPlayer( CBasePlayer *pPlayer )
{
	if ( CBasePlayerWeapon::AddToPlayer( pPlayer ) )
	{
		MESSAGE_BEGIN( MSG_ONE, gmsgWeapPickup, NULL, pPlayer->pev );
			WRITE_BYTE( m_iId );
		MESSAGE_END();
		return TRUE;
	}
	return FALSE;
}

/*
======================================
Spawn
======================================
*/

void CPython::Spawn( )
{
	pev->classname = MAKE_STRING("weapon_357"); // hack to allow for old names
	Precache( );
	m_iId = WEAPON_PYTHON;
	SET_MODEL(ENT(pev), "models/w_357.mdl");

	m_iDefaultAmmo = PYTHON_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

/*
======================================
Spawn
======================================
*/

void CPython::Precache( void )
{
	PRECACHE_MODEL("models/v_357.mdl");
	PRECACHE_MODEL("models/w_357.mdl");
	PRECACHE_MODEL("models/p_357.mdl");

	PRECACHE_MODEL("models/w_357ammobox.mdl");
	PRECACHE_SOUND("items/9mmclip1.wav");              

	PRECACHE_SOUND ("weapons/357_reload1.wav");
	PRECACHE_SOUND ("weapons/357_cock1.wav");
	PRECACHE_SOUND ("weapons/357_shot1.wav");
	PRECACHE_SOUND ("weapons/357_shot2.wav");

	m_usFirePython = PRECACHE_EVENT( 1, "events/python.sc" );
}

/*
======================================
Spawn
======================================
*/

BOOL CPython::Deploy( )
{
	return DefaultDeploy( "models/v_357.mdl", "models/p_357.mdl", PYTHON_DRAW, "python", UseDecrement() );
}

/*
======================================
Spawn
======================================
*/

void CPython::Holster( int skiplocal )
{
	DefaultHolster( PYTHON_HOLSTER );
}

/*
======================================
Spawn
======================================
*/

void CPython::SecondaryAttack( void )
{
	m_flNextSecondaryAttack = UTIL_WeaponTimeBase() + 0.5;
}

/*
======================================
Spawn
======================================
*/

void CPython::PrimaryAttack()
{
	// don't fire underwater
	if (m_pPlayer->pev->waterlevel == 3)
	{
		PlayEmptySound( );
		m_flNextPrimaryAttack = 0.15;
		return;
	}

	if (m_iClip <= 0)
	{
		if (!m_fFireOnEmpty)
			Reload( );
		else
		{
			EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_cock1.wav", 0.8, ATTN_NORM);
			m_flNextPrimaryAttack = 0.15;
		}

		return;
	}

#if 0
	m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
	m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

	m_iClip--;

	m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

	// player "shoot" animation
	m_pPlayer->SetAnimation( PLAYER_ATTACK1 );


	UTIL_MakeVectors( m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle );

	Vector vecSrc	 = m_pPlayer->GetGunPosition( );
	Vector vecAiming = m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	Vector vecDir;
	vecDir = m_pPlayer->FireBulletsPlayer( 1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed );

    int flags;
#if defined( CLIENT_WEAPONS )
	flags = FEV_NOTHOST;
#else
	flags = 0;
#endif

	PLAYBACK_EVENT_FULL( flags, m_pPlayer->edict(), m_usFirePython, 0.0, (float *)&g_vecZero, (float *)&g_vecZero, vecDir.x, vecDir.y, 0, 0, 0, 0 );
#endif


	// SERECKY JAN-27-26: this will make animations appear screwed up in M.P
	m_iForceAnim = 1;
	SendWeaponAnim(PYTHON_FIRE1);
	m_iForceAnim = 0;

	// SERECKY JAN-27-26: Moved this out of ItemPostFrame because the client for some reason forgets the attack delay
	// and ends up never playing the event. People that unknowingly have "cl_lw 1" on suck.
	PLAYBACK_EVENT_FULL(FEV_NOTHOST, m_pPlayer->edict(), m_usFirePython, 0.25f, (float*)&g_vecZero, (float*)&g_vecZero, VECTOR_CONE_1DEGREES.x, VECTOR_CONE_1DEGREES.y, 0, 0, 0, 0);

	// HEV suit - indicate out of ammo condition
	if (!m_iClip && m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] <= 0)
		m_pPlayer->SetSuitUpdate("!HEV_AMO0", FALSE, 0);

	m_pPlayer->m_iWeaponState = WS_SPECIAL;
	m_pPlayer->m_flAttackDelay = gpGlobals->time + 0.25f;

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.35f; // 0.75
	m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10.0f, 15.0f );
}

/*
======================================
Spawn
======================================
*/

void CPython::Reload( void )
{
	if ( m_pPlayer->ammo_357 <= 0 )
		return;

	if (DefaultReload( 6, PYTHON_RELOAD, 2.0f ))
	{
		m_flSoundDelay = 1.5f;
	}
}

/*
======================================
Spawn
======================================
*/

void CPython::WeaponIdle( void )
{
	ResetEmptySound( );

	m_pPlayer->GetAutoaimVector( AUTOAIM_10DEGREES );

	// ALERT( at_console, "%.2f\n", gpGlobals->time - m_flSoundDelay );
	if (m_flSoundDelay && (m_flSoundDelay <= UTIL_WeaponTimeBase()))
	{
		EMIT_SOUND(ENT(m_pPlayer->pev), CHAN_WEAPON, "weapons/357_reload1.wav", RANDOM_FLOAT(0.8, 0.9), ATTN_NORM);
		m_flSoundDelay = 0.0f;
	}

	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );

	if (flRand <= 0.5)
	{
		iAnim = PYTHON_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 51.0f / 30.0f );
	}
	else if (flRand <= 0.7)
	{
		iAnim = PYTHON_IDLE2;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 81.0f / 30.0f );
	}
	else if (flRand <= 0.9)
	{
		iAnim = PYTHON_IDLE3;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 66.0f / 30.0f );
	}
	else
	{
		iAnim = PYTHON_FIDGET;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + ( 91.0f / 30.0f );
	}
	
	SendWeaponAnim( iAnim, UseDecrement() ? 1 : 0 );
}

/*
======================================
ItemPostFrame
======================================
*/

void CPython::ItemPostFrame(void)
{
	if (m_pPlayer->m_flAttackDelay && (m_pPlayer->m_flAttackDelay <= gpGlobals->time))
	{
		m_pPlayer->m_iWeaponVolume = LOUD_GUN_VOLUME;
		m_pPlayer->m_iWeaponFlash = BRIGHT_GUN_FLASH;

		m_iClip--;

		m_pPlayer->pev->effects = (int)(m_pPlayer->pev->effects) | EF_MUZZLEFLASH;

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);


		UTIL_MakeVectors(m_pPlayer->pev->v_angle + m_pPlayer->pev->punchangle);

		Vector vecSrc = m_pPlayer->GetGunPosition();
		Vector vecAiming = m_pPlayer->GetAutoaimVector(AUTOAIM_10DEGREES);

		Vector vecDir;
		vecDir = m_pPlayer->FireBulletsPlayer(1, vecSrc, vecAiming, VECTOR_CONE_1DEGREES, 8192, BULLET_PLAYER_357, 0, 0, m_pPlayer->pev, m_pPlayer->random_seed);

		m_pPlayer->m_flAttackDelay = 0.0f;
	}

	CBasePlayerWeapon::ItemPostFrame();
}

/*
======================================
CPythonAmmo
======================================
*/

class CPythonAmmo : public CBasePlayerAmmo
{
	void Spawn( void )
	{ 
		Precache( );
		SET_MODEL(ENT(pev), "models/w_357ammobox.mdl");
		CBasePlayerAmmo::Spawn( );
	}
	void Precache( void )
	{
		PRECACHE_MODEL ("models/w_357ammobox.mdl");
		PRECACHE_SOUND("items/9mmclip1.wav");
	}
	BOOL AddAmmo( CBaseEntity *pOther ) 
	{ 
		if (pOther->GiveAmmo( AMMO_357BOX_GIVE, "357", _357_MAX_CARRY ) != -1)
		{
			EMIT_SOUND(ENT(pev), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_NORM);
			return TRUE;
		}
		return FALSE;
	}
};
LINK_ENTITY_TO_CLASS( ammo_357, CPythonAmmo );


#endif