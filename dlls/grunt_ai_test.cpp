
//=======================================
//	grunt_ai_test.cpp
//	rewrote parts of the hgrunt code so i 
//	can learn more about hl1 monster code
//	DISREGARD THIS CODE!!!
// 
//	History:
// 
//	FEB-09-26: started. brought a few funcs.
//	back from half-life alpha...
//
//=======================================

#if 0

#include	"extdll.h"
#include	"plane.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"animation.h"
#include	"squadmonster.h"
#include	"weapons.h"
#include	"talkmonster.h"
#include	"soundent.h"
#include	"customentity.h"

//=========================================================
// Monster's Anim Events Go Here
//=========================================================
#define		HGRUNT_AE_RELOAD		( 2 )
#define		HGRUNT_AE_KICK			( 3 )
#define		HGRUNT_AE_SHOOT			( 4 )

#define		STRAFE_NONE				0
#define		STRAFE_RIGHT			1
#define		STRAFE_LEFT				2

class CHSarge : public CSquadMonster
{
public:
	void		Spawn ( void );
	void		Precache ( void );
	int			Classify ( void );

	void		PainSound ( void );
	void		DeathSound ( void );
	void		Shoot ( void );
	void		Kick ( void );

	BOOL		FindLateralCover( const Vector &vecThreat, const Vector &vecViewOffset );
	void		EXPORT MonsterThink ( void );
	int			TakeDamage ( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType );

	float		m_flPainFinished;
	int			m_iStrafeMode;
	int			m_iCanStrafe;
	Vector		m_vecStrafeGoal;
};

LINK_ENTITY_TO_CLASS( monster_hsarge, CHSarge );

//===============================
//	Function Name
//===============================

void CHSarge::Precache(void)
{
	PRECACHE_MODEL("models/hsarge1.mdl");

	PRECACHE_SOUND("player/hoot1.wav");
	PRECACHE_SOUND("hgrunt/gr_reload1.wav");
	PRECACHE_SOUND("hgrunt/gr_loadtalk.wav");

	PRECACHE_SOUND("hgrunt/gr_pain1.wav");
	PRECACHE_SOUND("hgrunt/gr_pain2.wav");
	PRECACHE_SOUND("hgrunt/gr_pain3.wav");
	PRECACHE_SOUND("hgrunt/gr_pain4.wav");
	PRECACHE_SOUND("hgrunt/gr_pain5.wav");
	PRECACHE_SOUND("hgrunt/gr_alert1.wav");

	PRECACHE_SOUND("hgrunt/gr_idle1.wav");
	PRECACHE_SOUND("hgrunt/gr_idle2.wav");
	PRECACHE_SOUND("hgrunt/gr_idle3.wav");

	PRECACHE_SOUND("hgrunt/gr_mgun.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun2.wav");
	PRECACHE_SOUND("hgrunt/gr_mgun3.wav");

	PRECACHE_SOUND("hgrunt/gr_die1.wav");
	PRECACHE_SOUND("hgrunt/gr_die2.wav");
	PRECACHE_SOUND("hgrunt/gr_die3.wav");
}

//=========================================================
// Classify - indicates this monster's place in the 
// relationship table.
//=========================================================
int	CHSarge::Classify(void)
{
	return	CLASS_HUMAN_MILITARY;
}

//===============================
//	Function Name
//===============================

void CHSarge::Spawn(void)
{
	Precache();

	SET_MODEL(ENT(pev), "models/hsarge1.mdl");
	UTIL_SetSize(pev, Vector(-18, -18, 0), Vector(18, 18, 72));

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;
	pev->effects = 0;
	pev->health = RANDOM_FLOAT(0.f, 25.f) + 5000.f;
	pev->yaw_speed = 100;
	pev->view_ofs = VEC_VIEW;
	m_flFieldOfView = 0.2;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_cAmmoLoaded = 45;
	m_bloodColor = BLOOD_COLOR_RED;

	MonsterInit();
}

//===============================
//	Function Name
//===============================

