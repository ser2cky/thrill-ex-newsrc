
//=======================================
//	cl_tent_custom.cpp
// 
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
//	JAN-20-26: Improved smoke, added blood & bloodstream,
//	cl_explosion_style CVar, custom temp-ent usermsg
//
//=======================================

#include "hud.h"
#include "cl_util.h"
#include "r_efx.h"
#include "r_studioint.h"
#include "parsemsg.h"

#include "pm_defs.h"
#include "pmtrace.h"

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

extern engine_studio_api_t IEngineStudio;
int __MsgFunc_TempEntity(const char* pszName, int iSize, void* pbuf);

//===============================
//	CustomTent_Init
//	Init function for creating CVARS and all that good
//	stuff.
//===============================

cvar_t* cl_explosion_style;

void CustomTent_Init(void)
{
	HOOK_MESSAGE(TempEntity);

	cl_explosion_style = CVAR_CREATE( "cl_explosion_style", "1", FCVAR_ARCHIVE );
	// 0 = Sprite only, 1 = Sprite & Shrapnel, 2 = Shrapnel only
}

//===============================
//	"Custom Particle" struct.
//	Used to add custom variables to particles to work around
//	hardcoded engine behaviour.
//===============================

typedef enum {
	ct_static,
	ct_sparks,
	ct_smoke
} ctype_t;

typedef struct cpart_s
{
	float	bounce_factor;
	float	time_mult;
	float	ramp_mult;
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
int giSmokeRamp[9] = { 0x8, 0x7, 0x6, 0x5, 0x4, 0x3, 0x2, 0x1, 0x0 };

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

	if ( pvars->flags & ( CPART_COLLIDE_BOUNCY | CPART_COLLIDE_SLIDE | CPART_COLLIDE_STICKY ) )
	{
		pmtrace_t* tr;

		tr = gEngfuncs.PM_TraceLine(pvars->old_org, p->org, PM_TRACELINE_PHYSENTSONLY, 2, -1);

		if (tr && tr->fraction < 1.0f)
		{
			vec3_t vecNormal = tr->plane.normal;

			if (pvars->flags & CPART_COLLIDE_STICKY)
			{
				p->org = tr->endpos;
				if (gEngfuncs.PM_PointContents(p->org, NULL) != CONTENT_EMPTY)
				{
					p->vel = vec3_origin;
				}
				else
				{
					p->vel[0] = 0.0f;
					p->vel[1] = 0.0f;
				}
			}
			else
			{
				p->org = tr->endpos;
				p->vel = p->vel - 2 * DotProduct(p->vel, vecNormal) * vecNormal;
				p->vel = p->vel * pvars->bounce_factor;
			}
		}
	}

	switch (pvars->type)
	{
		case ct_sparks:
		{
			p->ramp = min(p->ramp + time2, 8.0f);
			p->color = giSparkRamp[ (int)p->ramp ];
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);
			break;
		}

		case ct_smoke:
		{
			p->ramp = min(p->ramp + (frametime * pvars->ramp_mult), 8.0f);
			p->color = giSmokeRamp[(int)p->ramp];
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
		p->ramp = 0.0 + gEngfuncs.pfnRandomLong(0, 2);
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
	pInfo.type = ct_smoke;
	pInfo.accel = { 0.0f, 0.0f, -10.0f };
	pInfo.time_mult = 8.0f;
	pInfo.flags = CPART_DECAY_VELOCITY;
	pInfo.ramp_mult = 5.0f;

	ParticleThink(particle, &pInfo, frametime);
}

void R_RenderSmoke(vec3_t org, float scale, float vert_scale, int count)
{
	particle_t* smoke;
	int i, j;

	float intensity = 0.0f;
	int color_add = -1;
	int color_rand = 16;

	for ( i = 0; i < count; i++ )
	{
		smoke = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);

		if (!smoke)
			return;

		for (j = 0; j < 3; j++)
			smoke->org[j] = org[j];
		intensity += (1.0f / (float)count );

		smoke->vel[0] = gEngfuncs.pfnRandomFloat(-scale, scale) * min(intensity * 2, 1.0f);
		smoke->vel[1] = gEngfuncs.pfnRandomFloat(-scale, scale) * min(intensity * 2, 1.0f);
		smoke->vel[2] = vert_scale * intensity;

		if (gEngfuncs.pfnRandomLong(0, 1) == 1)
			color_rand = 24;

		if ((i % color_rand) == 0)
			color_add++;

		smoke->ramp = color_add;
		smoke->color = giSmokeRamp[color_add];
		gEngfuncs.pEfxAPI->R_GetPackedColor(&smoke->packedColor, smoke->color);

		smoke->type = pt_clientcustom;
		smoke->callback = R_SmokeCallBack;
		smoke->die = gEngfuncs.GetClientTime() + 1.0f + ( i * (1.0f/ count) );
	}
}

//===============================
//	R_BloodStream
//	HL-Alpha R_BloodStream particle effect with a 25% chance
//	to collide with the world for that PIZAZ!!!
//===============================

void R_StickyBloodCallback(struct particle_s* particle, float frametime)
{
	cpart_t pInfo;
	pInfo.accel = { 0.0f, 0.0f, -40.0f };
	pInfo.flags = CPART_COLLIDE_BOUNCY;
	pInfo.bounce_factor = 0.5f;

	ParticleThink(particle, &pInfo, frametime);
}

