/*
===========================================================================

Return to Castle Wolfenstein multiplayer GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein multiplayer GPL Source Code (RTCW MP Source Code).

RTCW MP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW MP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW MP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW MP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the RTCW MP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "g_local.h"

#define IS_ACTIVE( x ) ( \
		x->r.linked == qtrue &&	\
		x->client->ps.stats[STAT_HEALTH] > 0 &&	\
		x->client->sess.sessionTeam != TEAM_SPECTATOR && \
		( x->client->ps.pm_flags & PMF_LIMBO ) == 0	\
		)

// Called from ClientEndFrame() -- stores the client's position at level.time,
// which is the position that will appear in the snapshot clients receive.
void G_StoreClientPosition( gentity_t* ent ) {
	int top;

	if ( !IS_ACTIVE( ent ) ) {
		return;
	}

	top = ( ent->client->topMarker + 1 ) % MAX_CLIENT_MARKERS;
	ent->client->topMarker = top;

	VectorCopy( ent->r.mins,            ent->client->clientMarkers[top].mins );
	VectorCopy( ent->r.maxs,            ent->client->clientMarkers[top].maxs );
	VectorCopy( ent->r.currentOrigin,   ent->client->clientMarkers[top].origin );
	ent->client->clientMarkers[top].time = level.time;
}

static void G_AdjustSingleClientPosition( gentity_t* ent, int time ) {
	int i, j;

	if ( time > level.time ) {
		time = level.time;
	}

	i = j = ent->client->topMarker;
	do {
		if ( ent->client->clientMarkers[i].time <= time ) {
			break;
		}

		j = i;
		i = ( ( i > 0 ) ? ( i ) : ( MAX_CLIENT_MARKERS ) ) - 1;
	} while ( i != ent->client->topMarker );

	if ( i == j ) {
		return;
	}

	// save current position for restore
	VectorCopy( ent->r.currentOrigin,    ent->client->backupMarker.origin );
	VectorCopy( ent->r.mins,             ent->client->backupMarker.mins );
	VectorCopy( ent->r.maxs,             ent->client->backupMarker.maxs );

	if ( i != ent->client->topMarker ) {
		float frac = ( (float)( ent->client->clientMarkers[j].time - time ) ) / ( ent->client->clientMarkers[j].time - ent->client->clientMarkers[i].time );

		LerpPosition( ent->client->clientMarkers[j].origin,      ent->client->clientMarkers[i].origin,   frac,   ent->r.currentOrigin );
		LerpPosition( ent->client->clientMarkers[j].mins,        ent->client->clientMarkers[i].mins,     frac,   ent->r.mins );
		LerpPosition( ent->client->clientMarkers[j].maxs,        ent->client->clientMarkers[i].maxs,     frac,   ent->r.maxs );
	} else {
		VectorCopy( ent->client->clientMarkers[j].origin,       ent->r.currentOrigin );
		VectorCopy( ent->client->clientMarkers[j].mins,         ent->r.mins );
		VectorCopy( ent->client->clientMarkers[j].maxs,         ent->r.maxs );
	}

	trap_LinkEntity( ent );
}

static void G_ReAdjustSingleClientPosition( gentity_t* ent ) {
	VectorCopy( ent->client->backupMarker.origin,       ent->r.currentOrigin );
	VectorCopy( ent->client->backupMarker.mins,         ent->r.mins );
	VectorCopy( ent->client->backupMarker.maxs,         ent->r.maxs );

	trap_LinkEntity( ent );
}

void G_AdjustClientPositions( gentity_t* ent, int time, qboolean forward ) {
	int i;
	gentity_t   *list;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		list = g_entities + level.sortedClients[i];
		if ( list != ent && IS_ACTIVE( list ) ) {
			if ( forward ) {
				G_AdjustSingleClientPosition( list, time );
			} else { G_ReAdjustSingleClientPosition( list );}
		}
	}
}

void G_ResetMarkers(gentity_t *ent)
{
	int   i, time;
	char  buffer[MAX_CVAR_VALUE_STRING];
	float period;

	trap_Cvar_VariableStringBuffer("sv_fps", buffer, sizeof(buffer) - 1);

	period = atoi(buffer);
	if (!period)
	{
		period = 50;
	}
	else
	{
		period = 1000.f / period;
	}

	ent->client->topMarker = MAX_CLIENT_MARKERS - 1;
	for (i = MAX_CLIENT_MARKERS - 1, time = level.time; i >= 0; i--, time -= period)
	{
		VectorCopy(ent->r.mins, ent->client->clientMarkers[i].mins);
		VectorCopy(ent->r.maxs, ent->client->clientMarkers[i].maxs);
		VectorCopy(ent->r.currentOrigin, ent->client->clientMarkers[i].origin);
		ent->client->clientMarkers[i].time = time;
	}
}

void G_AttachBodyParts( gentity_t* ent ) {
	int i;
	gentity_t   *list;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		list = g_entities + level.sortedClients[i];
		list->client->tempHead = ( list != ent && IS_ACTIVE( list ) ) ? G_BuildHead( list ) : NULL;
	}
}

void G_DettachBodyParts( void ) {
	int i;
	gentity_t   *list;

	for ( i = 0; i < level.numConnectedClients; i++ ) {
		list = g_entities + level.sortedClients[i];
		if ( list->client->tempHead != NULL ) {
			G_FreeEntity( list->client->tempHead );
		}
	}
}

int G_SwitchBodyPartEntity( gentity_t* ent ) {
	if ( ent->s.eType == ET_TEMPHEAD ) {
		return ent->parent - g_entities;
	}

	return ent - g_entities;
}

// Handles hit-box remapping and position backing-off
static void ResolveTraceCollision( trace_t *results, const vec3_t start, const vec3_t end ) {
    // Check if the body part entity needs to be mapped back to the main entity
    int actualEntityNum = G_SwitchBodyPartEntity( &g_entities[results->entityNum] );

    if ( actualEntityNum != results->entityNum ) {
        vec3_t dir;

        // Calculate direction of the trace
        VectorSubtract( end, start, dir );
        VectorNormalizeFast( dir );

        // Back off the hit position by 1 unit to prevent sticking/clipping
        // issues when the entity index changes.
        VectorMA( results->endpos, -1, dir, results->endpos );

        // Update the result with the correct entity index
        results->entityNum = actualEntityNum;
    }
}

static void PerformBodyPartTrace( gentity_t *ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) {
    G_AttachBodyParts( ent );

    trap_Trace( results, start, mins, maxs, end, passEntityNum, contentmask );
    ResolveTraceCollision( results, start, end );

    G_DettachBodyParts();
}

void G_HistoricalTrace( gentity_t* ent, trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask ) {

    // Determine if we need to rewind time
    qboolean useAntilag = ( g_antilag.integer && ent->client );

    if ( useAntilag ) {
        // Rewind players to positions at the time the shot was fired
        G_AdjustClientPositions( ent, ent->client->pers.cmd.serverTime, qtrue );
    }

    // Run the actual trace
    PerformBodyPartTrace( ent, results, start, mins, maxs, end, passEntityNum, contentmask );

    if ( useAntilag ) {
        // Restore players to current positions
        G_AdjustClientPositions( ent, 0, qfalse );
    }
}
