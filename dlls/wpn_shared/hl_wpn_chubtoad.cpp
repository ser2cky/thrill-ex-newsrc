
//=======================================
//	hl_wpn_chubtoad.cpp
// 
//	Purpose: "Weapon" players can throw to
//	distract enemies. Most of the code is
//	rewritten to simplify things.
// 
//	I've simplfied the Chumtoad's AI greatly
//	so it doesn't rely on schedules or any
//	other built-in CBaseMonster stuff.
//
//	History:
//	JAN-19-26: Started
//	JAN-24-26: Started adding logic to the
//	Chumtoad's AI.
//
//=======================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "monsters.h"
#include "weapons.h"

#ifndef CLIENT_DLL

enum chub_state
{
	CHUB_STAND_STILL = 0,
	CHUB_ROAM,
	CHUB_RUN_FOR_IT,
	CHUB_OBSERVE,
	CHUB_PLAYDEAD
};

class CChub : public CBaseMonster
{
public:
	void		Precache(void);
	int			Classify(void) { return CLASS_PLAYER_ALLY; };
	void		Spawn(void);

	void		MonsterThink(void);
	void		MoveThink(float flDist);
	void		ObserveThink(void);
	void		PlayDeadThink(void);

	static		CChub *CreateChumToad(CBasePlayer* pOwner);
private:
	int			m_iChubState;
	float		m_flNextEnemyHunt;
	float		m_flNextGoalChange;
	edict_t		*m_pDmgInflictor;
	EHANDLE		m_hAttacker;
};

LINK_ENTITY_TO_CLASS( monster_chumtoad, CChub );

//===============================
//	Precache
//===============================

void CChub::Precache( void )
{
	PRECACHE_SOUND("chub/frog.wav");
	PRECACHE_MODEL("models/chumtoad.mdl");
}

//===============================
//	Spawn
//===============================

void CChub::Spawn( void )
{
	Precache();

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_TOSS;
	pev->friction = 0.5f;

	SET_MODEL(ENT(pev), "models/chumtoad.mdl");
	UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 24));
	UTIL_SetOrigin(pev, pev->origin);

	pev->effects = 0;
	pev->health = 25.0f;
	pev->yaw_speed = 20.0f;

	pev->view_ofs = Vector(0, 0, 16);
	m_flFieldOfView = VIEW_FIELD_FULL;
	m_bloodColor = BLOOD_COLOR_GREEN;

	pev->sequence = LookupActivity(ACT_IDLE);
	m_iChubState = CHUB_ROAM;
	ResetSequenceInfo();

	MonsterInit();
}

//===============================
//	MonsterThink
//===============================

void CChub::MonsterThink( void )
{
	float flInterval = StudioFrameAdvance();

	if (pev->movetype != MOVETYPE_STEP)
	{
		pev->movetype = MOVETYPE_STEP;
		pev->owner = NULL;
		pev->angles.x = 0.0f;
		pev->nextthink = gpGlobals->time + 0.1f;
	}

	if (m_flNextEnemyHunt <= gpGlobals->time)
	{
		if ( m_hEnemy == NULL || !m_hEnemy->IsAlive() )
		{
			Look(512);
			m_hEnemy = BestVisibleEnemy();
		}
		m_flNextEnemyHunt = gpGlobals->time + 0.5f;
	}

	switch (m_iChubState)
	{
		case CHUB_STAND_STILL:
		{
			break;
		}
		case CHUB_ROAM:
		{
			MoveThink(flInterval);
			break;
		}
		case CHUB_RUN_FOR_IT:
		{
			MoveThink(flInterval);
			break;
		}
		case CHUB_OBSERVE:
		{
			break;
		}
		case CHUB_PLAYDEAD:
		{
			break;
		}
	}

	pev->nextthink = gpGlobals->time;
}


void CChub::MoveThink(float flDist)
{
	int oldseq = pev->sequence;
	int cantmove = 0;
	TraceResult tr;

	Vector vecStart;
	Vector vecEnd;

	if (m_hEnemy != NULL)
		ALERT(at_console, "%.2f\n", (m_hEnemy->pev->origin - pev->origin).Length());

	pev->sequence = LookupActivity(ACT_WALK);
	pev->framerate = 2.5f;

	if (oldseq != pev->sequence)
		ResetSequenceInfo();

	if ( !WALK_MOVE(ENT(pev), pev->angles.y, m_flGroundSpeed * flDist, WALKMOVE_NORMAL) )
	{
		cantmove = 1;
		m_flNextGoalChange = 0.0f;
	}

	if (m_flNextGoalChange <= gpGlobals->time)
	{
		if ( RANDOM_LONG( 0, 5 ) == 0 )
			pev->ideal_yaw = pev->angles.y + RANDOM_FLOAT(-360.0f, 360.0f);

		if ( m_hEnemy != NULL && m_hAttacker == NULL )
			pev->ideal_yaw = UTIL_VecToYaw( m_hEnemy->pev->origin - pev->origin );
		else if ( m_hAttacker != NULL )
			pev->ideal_yaw = -UTIL_VecToYaw( m_hAttacker->pev->origin - pev->origin );

		UTIL_MakeVectors(pev->angles);

		vecStart = pev->origin + pev->view_ofs;
		vecEnd = vecStart + gpGlobals->v_forward * 128.0f;

		UTIL_TraceLine(vecStart, vecEnd, dont_ignore_monsters, ENT(pev), &tr);

		// SERECKY JAN-24-26: Check to see if we've hit a wall so chumtoads can change
		// their direction.
		if ( tr.flFraction < 0.95f || cantmove || tr.pHit && !FNullEnt(tr.pHit) )
		{
			ALERT(at_console, "chub: hit wall %.2f\n", tr.flFraction);
			pev->ideal_yaw = ( pev->angles.y + 90.0f ) + RANDOM_FLOAT(-45.0f, 45.0f);
			cantmove = 0;
		}

		m_flNextGoalChange = gpGlobals->time + 1.5f;
	}

	if (pev->ideal_yaw < -360.0f)
		pev->ideal_yaw += 360.0f;
	if (pev->ideal_yaw > 360.0f)
		pev->ideal_yaw -= 360.0f;

	if (pev->angles.y < -360.0f)
		pev->angles.y += 360.0f;
	if (pev->angles.y > 360.0f)
		pev->angles.y -= 360.0f;

	ChangeYaw(pev->yaw_speed);
}

