
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
//	JAN-23-26: Added InitParticleInfo function so
//	creating new particle info. with zero'd out values
//	is less time-consuming.
// 
//	JAN-24-26: Added CL_RenderEntityFX to deal with
//	custom entity effects since we're giving the
//	effects field a few more bits to work with...
//	Also re-implemented cl_flashlight_style, and
//	S.W mode particle tracers from the Alpha...
// 
//	JAN-25-26: ...Did some funny OpenGL stuff with
//	gl_flashblend. added muzzleflash F.X too..
// 
//=======================================

#include "hud.h"
#include "cl_util.h"
#include "r_efx.h"
#include "r_studioint.h"
#include "parsemsg.h"

#include "pm_defs.h"
#include "pmtrace.h"
#include "event_api.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <gl/GL.h>
#pragma comment(lib, "opengl32.lib")

extern engine_studio_api_t IEngineStudio;
int __MsgFunc_TempEntity(const char* pszName, int iSize, void* pbuf);

#ifndef M_PI
#define M_PI		3.14159265358979323846	// matches value in gcc v2 math.h
#endif

#define				MAX_DLIGHTS		32
extern vec3_t		v_origin, v_angles;
extern float		v_blend[4];
extern cvar_t		*gl_flashblend;

dlight_t			cl_dlights[MAX_DLIGHTS];
/*
=============================================================================

DYNAMIC LIGHTS BLEND RENDERING

=============================================================================
*/

void AddLightBlend( float r, float g, float b, float a2 )
{
	float	a;

	v_blend[3] = a = v_blend[3] + a2 * (1 - v_blend[3]);

	a2 = a2 / a;

	v_blend[0] = v_blend[1] * (1 - a2) + r * a2;
	v_blend[1] = v_blend[1] * (1 - a2) + g * a2;
	v_blend[2] = v_blend[2] * (1 - a2) + b * a2;
}

//===============================
//	CL_AllocDlight
//	Override CL_AllocDlight if we got gl_flashblend
//	1 activated...
//===============================

dlight_t* CL_AllocDlight( int key )
{
	int	i;
	dlight_t* dl;

	if (!gl_flashblend->value || !IEngineStudio.IsHardware())
		return gEngfuncs.pEfxAPI->CL_AllocDlight(key);

	// first look for an exact key match
	if (key)
	{
		dl = cl_dlights;
		for (i = 0; i < MAX_DLIGHTS; i++, dl++)
		{
			if (dl->key == key)
			{
				memset(dl, 0, sizeof(*dl));
				dl->key = key;
				return dl;
			}
		}
	}

	// then look for anything else
	dl = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, dl++)
	{
		if (dl->die < gEngfuncs.GetClientTime())
		{
			memset(dl, 0, sizeof(*dl));
			dl->key = key;
			return dl;
		}
	}

	dl = &cl_dlights[0];
	memset(dl, 0, sizeof(*dl));
	dl->key = key;
	return dl;
}

//===============================
//	R_RenderDlight
//	For gl_flashblend stuff...
//===============================

void R_RenderDlight(dlight_t* light)
{
	vec3_t	vpn, vright, vup, v;
	float	rad, a;
	int		i, j;

	rad = light->radius * 0.35;

	AngleVectors(v_angles, vpn, vright, vup);
	VectorSubtract(light->origin, v_origin, v);

	if (Length(v) < rad)
	{	// view is inside the dlight
		AddLightBlend(1, 0.5, 0, light->radius * 0.0003);
		//gEngfuncs.Con_Printf("cl_tent_custom: r:%.2f g:%.2f b:%.2f a:%.2f\n", v_blend[0], v_blend[1], v_blend[2], v_blend[3]);
		return;
	}


	glBegin(GL_TRIANGLE_FAN);
	glColor3f(0.2f, 0.1f, 0.0f);
	for (i = 0; i < 3; i++)
		v[i] = light->origin[i] - vpn[i] * rad;
	glVertex3fv(v);
	glColor3f(0.0f, 0.0f, 0.0f);
	for (i = 16; i >= 0; i--)
	{
		a = i / 16.0 * M_PI * 2;
		for (j = 0; j < 3; j++)
			v[j] = light->origin[j] + vright[j] * cos(a) * rad
			+ vup[j] * sin(a) * rad;
		glVertex3fv(v);
	}
	glEnd();

	// SERECKY JAN-27-26: triapi is a piece of shit and cannot
	// keep the flashblend color consistent. Triapi in general
	// is a piece of shit like oh my god like its good that 
	// it exists but it can do fuck all nothing

#if 0
	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);
	gEngfuncs.pTriAPI->Begin(TRI_TRIANGLE_FAN);
	gEngfuncs.pTriAPI->Color4f(0.2f, 0.1f, 0.0f, 1.0f);
	for (i = 0; i < 3; i++)
		v[i] = light->origin[i] - vpn[i] * rad;
	gEngfuncs.pTriAPI->Vertex3fv(v);
	gEngfuncs.pTriAPI->Color4f(0.0f, 0.0f, 0.0f, 0.0f);
	for (i = 16; i >= 0; i--)
	{
		a = i / 16.0 * M_PI * 2;
		for (j = 0; j < 3; j++)
			v[j] = light->origin[j] + vright[j] * cos(a) * rad
			+ vup[j] * sin(a) * rad;
		gEngfuncs.pTriAPI->Vertex3fv(v);
	}
	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
