/*
Omnispeak: A Commander Keen Reimplementation
Copyright (C) 2012 David Gow <david@ingeniumdigital.com>

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "id_ca.h"
#include "id_in.h"
#include "id_vl.h"
#include "id_rf.h"
#include "id_sd.h"
#include "ck_play.h"
#include "ck_phys.h"
#include "ck_def.h"
#include "ck_game.h"
#include "ck_act.h"
#include "ck4_ep.h"
#include "ck5_ep.h"

#include <string.h>
#include <stdio.h>

void CK_MapKeenWalk(CK_object * obj);

// =========================================================================


void CK_DemoSignSpawn()
{

	ck_scoreBoxObj->type = CT_Friendly;
	ck_scoreBoxObj->zLayer = 3;
	ck_scoreBoxObj->active = OBJ_ALWAYS_ACTIVE;
	ck_scoreBoxObj->clipped = CLIP_not;

	// Set all user vars to -1 to force scorebox redraw
	ck_scoreBoxObj->user1 = ck_scoreBoxObj->user2 = ck_scoreBoxObj->user3 = ck_scoreBoxObj->user4 = -1;

	if (ck_inHighScores)
	{
		// Don't display anything in the high scores
		ck_scoreBoxObj->currentAction = CK_GetActionByName("CK_ACT_NULL");
	}
	else if (IN_DemoGetMode())
	{
		// If this is a demo, display the DEMO banner
		CK_SetAction(ck_scoreBoxObj, CK_GetActionByName("CK_ACT_DemoSign"));
		CA_CacheGrChunk(SPR_DEMOSIGN);
	}
	else
	{
		// If a normal game, display the scorebox
		CK_SetAction(ck_scoreBoxObj, CK_GetActionByName("CK_ACT_ScoreBox"));
	}
}

void CK_DemoSign( CK_object *demo)
{
	if (	demo->posX == rf_scrollXUnit && demo->posY == rf_scrollYUnit )
		return;
	demo->posX = rf_scrollXUnit;
	demo->posY = rf_scrollYUnit;

	//place demo sprite in center top
	RF_AddSpriteDraw( &(demo->sde), demo->posX+0x0A00 - 0x200, demo->posY+0x80,SPR_DEMOSIGN,false,3);
}

/*
 * ScoreBox update
 * Scorebox works by drawing tile8s over the sprite that has been cached in
 * memory.
 *
 * Because vanilla keen cached four shifts of the scorebox, this redraw
 * procedure would have to be repeated for each shift.
 *
 * This is not the case for omnispeak, which just caches one copy of
 * each sprite.
 *
 */

/*
 * Draws a Tile8 to an unmasked planar graphic
 */
void CK_ScoreBoxDrawTile8(int tilenum, uint8_t *dest, int destWidth, int planeSize)
{
	uint8_t *src = (uint8_t *)ca_graphChunks[ca_gfxInfoE.offTiles8] + 32 * tilenum;

	// Copy the tile to the target bitmap
	for (int plane = 0; plane < 4; plane++, dest+=planeSize)
	{
		uint8_t *d = dest;
		for (int row = 0; row < 8; row++)
		{
			*d = *(src++);
			d += destWidth;
		}
	}
}

