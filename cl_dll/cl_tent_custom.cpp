
//=======================================
//	cl_tent_custom.cpp
//	Purpose: Custom temp-entities that
//	Thrill-EX uses. Parts of code were based
//	on ScriptedSnark's NetTest Decomp & Quake's
//	source code.
//
//	History:
//	JAN-19-26: Started. First custom file
//	created for ThrillEX Rewrite. Added cpart_t
//	struct for custom Qparticle parameters.
// 
//	Added Init function for future CVARS, Particle
//	thinking functions, code to draw particle 
//	sparks (both bouncy and normal), and WIP
//	smoke..
//
//=======================================

#include "hud.h"
#include "cl_util.h"
#include "r_efx.h"
#include "r_studioint.h"

#include "pm_defs.h"
#include "pmtrace.h"

extern engine_studio_api_t IEngineStudio;

//===============================
//	CustomTent_Init
//	Init function for creating CVARS and all that good
//	stuff.
//===============================

void CustomTent_Init(void)
{

}

//===============================
//	"Custom Particle" struct.
//	Used to add custom variables to particles to work around
//	hardcoded engine behaviour.
//===============================

typedef enum {
	ct_static,
	ct_grav,
	ct_slowgrav,
	ct_sparks
} ctype_t;

typedef struct cpart_s
{
	float	bounce_factor;
	float	time_mult;
	int		flags;
	vec3_t	old_org;
	vec3_t	accel;
	ctype_t type;
} cpart_t;

#define CPART_COLLIDE_STICKY	(1 << 0)
#define CPART_COLLIDE_BOUNCY	(1 << 1)
#define CPART_COLLIDE_SLIDE		(1 << 2)
#define CPART_DECAY_VELOCITY	(1 << 3)

//===============================
//	ParticleThink
//	Large particle thinking code based partly off of Quake's.
//===============================

int giSparkRamp[9] = { 0xfe, 0xfd, 0xfc, 0x6f, 0x6e, 0x6d, 0x6c, 0x67, 0x60 };

void ParticleThink(struct particle_s* p, struct cpart_s* pvars, float frametime)
{
	float time2, grav;

	grav = frametime * 800.0f * 0.05f;
	time2 = frametime * 10.0f;

	VectorCopy(p->org, pvars->old_org);
	VectorMA(p->vel, frametime, pvars->accel, p->vel);
	VectorMA(p->org, frametime, p->vel, p->org);

	if (pvars->flags & CPART_DECAY_VELOCITY)
	{
		for (int i = 0; i < 3; i++)
		{
			if (p->vel[i] < 0.0f)
				p->vel[i] += frametime * pvars->time_mult;
			if (p->vel[i] > 0.0f)
				p->vel[i] -= frametime * pvars->time_mult;
		}
	}

	if (pvars->flags & ( CPART_COLLIDE_BOUNCY | CPART_COLLIDE_SLIDE ))
	{
		pmtrace_t* tr;

		tr = gEngfuncs.PM_TraceLine(pvars->old_org, p->org, PM_TRACELINE_PHYSENTSONLY, 2, -1);

		if (tr && tr->fraction < 1.0f)
		{
			vec3_t vecNormal = tr->plane.normal;

			p->org = tr->endpos;
			p->vel = p->vel - 2 * DotProduct(p->vel, vecNormal) * vecNormal;
			p->vel = p->vel * pvars->bounce_factor;
		}
	}

	switch (pvars->type)
	{
		case ct_grav:
		{
			break;
		}

		case ct_slowgrav:
		{
			break;
		}

		case ct_sparks:
		{
			p->ramp = min(p->ramp + time2, 9.0f);
			p->color = giSparkRamp[ (int)p->ramp ];
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);
			break;
		}

		case ct_static:
		default:
		{
			break;
		}
	}
}

//===============================
//	R_BasicParticleSparks
//	Render Half-Life Alpha styled particles.
//===============================

void R_SparksCallback(struct particle_s* particle, float frametime)
{
	cpart_t pInfo;
	pInfo.type = ct_sparks;
	pInfo.accel = { 0.0f, 0.0f, -160.0f };

	ParticleThink(particle, &pInfo, frametime);
}

void R_ParticleSparks(vec3_t org)
{
	particle_t* p;
	int i, j;

	for ( i = 0; i < 15; i++ )
	{
		p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);

		if (!p)
			return;

		for ( j = 0; j < 3; j++ )
			p->org[j] = org[j];

		p->vel[2] = gEngfuncs.pfnRandomFloat(0.0f, 64.0f);
		p->ramp = 0.0;
		p->color = 254;
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

		p->type = pt_clientcustom;
		p->callback = R_SparksCallback;
		p->die = gEngfuncs.GetClientTime() + 3.0f;
	}
}

//===============================
//	R_BouncySparks
//	Render bouncy Half-Life Alpha styled particles.
//===============================

void R_BouncySparksCallback(struct particle_s* particle, float frametime)
{
	cpart_t pInfo;
	pInfo.type = ct_sparks;
	pInfo.flags = CPART_COLLIDE_BOUNCY;
	pInfo.accel = { 0.0f, 0.0f, -400.0f };
	pInfo.bounce_factor = 0.35f;

	ParticleThink(particle, &pInfo, frametime);
}