#endif
}

/*
=============
R_RenderDlights
=============
*/
void R_RenderDlights( void )
{
	int		i;
	dlight_t* l;

	if (!gl_flashblend->value || !IEngineStudio.IsHardware())
		return;


	glDepthMask(GL_FALSE);
	glDisable(GL_TEXTURE_2D);
	glShadeModel(GL_SMOOTH);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	l = cl_dlights;
	for (i = 0; i < MAX_DLIGHTS; i++, l++)
	{
		if (l->die < gEngfuncs.GetClientTime() || !l->radius)
			continue;

		R_RenderDlight(l);

		l->radius -= gHUD.m_flTimeDelta * l->decay;
		if (l->radius < 0)
			l->radius = 0;
		//gEngfuncs.Con_Printf("Rendering dlight\n");
	}

	glColor3f(1, 1, 1);
	glDisable(GL_BLEND);
	glEnable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDepthMask(GL_TRUE);
}

//===============================
//	CustomTent_Init
//	Init function for creating CVARS and all that good
//	stuff.
//===============================

cvar_t	*cl_explosion_style;
cvar_t	*cl_flashlight_style;
cvar_t	*tracer_style;
cvar_t	*cl_muzzleflash_style;
cvar_t	*tracerSpeed = NULL;
cvar_t	*tracerOffset = NULL;

void CustomTent_Init(void)
{
	HOOK_MESSAGE(TempEntity);

	// 0 = Sprite, 1 = Shrapnel, 2 = Sprite & Shrapnel
	cl_explosion_style		= CVAR_CREATE( "cl_explosion_style", "2", FCVAR_ARCHIVE );
	cl_flashlight_style		= CVAR_CREATE( "cl_flashlight_style", "0", FCVAR_ARCHIVE );
	cl_muzzleflash_style	= CVAR_CREATE( "cl_muzzleflash_style", "0", FCVAR_ARCHIVE );
	tracer_style			= CVAR_CREATE( "tracer_style", "0", FCVAR_ARCHIVE );
	
	// SERECKY JAN-25-26: Evil hack to override default tracer properties.
	gEngfuncs.Cvar_SetValue( "tracerspeed", 3000 );
	gEngfuncs.Cvar_SetValue( "traceroffset", 35 );
	gEngfuncs.Cvar_SetValue( "tracerlength", 1.0 );
	gEngfuncs.Cvar_SetValue( "tracerred", 0.6 );
	gEngfuncs.Cvar_SetValue( "tracergreen", 0.8 );
	gEngfuncs.Cvar_SetValue( "tracerblue", 0.1 );
	gEngfuncs.Cvar_SetValue( "traceralpha", 0.2 );

	gEngfuncs.Cvar_SetValue( "texgamma", 1.8 );
	gEngfuncs.Cvar_SetValue( "lambert", 1.7 );

	tracerSpeed = gEngfuncs.pfnGetCvarPointer("tracerspeed");
	tracerOffset = gEngfuncs.pfnGetCvarPointer("traceroffset");
}

//===============================
//	CustomTent_Reset
//===============================