void R_NormalBloodCallback(struct particle_s* particle, float frametime)
{
	cpart_t pInfo;
	pInfo.accel = { 0.0f, 0.0f, -40.0f };
	pInfo.flags = 0;
	pInfo.bounce_factor = 0.5f;

	ParticleThink(particle, &pInfo, frametime);
}

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
		p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);

		if (!p)
			return;

		p->die = gEngfuncs.GetClientTime() + 2.0;
		p->color = pcolor + gEngfuncs.pfnRandomFloat(0, 9);
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

		p->type = pt_clientcustom;
		if (gEngfuncs.pfnRandomLong(0, 4) <= 1)
			p->callback = R_StickyBloodCallback;
		else
			p->callback = R_NormalBloodCallback;

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
		p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);

		if (!p)
			return;

		p->die = gEngfuncs.GetClientTime() + 3.0;
		p->color = pcolor + gEngfuncs.pfnRandomFloat(0, 9);
		gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

		p->type = pt_clientcustom;
		if (gEngfuncs.pfnRandomLong(0, 4) <= 1)
			p->callback = R_StickyBloodCallback;
		else
			p->callback = R_NormalBloodCallback;

		VectorCopy(org, p->org);
		VectorCopy(dir, dirCopy);
		dirCopy[2] -= arc;

		arc -= 0.005;

		num = gEngfuncs.pfnRandomFloat(0, 1);
		speedCopy = speed * num;
		num *= 1.7;

		VectorScale(dirCopy, num, dirCopy); // randomize a bit
		VectorScale(dirCopy, speedCopy, p->vel);

		for (count2 = 0; count2 < 2; count2++)
		{
			p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);

			if (!p)
				return;

			p->die = gEngfuncs.GetClientTime() + 3.0;
			p->color = pcolor + gEngfuncs.pfnRandomFloat(0, 9);
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

			p->type = pt_clientcustom;
			if (gEngfuncs.pfnRandomLong(0, 4) <= 1)
				p->callback = R_StickyBloodCallback;
			else
				p->callback = R_NormalBloodCallback;

			p->org[0] = org[0] + gEngfuncs.pfnRandomFloat(-1, 1);
			p->org[1] = org[1] + gEngfuncs.pfnRandomFloat(-1, 1);
			p->org[2] = org[2] + gEngfuncs.pfnRandomFloat(-1, 1);

			VectorCopy(dir, dirCopy);
			dirCopy[2] -= arc;

			VectorScale(dirCopy, num, dirCopy); // randomize a bit
			VectorScale(dirCopy, speedCopy, p->vel);
		}
	}
}

//===============================
//	R_Blood
//	HL-Alpha R_Blood particle effect with a 25% chance
//	to collide with the world for that PIZAZ!!!
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
		orgCopy[0] = org[0] + gEngfuncs.pfnRandomFloat(-3, 3);
		orgCopy[1] = org[1] + gEngfuncs.pfnRandomFloat(-3, 3);
		orgCopy[2] = org[2] + gEngfuncs.pfnRandomFloat(-3, 3);

		dirCopy[0] = dir[0] + gEngfuncs.pfnRandomFloat(-arc, arc);
		dirCopy[1] = dir[1] + gEngfuncs.pfnRandomFloat(-arc, arc);
		dirCopy[2] = dir[2] + gEngfuncs.pfnRandomFloat(-arc, arc);

		for (count2 = 0; count2 < 8; count2++)
		{
			p = gEngfuncs.pEfxAPI->R_AllocParticle(NULL);

			if (!p)
				return;

			p->type = pt_clientcustom;
			if (gEngfuncs.pfnRandomLong(0, 4) <= 1)
				p->callback = R_StickyBloodCallback;
			else
				p->callback = R_NormalBloodCallback;

			p->die = gEngfuncs.GetClientTime() + 1.5;
			p->color = pcolor + gEngfuncs.pfnRandomFloat(0, 9);
			gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);

			p->org[0] = orgCopy[0] + gEngfuncs.pfnRandomFloat(-1, 1);
			p->org[1] = orgCopy[1] + gEngfuncs.pfnRandomFloat(-1, 1);
			p->org[2] = orgCopy[2] + gEngfuncs.pfnRandomFloat(-1, 1);

			VectorScale(dirCopy, pspeed, p->vel);
		}

		pspeed -= speed;
	}
}

//===============================
//	R_Blood
//	Half-Life Alpha styled explosion effect. Can be
//	controlled using the "cl_explosion_style" CVar.
//===============================

int __MsgFunc_TempEntity(const char* pszName, int iSize, void* pbuf)
{
	vec3_t org, dir;
	int type, count, speed, color;
	int width, height;
	int ltime, noise;

	BEGIN_READ(pbuf, iSize);

	type = READ_BYTE();

	switch (type)
	{
		case TEX_TE_EXPLOSION:
		{
			org[0] = READ_COORD();
			org[1] = READ_COORD();
			org[2] = READ_COORD();
			break;
		}

		case TEX_TE_SPARKS:
		{
			org[0] = READ_COORD();
			org[1] = READ_COORD();
			org[2] = READ_COORD();
			R_ParticleSparks(org);
			break;
		}

		case TEX_BOUNCE_SPARKS:
		{
			org[0] = READ_COORD();
			org[1] = READ_COORD();
			org[2] = READ_COORD();

			dir[0] = READ_COORD();
			dir[1] = READ_COORD();
			dir[2] = READ_COORD();

			count = READ_BYTE();
			noise = READ_BYTE();
			ltime = READ_BYTE();
			R_BouncySparks(org, dir, count, noise, ltime);
			break;
		}

		case TEX_SMOKE:
		{
			org[0] = READ_COORD();
			org[1] = READ_COORD();
			org[2] = READ_COORD();

			width = READ_BYTE();
			height = READ_BYTE();
			count = READ_BYTE();
			R_RenderSmoke(org, width, height, count);
			break;
		}

		case TEX_BLOOD:
		{
			break;
		}

		default:
		{
			gEngfuncs.Con_DPrintf("ThrillEX: I.D of %d is invalid\n", type);
			break;
		}
	}

	return 1;
}