void CK_UpdateScoreBox(CK_object *scorebox)
{

	bool updated = false;

	// Don't draw anything for the high score level
	if (ck_inHighScores)
		return;

	// Show the demo sign for the demo mode
	if (IN_DemoGetMode())
	{
		CK_DemoSign(scorebox);
		return;
	}

	if (!ck_scoreBoxEnabled)
		return;


	// NOTE: Score is stored in user1 and user2 in original keen
	// Can just use user1 here, but be careful about loading/saving games!


	// Draw the score if it's changed
	if (scorebox->user1 != ck_gameState.keenScore)
	{
		int place, len, planeSize;
		char buf[16];
		uint8_t* dest;

		VH_SpriteTableEntry box = VH_GetSpriteTableEntry(SPR_SCOREBOX - ca_gfxInfoE.offSprites);

		// Start drawing the tiles after the mask plane,
		// and four rows from the top
		dest = (uint8_t*)ca_graphChunks[SPR_SCOREBOX];
		dest += (planeSize = box.width * box.height);
		dest += box.width * 4 + 1;

		sprintf(buf, "%d", ck_gameState.keenScore);
		len = strlen(buf);

		// Draw the leading emptiness
		for (place = 9; place > len; place--)
		{
			CK_ScoreBoxDrawTile8(0x29, dest++, box.width, planeSize);
		}

		// Draw the score
		for (char *c = buf; *c != 0; c++)
		{
			CK_ScoreBoxDrawTile8(*c - 6, dest++, box.width, planeSize);

		}

		scorebox->user1 = ck_gameState.keenScore;
		updated = true;

	}

	// Draw the number of shots if it's changed
	if (scorebox->user3 != ck_gameState.numShots)
	{
		int place, len, planeSize;
		char buf[16];
		uint8_t* dest;

		VH_SpriteTableEntry box = VH_GetSpriteTableEntry(SPR_SCOREBOX - ca_gfxInfoE.offSprites);

		// Start drawing the tiles after the mask plane,
		// and 12 rows from the top
		dest = (uint8_t*)ca_graphChunks[SPR_SCOREBOX];
		dest += (planeSize = box.width * box.height);
		dest += box.width * 20 + 8;

		if (ck_gameState.numShots >= 99)
			sprintf(buf, "99");
		else
			sprintf(buf, "%d", ck_gameState.numShots);

		len = strlen(buf);

		// Draw the leading emptiness
		for (place = 2; place > len; place--)
		{
			CK_ScoreBoxDrawTile8(0x29, dest++, box.width, planeSize);
		}

		// Draw the score
		for (char *c = buf; *c != 0; c++)
		{
			CK_ScoreBoxDrawTile8(*c - 6, dest++, box.width, planeSize);
		}

		scorebox->user3 = ck_gameState.numShots;
		updated = true;
	}

	// Draw the number of lives if it's changed
	if (scorebox->user4 != ck_gameState.numLives)
	{
		int place, len, planeSize;
		char buf[16];
		uint8_t* dest;

		VH_SpriteTableEntry box = VH_GetSpriteTableEntry(SPR_SCOREBOX - ca_gfxInfoE.offSprites);

		// Start drawing the tiles after the mask plane,
		// and 12 rows from the top
		dest = (uint8_t*)ca_graphChunks[SPR_SCOREBOX];
		dest += (planeSize = box.width * box.height);
		dest += box.width * 20 + 3;

		if (ck_gameState.numLives >= 99)
			sprintf(buf, "99");
		else
			sprintf(buf, "%d", ck_gameState.numLives);

		len = strlen(buf);

		// Draw the leading emptiness
		for (place = 2; place > len; place--)
		{
			CK_ScoreBoxDrawTile8(0x29, dest++, box.width, planeSize);
		}

		// Draw the score
		for (char *c = buf; *c != 0; c++)
		{
			CK_ScoreBoxDrawTile8(*c - 6, dest++, box.width, planeSize);
		}

		scorebox->user4 = ck_gameState.numLives;
		updated = true;
	}

	// Now draw the scorebox to the screen
	if (scorebox->posX != rf_scrollXUnit || scorebox->posY != rf_scrollYUnit)
	{
		scorebox->posX = rf_scrollXUnit;
		scorebox->posY = rf_scrollYUnit;
		updated = true;
	}

	if (updated)
		RF_AddSpriteDraw(&scorebox->sde, scorebox->posX + 0x40, scorebox->posY + 0x40, SPR_SCOREBOX, false, 3);

}

// =========================================================================


/*
 * MapKeen Thinks
 * user1 stores compass direction
 * user2 stores animation frame counter
 * user3 stores some sort of velocity
 */

int *ck_mapKeenFrames;
static int word_417BA[] ={ 2, 3, 1, 3, 4, 6, 0, 2};

void CK_SpawnMapKeen(int tileX, int tileY)
{

	ck_keenObj->type = 2;
	if (ck_gameState.mapPosX == 0)
	{
		ck_keenObj->posX = (tileX << 8);
		ck_keenObj->posY = (tileY << 8);
	}
	else
	{
		ck_keenObj->posX = ck_gameState.mapPosX;
		ck_keenObj->posY = ck_gameState.mapPosY;
	}

	ck_keenObj->active = OBJ_ALWAYS_ACTIVE;
	ck_keenObj->zLayer = 1;
	ck_keenObj->xDirection= ck_keenObj->yDirection = IN_motion_None;
	ck_keenObj->user1 = 6;
	ck_keenObj->user2 = 3;
	ck_keenObj->user3 = 0;
  ck_keenObj->gfxChunk = SPR_MAPKEEN_STAND_W;
	CK_SetAction(ck_keenObj, CK_GetActionByName("CK_ACT_MapKeenStart"));
}