void CustomTent_Reset(void)
{
	memset(cl_dlights, 0, sizeof(cl_dlights));
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
	vec3_t	accel;
	ctype_t type;

	vec3_t	old_org; // don't touch
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
//	InitParticleInfo
//	reset particle info back to defaults
//===============================

void InitParticleInfo(cpart_t* p)
{
	p->bounce_factor = 0.0f;
	p->time_mult = 0.0f;
	p->ramp_mult = 0.0f;
	p->flags = 0;
	p->type = ct_static;
}

//===============================
//	R_BasicParticleSparks
//	Render Half-Life Alpha styled particles.
//===============================

void R_SparksCallback(struct particle_s* particle, float frametime)
{
	cpart_t pInfo;
	InitParticleInfo(&pInfo);

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
		p = gEngfuncs.pEfxAPI->R_AllocParticle(R_SparksCallback);

		if (!p)
			return;

		for ( j = 0; j < 3; j++ )
		{
			p->org[j] = org[j];
			p->vel[j] = gEngfuncs.pfnRandomFloat(-16.0, 15.0);
		}

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
	InitParticleInfo(&pInfo);

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
	InitParticleInfo(&pInfo);

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
		smoke->vel[2] = (vert_scale * intensity) + 30.0f;

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
	InitParticleInfo(&pInfo);

	pInfo.accel = { 0.0f, 0.0f, -160.0f };
	pInfo.flags = CPART_COLLIDE_BOUNCY;
	pInfo.bounce_factor = 0.5f;

	ParticleThink(particle, &pInfo, frametime);
}

void R_NormalBloodCallback(struct particle_s* particle, float frametime)
{
	cpart_t pInfo;
	InitParticleInfo(&pInfo);

	pInfo.accel = { 0.0f, 0.0f, -160.0f };
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

		p->type = pt_vox_slowgrav;
		//p->type = pt_clientcustom;
		//if (gEngfuncs.pfnRandomLong(0, 24) == 0)
		//	p->callback = R_StickyBloodCallback;
		//else
		//	p->callback = R_NormalBloodCallback;

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

		p->type = pt_vox_slowgrav;
		//p->type = pt_clientcustom;
		//if (gEngfuncs.pfnRandomLong(0, 24) == 0)
		//	p->callback = R_StickyBloodCallback;
		//else
		//	p->callback = R_NormalBloodCallback;

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

			p->type = pt_vox_slowgrav;
			//p->type = pt_clientcustom;
			//if (gEngfuncs.pfnRandomLong(0, 24) == 0)
			//	p->callback = R_StickyBloodCallback;
			//else
			//	p->callback = R_NormalBloodCallback;

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

void R_StickyBlood1Callback(struct particle_s* particle, float frametime)
{
	cpart_t pInfo;
	InitParticleInfo(&pInfo);

	pInfo.accel = { 0.0f, 0.0f, -320.0f };
	pInfo.flags = CPART_COLLIDE_BOUNCY;
	pInfo.bounce_factor = 0.5f;

	ParticleThink(particle, &pInfo, frametime);
}

void R_NormalBlood1Callback(struct particle_s* particle, float frametime)
{
	cpart_t pInfo;
	InitParticleInfo(&pInfo);

	pInfo.accel = { 0.0f, 0.0f, -320.0f };
	pInfo.flags = 0;
	pInfo.bounce_factor = 0.5f;

	ParticleThink(particle, &pInfo, frametime);
}

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

			p->type = pt_vox_grav;
			//p->type = pt_clientcustom;
			//if (gEngfuncs.pfnRandomLong(0, 24) == 0)
			//	p->callback = R_StickyBlood1Callback;
			//else
			//	p->callback = R_NormalBlood1Callback;

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
//	R_Explosion
//	Half-Life Alpha styled explosion effect made simple. 
//	Can be controlled using the "cl_explosion_style" CVar.
//===============================

void R_Explosion(vec3_t org, int scale, int framerate, int speed, int sprite, int model, int flags)
{
	if (cl_explosion_style->value == 0 || cl_explosion_style->value == 2)
	{
		TEMPENTITY* explosion = gEngfuncs.pEfxAPI->R_DefaultSprite(org, sprite, framerate);
		gEngfuncs.pEfxAPI->R_Sprite_Explode(explosion, scale, 0);
	}

	if (cl_explosion_style->value == 1 || cl_explosion_style->value == 2)
		gEngfuncs.pEfxAPI->R_TempSphereModel(org, 400.0f, 1.5f, 30, model);

	dlight_t* dl = CL_AllocDlight(0);
	VectorCopy(org, dl->origin);

	// SERECKY JAN-24-26: Thanks SanyaSho for  the exact
	// alpha explosion DLIGHT properties!!!

	dl->radius = 250.0f;
	dl->color.r = 250; // -6
	dl->color.g = 250; // -6
	dl->color.b = 150; // -106
	dl->die = gEngfuncs.GetClientTime() + 0.01f;
	dl->decay = 800.0f;

	gEngfuncs.pEventAPI->EV_PlaySound(-1, org, CHAN_AUTO, va("weapons/explode%d.wav", gEngfuncs.pfnRandomLong(3, 5)), 1.0f, 0.3f, 0, PITCH_NORM);
}

//===============================
//	R_TracerEffect
//	Tracer effect that can be controlled w/ CVars
//===============================

void R_TracerEffect(vec3_t start, vec3_t end)
{
	particle_t* p;
	vec3_t	temp, vel;
	float	len;

	// Return if we're using regular tracers.
	if (!tracer_style->value)
	{
		gEngfuncs.pEfxAPI->R_TracerEffect(start, end);
		return;
	}

	if (tracerSpeed->value <= 0)
		tracerSpeed->value = 3;

	VectorSubtract(end, start, temp);
	len = Length(temp);

	VectorScale(temp, 1.0 / len, temp);
	VectorScale(temp, gEngfuncs.pfnRandomLong(-10, 9) + tracerOffset->value, vel);
	VectorAdd(start, vel, start);

	// SERECKY JAN-15-26: particle tracers seem to go at 1/4th the
	// speed for some weird reason...
	VectorScale(temp, tracerSpeed->value * 0.25f, vel);

	p = gEngfuncs.pEfxAPI->R_AllocParticle( NULL );

	if (!p)
		return;

	VectorCopy( start, p->org );
	VectorCopy( vel, p->vel );

	p->type = pt_static;
	p->color = 109;
	gEngfuncs.pEfxAPI->R_GetPackedColor(&p->packedColor, p->color);
	p->die = gEngfuncs.GetClientTime() + (len / (tracerSpeed->value * 0.25f));
}

//===============================
//	__MsgFunc_TempEntity
//	custom tempentity message
//===============================

int __MsgFunc_TempEntity(const char* pszName, int iSize, void* pbuf)
{
	vec3_t org, dir;
	int type, count, speed, color;
	int width, height, scale;
	int ltime, noise, flags;
	int modelindex1, modelindex2;
	int framerate;

	BEGIN_READ(pbuf, iSize);

	type = READ_BYTE();

	switch (type)
	{
		case THRILLEX_EXPLOSION:
		{
			org[0] = READ_COORD();
			org[1] = READ_COORD();
			org[2] = READ_COORD();
			scale = READ_BYTE() * 0.1f;
			speed = READ_BYTE();
			framerate = READ_BYTE();
			modelindex1 = READ_SHORT();
			modelindex2 = READ_SHORT();
			flags = READ_BYTE();

			R_Explosion(org, scale, framerate, speed, modelindex1, modelindex2, flags);
			break;
		}

		case THRILLEX_SPARKS:
		{
			org[0] = READ_COORD();
			org[1] = READ_COORD();
			org[2] = READ_COORD();

			R_ParticleSparks(org);
			break;
		}

		case THRILLEX_BOUNCE_SPARKS:
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

		case THRILLEX_SMOKE:
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

		case THRILLEX_BLOOD:
		{
			org[0] = READ_COORD();
			org[1] = READ_COORD();
			org[2] = READ_COORD();
			dir[0] = READ_COORD();
			dir[1] = READ_COORD();
			dir[2] = READ_COORD();
			color = READ_BYTE();
			speed = READ_BYTE();

			R_Blood(org, dir, color, speed);
			break;
		}

		case THRILLEX_BLOODSTREAM:
		{
			org[0] = READ_COORD();
			org[1] = READ_COORD();
			org[2] = READ_COORD();
			dir[0] = READ_COORD();
			dir[1] = READ_COORD();
			dir[2] = READ_COORD();
			color = READ_BYTE();
			speed = READ_BYTE();

			R_BloodStream(org, dir, color, speed);
			break;
		}

		case THRILLEX_TRACER:
		{
			org[0] = READ_COORD();
			org[1] = READ_COORD();
			org[2] = READ_COORD();
			dir[0] = READ_COORD();
			dir[1] = READ_COORD();
			dir[2] = READ_COORD();

			R_TracerEffect(org, dir);
			break;
		}

		case THRILLEX_PARTICLE_LINE:
		{
			org[0] = READ_COORD();
			org[1] = READ_COORD();
			org[2] = READ_COORD();
			dir[0] = READ_COORD();
			dir[1] = READ_COORD();
			dir[2] = READ_COORD();

			gEngfuncs.pEfxAPI->R_ParticleLine( org, dir, 255, 255, 255, 0.5f );
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

//===============================
//	CL_RenderEntityFX
//	Handle THRILLEX's newly added pev->effects!!!
//===============================

#define FLASHLIGHT_DISTANCE		2000

void CL_RenderEntityFX(cl_entity_t* ent)
{
	vec3_t		fv, rv, uv;
	dlight_t *dl, *dl_1;

	switch (ent->curstate.effects)
	{
		case EF_MUZZLEFLASH:
		{
			if ( cl_muzzleflash_style->value == 0 )
				break;

			dl = CL_AllocDlight(0);
			VectorCopy(ent->origin, dl->origin);
			dl->origin[2] += 16;
			AngleVectors(ent->angles, fv, rv, uv);

			VectorMA(dl->origin, 18, fv, dl->origin);
			dl->color.r = dl->color.g = dl->color.b = 255;
			dl->radius = 200 + (rand() & 31);
			dl->minlight = 32;
			dl->die = gEngfuncs.GetClientTime() + 0.1;
			break;
		}
		case EF_FLASHLIGHT:
		{
			if ( ( cl_flashlight_style->value == 0 ) || ( cl_flashlight_style->value == 2 ) )
			{
				dl_1 = CL_AllocDlight(2);

				pmtrace_t tr;
				physent_t *pe;
				float falloff;

				vec3_t view_ofs, end;
				vec3_t angles, forward;

				// Basic setup for all the start & end stuff...
				gEngfuncs.GetViewAngles( (float *)angles );
				AngleVectors ( angles, forward, NULL, NULL );

				gEngfuncs.pEventAPI->EV_LocalPlayerViewheight( view_ofs );

				VectorCopy(ent->origin, dl_1->origin);
				VectorAdd(dl_1->origin, view_ofs, dl_1->origin);

				VectorMA(dl_1->origin, FLASHLIGHT_DISTANCE, forward, end);

				// Boring traceline stuff...
				gEngfuncs.pEventAPI->EV_SetUpPlayerPrediction( false, true );

				gEngfuncs.pEventAPI->EV_PushPMStates();

				gEngfuncs.pEventAPI->EV_SetSolidPlayers ( ent->index - 1 );	

				gEngfuncs.pEventAPI->EV_SetTraceHull( 2 );

				gEngfuncs.pEventAPI->EV_PlayerTrace( dl_1->origin, end, PM_STUDIO_BOX, -1, &tr );

				if (tr.ent > 0)
				{
					pe = gEngfuncs.pEventAPI->EV_GetPhysent( tr.ent );

					if (pe->studiomodel != NULL)
						VectorCopy(pe->origin, tr.endpos);
				}

				VectorCopy(tr.endpos, dl_1->origin);
				falloff = tr.fraction * FLASHLIGHT_DISTANCE;

				if (falloff < 500)
					falloff = 1.0;
				else
					falloff = 500.0 / (falloff);
			
				falloff *= falloff;

				dl_1->radius = 80;
				dl_1->color.r = dl_1->color.g = dl_1->color.b = 255 * falloff;
				dl_1->die = gEngfuncs.GetClientTime() + 0.2f;
				dl_1->decay = 2048;

				gEngfuncs.pEventAPI->EV_PopPMStates();
			}

			if ( cl_flashlight_style->value >= 1 )
			{
				dl = CL_AllocDlight(1);
				VectorCopy(ent->origin, dl->origin);
				dl->radius = 200 + (rand() & 31);
				dl->color.r = dl->color.g = dl->color.b = 255;
				dl->die = gEngfuncs.GetClientTime() + 0.001;
			}
			break;
		}
	}
}