void R_BouncySparks(vec3_t org, vec3_t dir, int count, int noise, float lifetime)
{
	particle_t* p;
	int i, j;

	for (i = 0; i < count; i++)
	{
		p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);

		if (!p)
			return;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j];
			p->vel[j] = dir[j] + gEngfuncs.pfnRandomFloat((j == 2) ? 0 : -noise, noise);
		}

		p->vel[2] = gEngfuncs.pfnRandomFloat(0.0f, 64.0f);
		p->ramp = 0.0;
		p->color = 254;
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

		p->type = pt_clientcustom;
		p->callback = R_BouncySparksCallback;
		p->die = gEngfuncs.GetClientTime() + lifetime;
	}
}

//===============================
//	R_RenderSmoke
//	Render particle smoke particles that rise up to the
//	ceiling.
//===============================

void R_SmokeCallBack(struct particle_s* particle, float frametime)
{
	cpart_t pInfo;
	pInfo.type = ct_static;
	pInfo.accel = { 0.0f, 0.0f, -20.0f };
	pInfo.time_mult = 4.0f;
	pInfo.flags = CPART_DECAY_VELOCITY;

	ParticleThink(particle, &pInfo, frametime);
}

void R_RenderSmoke(vec3_t org)
{
	particle_t* smoke;
	int i, j;
	float scale = 32.0f;

	for ( i = 0; i < 64; i++ )
	{
		smoke = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);

		if (!smoke)
			return;

		for (j = 0; j < 3; j++)
		{
			smoke->org[j] = org[j];
			smoke->org[2] = org[j] + (i * 0.5f);
		}

		smoke->vel[0] = gEngfuncs.pfnRandomFloat(-16.0f, 16.0f);
		smoke->vel[1] = gEngfuncs.pfnRandomFloat(-16.0f, 16.0f);
		smoke->vel[2] = 32.0f;

		smoke->color = 12;
		smoke->type = pt_clientcustom;
		smoke->callback = R_SmokeCallBack;
		smoke->die = gEngfuncs.GetClientTime() + 5.0f;
	}
}

//===============================
//	R_BloodStream
//	WIP
//===============================

#if 0

void R_BloodStream(vec_t* org, vec_t* dir, int pcolor, int speed)
{
	// Add our particles
	vec3_t	dirCopy;
	float	arc;
	int		count;
	int		count2;
	particle_t* p;
	float	num;
	int		speedCopy = speed;

	VectorNormalize(dir);

	arc = 0.05;
	for (count = 0; count < 100; count++)
	{
		p->die = cl.time + 2.0;
		p->color = pcolor + RandomLong(0, 9);
		p->packedColor = hlRGB(host_basepal, p->color);

		p->type = pt_vox_grav;

		VectorCopy(org, p->org);
		VectorCopy(dir, dirCopy);
		dirCopy[2] -= arc;

		arc -= 0.005;

		VectorScale(dirCopy, speedCopy, p->vel);
		speedCopy -= 0.00001; // make last few drip
	}

	arc = 0.075;
	for (count = 0; count < (speed / 5); count++)
	{
		p->die = cl.time + 3.0;
		p->type = pt_vox_slowgrav;
		p->color = pcolor + RandomLong(0, 9);
		p->packedColor = hlRGB(host_basepal, p->color);

		VectorCopy(org, p->org);
		VectorCopy(dir, dirCopy);
		dirCopy[2] -= arc;

		arc -= 0.005;

		num = RandomFloat(0, 1);
		speedCopy = speed * num;
		num *= 1.7;

		VectorScale(dirCopy, num, dirCopy); // randomize a bit
		VectorScale(dirCopy, speedCopy, p->vel);

		for (count2 = 0; count2 < 2; count2++)
		{
			p->die = cl.time + 3.0;
			p->type = pt_vox_slowgrav;
			p->color = pcolor + RandomLong(0, 9);
			p->packedColor = hlRGB(host_basepal, p->color);

			p->org[0] = org[0] + RandomFloat(-1, 1);
			p->org[1] = org[1] + RandomFloat(-1, 1);
			p->org[2] = org[2] + RandomFloat(-1, 1);

			VectorCopy(dir, dirCopy);
			dirCopy[2] -= arc;

			VectorScale(dirCopy, num, dirCopy); // randomize a bit
			VectorScale(dirCopy, speedCopy, p->vel);
		}
	}
}

//===============================
//	R_Blood
//	WIP
//===============================

void R_Blood(vec_t* org, vec_t* dir, int pcolor, int speed)
{
	vec3_t	dirCopy;
	vec3_t	orgCopy;
	float	arc;
	int		count;
	int		count2;
	particle_t* p;
	int		pspeed;

	VectorNormalize(dir);

	pspeed = speed * 3;

	arc = 0.06;
	for (count = 0; count < (speed / 2); count++)
	{
		orgCopy[0] = org[0] + RandomFloat(-3, 3);
		orgCopy[1] = org[1] + RandomFloat(-3, 3);
		orgCopy[2] = org[2] + RandomFloat(-3, 3);

		dirCopy[0] = dir[0] + RandomFloat(-arc, arc);
		dirCopy[1] = dir[1] + RandomFloat(-arc, arc);
		dirCopy[2] = dir[2] + RandomFloat(-arc, arc);

		for (count2 = 0; count2 < 8; count2++)
		{
			p->die = cl.time + 1.5;
			p->color = pcolor + RandomLong(0, 9);
			p->type = pt_vox_grav;
			p->packedColor = hlRGB(host_basepal, p->color);

			p->org[0] = orgCopy[0] + RandomFloat(-1, 1);
			p->org[1] = orgCopy[1] + RandomFloat(-1, 1);
			p->org[2] = orgCopy[2] + RandomFloat(-1, 1);

			VectorScale(dirCopy, pspeed, p->vel);
		}

		pspeed -= speed;
	}
}

#endif