//look for level entrry

void CK_ScanForLevelEntry(CK_object * obj)
{

	int tx, ty;
	int tileY_0 = obj->clipRects.tileY1;

	for (ty = obj->clipRects.tileY1; ty <= obj->clipRects.tileY2; ty++)
	{
		for (tx = obj->clipRects.tileX1; tx <= obj->clipRects.tileX2; tx++)
		{
			int infotile =CA_TileAtPos(tx, ty, 2);
			if (infotile > 0xC000 && infotile < 0xC012)
			{
				// Vanilla keen stores the current map loaded in the cache manager
				// and the "current_map" variable stored in the gamestate
				// would have been changed here.
				ck_gameState.mapPosX = obj->posX;
				ck_gameState.mapPosY = obj->posY;
				ck_gameState.currentLevel = infotile - 0xC000;
				ck_gameState.levelState = 2;
				SD_PlaySound(SOUND_UNKNOWN12);
				return;
			}
		}
	}
}

void CK_MapKeenStill(CK_object * obj)
{

	if (ck_inputFrame.dir != IN_dir_None)
	{
		obj->currentAction = CK_GetActionByName("CK_ACT_MapKeenWalk0");
		obj->user2 = 0;
		CK_MapKeenWalk(obj);
	}

	if (ck_keenState.jumpIsPressed || ck_keenState.pogoIsPressed || ck_keenState.shootIsPressed)
	{
		CK_ScanForLevelEntry(obj);
	}
}

void CK_MapKeenWalk(CK_object * obj)
{

	if (obj->user3 == 0)
	{
		obj->xDirection = ck_inputFrame.xDirection;
		obj->yDirection = ck_inputFrame.yDirection;
		if (ck_keenState.pogoIsPressed || ck_keenState.jumpIsPressed || ck_keenState.shootIsPressed)
			CK_ScanForLevelEntry(obj);

		// Go back to standing if no arrows pressed
		if (ck_inputFrame.dir == IN_dir_None)
		{
			obj->currentAction = CK_GetActionByName("CK_ACT_MapKeenStart");
			obj->gfxChunk = ck_mapKeenFrames[obj->user1] + 3;
			return;
		}
		else
		{
			obj->user1 = ck_inputFrame.dir;
		}
	}
	else
	{
		if ((obj->user3 -= 4) < 0)
			obj->user3 = 0;
	}

	// Advance Walking Frame Animation
	if (++obj->user2 == 4)
		obj->user2 = 0;
	obj->gfxChunk = ck_mapKeenFrames[obj->user1] + word_417BA[obj->user2];

	//walk hi lo sound
	if (obj->user2 == 1)
	{
		SD_PlaySound(SOUND_KEENWALK0);
	}
	else if (obj->user2 == 3)
	{
		SD_PlaySound(SOUND_KEENWALK1);
	}
}

//Thisis called from playloop

#define MISCFLAG_TELEPORT 0x14
#define MISCFLAG_LEFTELEVATOR 0x21
#define MISCFLAG_RIGHTELEVATOR 0x22

void CK_MapMiscFlagsCheck(CK_object *keen)
{

	int midTileX, midTileY;

	if (keen->user3)
		return;

	midTileX = keen->clipRects.tileXmid;
	midTileY = ((keen->clipRects.unitY2 - keen->clipRects.unitY1) / 2 + keen->clipRects.unitY1) >> 8;

	switch (TI_ForeMisc(CA_TileAtPos(midTileX, midTileY, 1)))
	{

	case MISCFLAG_TELEPORT:
		CK5_AnimateMapTeleporter(midTileX, midTileY);
		break;

	case MISCFLAG_LEFTELEVATOR:
		CK5_AnimateMapElevator(midTileX, midTileY, 0);
		break;

	case MISCFLAG_RIGHTELEVATOR:
		CK5_AnimateMapElevator(midTileX, midTileY, -1);
		break;
	}
}

