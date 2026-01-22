//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"

// ThrillEX Addition/Edit Start
#include "r_studioint.h"
#include "com_model.h"

extern engine_studio_api_t IEngineStudio;
// ThrillEX Addition/Edit End

#define DLLEXPORT __declspec( dllexport )

extern "C"
{
	void DLLEXPORT HUD_DrawNormalTriangles( void );
	void DLLEXPORT HUD_DrawTransparentTriangles( void );
};

// ThrillEX Addition/Edit Start

model_s* TRI_pModel;

/*
================
R_GetSpriteFrame

assume pModel is valid
================
*/

mspriteframe_t *R_GetSpriteFrame( model_t *pModel, int frame, float yaw )
{
	msprite_t		*psprite;
	mspritegroup_t	*pspritegroup;
	mspriteframe_t	*pspriteframe = NULL;
	float		*pintervals, fullinterval;
	int		i, numframes;
	float		targettime;

	psprite = (msprite_t*)pModel->cache.data;

	if ( !psprite )
	{
		gEngfuncs.Con_Printf("Sprite:  no pSprite!!!\n");
		return 0;
	}

	if( frame < 0 )
	{
		frame = 0;
	}
	else if( frame >= psprite->numframes )
	{
		if( frame > psprite->numframes )
			gEngfuncs.Con_Printf( "R_GetSpriteFrame: no such frame %d (%s)\n", frame, pModel->name );
		frame = psprite->numframes - 1;
	}

	if( psprite->frames[frame].type == SPR_SINGLE )
	{
		pspriteframe = psprite->frames[frame].frameptr;
	}
	else if( psprite->frames[frame].type == SPR_GROUP ) 
	{
		pspritegroup = (mspritegroup_t *)psprite->frames[frame].frameptr;
		pintervals = pspritegroup->intervals;
		numframes = pspritegroup->numframes;
		fullinterval = pintervals[numframes-1];

		// when loading in Mod_LoadSpriteGroup, we guaranteed all interval values
		// are positive, so we don't have to worry about division by zero
		targettime = gEngfuncs.GetClientTime() - ((int)( gEngfuncs.GetClientTime() / fullinterval )) * fullinterval;

		for( i = 0; i < (numframes - 1); i++ )
		{
			if( pintervals[i] > targettime )
				break;
		}
		pspriteframe = pspritegroup->frames[i];
	}
	
	return pspriteframe;
}

/*
================
TRI_GetSpriteParms

================
*/

void TRI_GetSpriteParms( model_t *pSprite, int *frameWidth, int *frameHeight, int *numFrames, int currentFrame )
{
	mspriteframe_t	*pFrame;

	if ( !pSprite || pSprite->type != mod_sprite ) 
		return;

	pFrame = R_GetSpriteFrame( pSprite, currentFrame, 0.0f);

	if( frameWidth ) *frameWidth = pFrame->width;
	if( frameHeight ) *frameHeight = pFrame->height;
	if( numFrames ) *numFrames = pSprite->numframes;
}

/*
================
TRI_SprAdjustSize

================
*/

void TRI_SprAdjustSize( float *x, float *y, float *w, float *h )
{
	float xscale, yscale;

	if ( !x && !y && !w && !h ) 
	  return;

	// scale for screen sizes
	xscale = ScreenWidth / (float)gHUD.m_iHudScaleWidth;
	yscale = ScreenHeight / (float)gHUD.m_iHudScaleHeight;
	
	if( x ) *x *= xscale;
	if( y ) *y *= yscale;
	if( w ) *w *= xscale;
	if( h ) *h *= yscale;
}

/*
================
TRI_GetSpriteParms

================
*/