void CChub::ObserveThink(void)
{

}

void CChub::PlayDeadThink(void)
{

}

//===============================
//	CreateChumToad
//===============================

CChub *CChub::CreateChumToad( CBasePlayer* pOwner )
{
	CChub *pToad = GetClassPtr( (CChub*)NULL );
	pToad->Spawn();

	pToad->pev->owner = pOwner->edict();
	pToad->pev->nextthink = gpGlobals->time + 0.5f;

	pToad->pev->sequence = pToad->LookupActivity(ACT_SWIM);
	pToad->ResetSequenceInfo();

	return pToad;
}

#endif

//===============================
//	Weapon Code
//===============================

LINK_WEAPON_TO_CLASS(weapon_chub, CChubGrenade);

enum chub_e 
{
	CHUB_IDLE1 = 0,
	CHUB_FIDGETLICK,
	CHUB_FIDGETCROAK,
	CHUB_DOWN,
	CHUB_UP,
	CHUB_THROW
};

/*
======================================
Spawn
======================================
*/

void CChubGrenade::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/chumtoad.mdl");
	pev->effects = EF_NODRAW;

	m_iId = WEAPON_CHUB;
	m_iDefaultAmmo = CHUB_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

/*
======================================
Precache
======================================
*/

void CChubGrenade::Precache( void )
{
	UTIL_PrecacheOther("monster_chumtoad");

	// SERECKY JAN-19-26: Needs a W_ and P_
	// model soon.

	PRECACHE_MODEL("models/v_chub.mdl");
}

/*
======================================
GetItemInfo
======================================
*/

int CChubGrenade::GetItemInfo( ItemInfo* p )
{
	p->pszName = STRING(pev->classname);	
	p->pszAmmo1 = "chubtoads";				
	p->iMaxAmmo1 = CHUB_MAX_CARRY;			
	p->pszAmmo2 = NULL;						
	p->iMaxAmmo2 = -1;						
	p->iMaxClip = WEAPON_NOCLIP;			
	p->iSlot = 4;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_CHUB;
	p->iWeight = CHUB_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

/*
======================================
Deploy
======================================
*/

BOOL CChubGrenade::Deploy()
{
	return DefaultDeploy(
		"models/v_chub.mdl",
		"models/p_squeak.mdl",
		CHUB_UP,
		"onehanded",
		0);
}

/*
======================================
Holster
======================================
*/

void CChubGrenade::Holster( int skiplocal )
{

}

/*
======================================
PrimaryAttack
======================================
*/

void CChubGrenade::PrimaryAttack( void )
{
#ifndef CLIENT_DLL
	CChub* pChub = CChub::CreateChumToad(m_pPlayer);

	UTIL_MakeVectors(m_pPlayer->pev->v_angle);

	pChub->pev->origin = m_pPlayer->GetGunPosition() + gpGlobals->v_forward * 24.0f;
	pChub->pev->velocity = gpGlobals->v_forward * 500.0f;
	pChub->pev->angles = m_pPlayer->pev->v_angle;
	pChub->pev->ideal_yaw = 0.0f;

	pChub->ResetSequenceInfo();
#endif

	m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 1.0f;
}

/*
======================================
WeaponIdle
======================================
*/

void CChubGrenade::WeaponIdle( void )
{
	if (m_flTimeWeaponIdle > UTIL_WeaponTimeBase())
		return;

	int iAnim;
	float flRand = UTIL_SharedRandomFloat(m_pPlayer->random_seed, 0, 1);

	if (flRand <= 0.75)
	{
		iAnim = CHUB_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0 / 16 * (2);
	}
	else if (flRand <= 0.875)
	{
		iAnim = CHUB_FIDGETLICK;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0 / 16.0;
	}
	else
	{
		iAnim = CHUB_FIDGETCROAK;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 16.0;
	}
	SendWeaponAnim(iAnim);
}