void CHSarge::Shoot( void )
{
	if (m_hEnemy == NULL)
	{
		return;
	}

	Vector vecShootOrigin = GetGunPosition();
	Vector vecShootDir = ShootAtEnemy(vecShootOrigin);

	UTIL_MakeVectors(pev->angles);

	FireBullets(1, vecShootOrigin, vecShootDir, VECTOR_CONE_10DEGREES, 2048, BULLET_MONSTER_MP5); // shoot +-5 degrees
	pev->effects |= EF_MUZZLEFLASH;

	m_cAmmoLoaded--;// take away a bullet!
}

//===============================
//	Function Name
//===============================

void CHSarge::Kick( void )
{
	TraceResult tr;

	UTIL_MakeVectors( pev->angles );
	Vector vecStart = pev->origin;
	vecStart.z += pev->size.z * 0.5;
	Vector vecEnd = vecStart + (gpGlobals->v_forward * 70);

	// UNDONE: this one can't hit small monsters
	UTIL_TraceHull( vecStart, vecEnd, dont_ignore_monsters, human_hull, ENT(pev), &tr );
	
	if ( tr.pHit )
	{
		CBaseEntity *pEntity = CBaseEntity::Instance( tr.pHit );


	}
}

//===============================
//	FindLateralCover
//	Improved version of "FindLateralCover" that
//	makes the Grunts strafe to somewhere more reasonable.
//	Though if all else fails, we can just fall back to the
//	original CheckLateralCover code.
//===============================

#define		NUM_OF_STRAFE_CHECKS	8
#define		STRAFE_CHECK_DIST		24;

BOOL CHSarge::FindLateralCover( const Vector &vecThreat, const Vector &vecViewOffset )
{
	Vector vecSrc, vecEnd;
	int i, j, iDist;
	TraceResult tr;

	Activity actStrafe[2] = { ACT_STRAFE_RIGHT, ACT_STRAFE_LEFT };
	Vector vecStrafeDir;

	iDist = STRAFE_CHECK_DIST;
	
	UTIL_MakeVectors(pev->angles);
	vecSrc = pev->origin + vecViewOffset;

	for ( i = 0; i < NUM_OF_STRAFE_CHECKS; i++ )
	{
		vecStrafeDir = gpGlobals->v_right * ((i + 1) * iDist);
		vecEnd = vecSrc + vecStrafeDir;

		for ( j = 0; j < 2; j++ )
		{
			UTIL_TraceLine( vecThreat + vecViewOffset, vecEnd, dont_ignore_monsters, ENT(pev), &tr );
			UTIL_ParticleLine( vecThreat + vecViewOffset, tr.vecEndPos, 0.5f, 255, 255, 255 );
			if ( tr.flFraction != 1.f )
			{
				if ( FValidateCover( vecEnd ) && CheckLocalMove( pev->origin, vecEnd, NULL, NULL ) == LOCALMOVE_VALID )
				{
					if ( MoveToLocation( actStrafe[j], 0, vecEnd ) )
					{
						ALERT( at_console, "HSarge: I can strafe!\n" );
						return TRUE;
					}
				}
			}
			vecEnd = vecSrc - vecStrafeDir;
		}
	}

	return FALSE;
}

//===============================
//	MonsterThink
//	Here we can make grunts run n' gun!!!
//===============================

void CHSarge::MonsterThink( void )
{
	float flInterval = StudioFrameAdvance();

	if ( m_Activity == ACT_STRAFE_LEFT || m_Activity == ACT_STRAFE_RIGHT )
	{

	}

	CBaseMonster::MonsterThink();
}

int CHSarge::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( pevAttacker != NULL )
		FindLateralCover( pevAttacker->origin, pevAttacker->view_ofs );
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

//===============================
//	Function Name
//===============================

void CHSarge::DeathSound(void)
{

}

//===============================
//	Function Name
//===============================

void CHSarge::PainSound(void)
{
	pev->pain_finished = gpGlobals->time + 1.f;
	

}

#endif