void TRI_SprDrawStretchPic( model_t* pModel, int frame, float x, float y, float w, float h, float s1, float t1, float s2, float t2 )
{
	if ( pModel == NULL )
		return;

	gEngfuncs.pTriAPI->SpriteTexture( pModel, frame );

	gEngfuncs.pTriAPI->Begin(TRI_QUADS);

	gEngfuncs.pTriAPI->TexCoord2f(s1, t1);
	gEngfuncs.pTriAPI->Vertex3f(x, y, 0);

	gEngfuncs.pTriAPI->TexCoord2f(s2, t1);
	gEngfuncs.pTriAPI->Vertex3f(x + w, y, 0);

	gEngfuncs.pTriAPI->TexCoord2f(s2, t2);
	gEngfuncs.pTriAPI->Vertex3f(x + w, y + h, 0);

	gEngfuncs.pTriAPI->TexCoord2f(s1, t2);
	gEngfuncs.pTriAPI->Vertex3f(x, y + h, 0);

	gEngfuncs.pTriAPI->End();
}

/*
================
TRI_GetSpriteParms

================
*/

void TRI_SprDrawGeneric( int frame, float x, float y, wrect_t* prc)
{
	float	s1, s2, t1, t2, flScale;
	float width, height;
	int	w, h;

	// assume we get sizes from image
	TRI_GetSpriteParms( TRI_pModel, &w, &h, NULL, frame );

	width = w;
	height = h;

	if (prc)
	{
		wrect_t	rc = *prc;

		if( rc.left <= 0 || rc.left >= width ) rc.left = 0;
		if( rc.top <= 0 || rc.top >= height ) rc.top = 0;
		if( rc.right <= 0 || rc.right > width ) rc.right = width;
		if( rc.bottom <= 0 || rc.bottom > height ) rc.bottom = height;

		if ( gHUD.m_pCvarScale->value )
			flScale = 0.125;
		else
			flScale = 0.0;

		s1 = ((float)rc.left + flScale) / width;
		t1 = ((float)rc.top + flScale) / height;
		s2 = ((float)rc.right - flScale) / width;
		t2 = ((float)rc.bottom - flScale) / height;

		width = rc.right - rc.left;
		height = rc.bottom - rc.top;
	}
	else
	{
		s1 = t1 = 0.0;
		s2 = t2 = 1.0;
	}

	TRI_SprAdjustSize( &x, &y, &width, &height);

	TRI_SprDrawStretchPic( TRI_pModel, frame, x, y, width, height, s1, t1, s2, t2);
}

/*
================
TRI_GetSpriteParms

================
*/

void TRI_SprDrawAdditive( int frame, int x, int y, wrect_t *prc)
{
	if (!IEngineStudio.IsHardware())
	{
		gEngfuncs.pfnSPR_DrawAdditive(frame, x, y, prc);
		return;
	}

	gEngfuncs.pTriAPI->RenderMode(kRenderTransAdd);

	TRI_SprDrawGeneric(frame, x, y, prc);

	gEngfuncs.pTriAPI->RenderMode(kRenderNormal);
}

/*
================
TRI_GetSpriteParms

================
*/

void TRI_SprSet( HSPRITE spr, int r, int g, int b)
{
	if (!IEngineStudio.IsHardware())
	{
		gEngfuncs.pfnSPR_Set(spr, r, g, b);
		return;
	}

	TRI_pModel = (model_s*)gEngfuncs.GetSpritePointer(spr);

	gEngfuncs.pTriAPI->Color4ub(r, g, b, 255);
}

/*
================
TRI_DrawConsoleString

SERECKY JAN-18-26: NEW
================
*/

void TRI_DrawConsoleString( float x, float y, const char *string, float width, float height )
{
	if (!IEngineStudio.IsHardware())
	{
		DrawConsoleString(x, y, string);
		return;
	}

	TRI_SprAdjustSize( &x, &y, &width, &height);
	DrawConsoleString( x, y, string );
}

/*
================
TRI_FillRGBA

================
*/