// =========================================================================
// Map Flags

typedef struct
{
  uint16_t x, y;

} CK_FlagPoint;

static CK_FlagPoint ck_flagPoints[30];

void CK_MapFlagSpawn(int tileX, int tileY)
{

	CK_object *flag = CK_GetNewObj(false);

	flag->clipped = CLIP_not;
	flag->zLayer = 3;
  flag->type = CT_CLASS(MapFlag);
	flag->active = OBJ_ACTIVE;
  flag->posX = (tileX << 8) + (ck_currentEpisode->ep == EP_CK5 ? -0x50 : 0x60);
	flag->posY = (tileY << 8) - 0x1E0;
	flag->actionTimer = US_RndT() / 16;

	CK_SetAction(flag, CK_GetActionByName("CK_ACT_MapFlag0"));
}

void CK_FlippingFlagSpawn(int tileX, int tileY)
{
  int32_t dx, dy;

  CK_object *obj = CK_GetNewObj(false);
  obj->clipped = CLIP_not;
  obj->zLayer = PRIORITIES - 1;
  obj->type = CT_CLASS(MapFlag);
  obj->posX = ck_gameState.mapPosX - 0x100;
  obj->posY = ck_gameState.mapPosY - 0x100;

  // Destination coords
  obj->user1 = (tileX << G_T_SHIFT) + 0x60;
  obj->user2 = (tileY << G_T_SHIFT) - 0x260;

  dx = (int32_t)obj->user1 - (int32_t)obj->posX;
  dy = (int32_t)obj->user2 - (int32_t)obj->posY;

  // Make a table of coordinates for the flag's path
  for (int i = 0; i < 30; i++)
  {
     // Draw points in a straight line between keen and the holster
     ck_flagPoints[i].x = obj->posX + dx * (i < 24 ? i : 24) / 24;
     ck_flagPoints[i].y = obj->posY + dy * i / 30;

     // Offset th eY points to mimic a parabolic trajectory
     if (i < 10)
       ck_flagPoints[i].y -= i * 0x30; // going up
     else if (i < 15)
       ck_flagPoints[i].y -= i * 16 + 0x140;
     else if (i < 20)
       ck_flagPoints[i].y -= (20 - i) * 16 + 0x1E0;
     else
       ck_flagPoints[i].y -= (29 - i) * 0x30;
  }

  CK_SetAction(obj, CK_GetActionByName("CK_ACT_MapFlagFlips0"));

}

void CK_MapFlagThrown(CK_object *obj)
{
  // Might this be a source of non-determinism?
  // (if screen unfades at different rates based on diff architecture)
  if (!vl_screenFaded)
  {
    SD_StopSound();
    SD_PlaySound(SOUND_FLAGFLIP);
    obj->currentAction = obj->currentAction->next;
  }

}

void CK_MapFlagFall(CK_object *obj)
{
  obj->user3 += SD_GetSpriteSync();

  if (obj->user3 > 50)
    obj->user3 = 50;

  obj->posX = ck_flagPoints[obj->user3/2].x;
  obj->posY = ck_flagPoints[obj->user3/2].y;

  obj->visible = true;
  if (!obj->user1)
    SD_PlaySound(SOUND_FLAGFLIP);
}

void CK_MapFlagLand(CK_object *obj)
{
  // Plop the flag in its holster
  obj->posX = obj->user1;
  obj->posY = obj->user2 + 0x80;
  obj->zLayer = PRIORITIES - 1;

  SD_PlaySound(SOUND_FLAGLAND);
}

/*
 * Setup all of the functions in this file.
 */
void CK_Map_SetupFunctions()
{
	CK_ACT_AddFunction("CK_MapKeenStill", &CK_MapKeenStill);
	CK_ACT_AddFunction("CK_MapKeenWalk", &CK_MapKeenWalk);
  CK_ACT_AddFunction("CK_MapFlagThrown", &CK_MapFlagThrown);
  CK_ACT_AddFunction("CK_MapFlagFall", &CK_MapFlagFall);
  CK_ACT_AddFunction("CK_MapFlagLand", &CK_MapFlagLand);
}