void TRI_FillRGBA( float x, float y, float width, float height, int r, int g, int b, int a, int additive)
{
	if (!IEngineStudio.IsHardware())
	{
		gEngfuncs.pfnFillRGBA((int)x, (int)y, (int)width, (int)height, r, g, b, a);
		return;
	}

	TRI_SprAdjustSize( &x, &y, &width, &height);

	if (additive)
	{
		gEngfuncs.pfnFillRGBA(x, y, width, height, r, g, b, a);
	}
	else // NEW - serecky 1.2.26
	{
		float flRGBA[4];

		flRGBA[0] = r * (1.0f / 255.0f);
		flRGBA[1] = g * (1.0f / 255.0f);
		flRGBA[2] = b * (1.0f / 255.0f);
		flRGBA[3] = a * (1.0f / 255.0f);

		gEngfuncs.pTriAPI->RenderMode(kRenderTransAlpha);
		gEngfuncs.pTriAPI->Color4f(flRGBA[0], flRGBA[1], flRGBA[2], flRGBA[3]);

		gEngfuncs.pTriAPI->SpriteTexture( (struct model_s*)gEngfuncs.GetSpritePointer(SPR_Load("sprites/white.spr")), 0 );

		gEngfuncs.pTriAPI->Begin(TRI_QUADS);

		gEngfuncs.pTriAPI->TexCoord2f(0, 0);
		gEngfuncs.pTriAPI->Vertex3f(x, y, 0);

		gEngfuncs.pTriAPI->TexCoord2f(0, 1);
		gEngfuncs.pTriAPI->Vertex3f(x + width, y, 0);

		gEngfuncs.pTriAPI->TexCoord2f(1, 1);
		gEngfuncs.pTriAPI->Vertex3f(x + width, y + height, 0);

		gEngfuncs.pTriAPI->TexCoord2f(1, 0);
		gEngfuncs.pTriAPI->Vertex3f(x, y + height, 0);

		gEngfuncs.pTriAPI->End();

		gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	}

}

/*
================
TRI_SprDrawHoles

================
*/

void TRI_SprDrawHoles( int frame, int x, int y, wrect_t *prc)
{
	if (!IEngineStudio.IsHardware())
	{
		gEngfuncs.pfnSPR_DrawHoles(frame, x, y, prc);
		return;
	}

	gEngfuncs.pTriAPI->RenderMode( kRenderTransAlpha );

	TRI_SprDrawGeneric(frame, x, y, prc);

	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}
// ThrillEX Addition/Edit End

//#define TEST_IT
#if defined( TEST_IT )

/*
=================
Draw_Triangles

Example routine.  Draws a sprite offset from the player origin.
=================
*/
void Draw_Triangles( void )
{
	cl_entity_t *player;
	vec3_t org;

	// Load it up with some bogus data
	player = gEngfuncs.GetLocalPlayer();
	if ( !player )
		return;

	org = player->origin;

	org.x += 50;
	org.y += 50;

	if (gHUD.m_hsprCursor == 0)
	{
		char sz[256];
		sprintf( sz, "sprites/cursor.spr" );
		gHUD.m_hsprCursor = SPR_Load( sz );
	}

	if ( !gEngfuncs.pTriAPI->SpriteTexture( (struct model_s *)gEngfuncs.GetSpritePointer( gHUD.m_hsprCursor ), 0 ))
	{
		return;
	}
	
	// Create a triangle, sigh
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
	gEngfuncs.pTriAPI->CullFace( TRI_NONE );
	gEngfuncs.pTriAPI->Begin( TRI_QUADS );
	// Overload p->color with index into tracer palette, p->packedColor with brightness
	gEngfuncs.pTriAPI->Color4f( 1.0, 1.0, 1.0, 1.0 );
	// UNDONE: This gouraud shading causes tracers to disappear on some cards (permedia2)
	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 0, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 1 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y + 50, org.z );

	gEngfuncs.pTriAPI->Brightness( 1 );
	gEngfuncs.pTriAPI->TexCoord2f( 1, 0 );
	gEngfuncs.pTriAPI->Vertex3f( org.x + 50, org.y, org.z );

	gEngfuncs.pTriAPI->End();
	gEngfuncs.pTriAPI->RenderMode( kRenderNormal );
}

#endif

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles( void )
{

	gHUD.m_Spectator.DrawOverview();
	
#if defined( TEST_IT )
//	Draw_Triangles();
#endif
}

/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles( void )
{

#if defined( TEST_IT )
//	Draw_Triangles();
#endif
}