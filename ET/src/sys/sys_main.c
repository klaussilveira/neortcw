/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Quake III Arena source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include <signal.h>
#include <stdlib.h>
#include <limits.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#ifndef DEDICATED
#ifdef USE_LOCAL_HEADERS
#	include "SDL.h"
#	include "SDL_cpuinfo.h"
#else
#	include <SDL.h>
#	include <SDL_cpuinfo.h>
#endif
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "sys_local.h"
#include "sys_loadlib.h"

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"

#include <math.h>

// Forward declarations for functions used before their definition
void Sys_SendKeyEvents( void );
qboolean Sys_GetPacket( netadr_t *net_from, msg_t *net_message );

// com_ansiColor cvar (not in ET's qcommon)
cvar_t *com_ansiColor;

#ifdef __EMSCRIPTEN__
// stdin_active referenced by unix_net.c but not defined in con_passive.c
qboolean stdin_active = qfalse;
#endif

// stdinIsATTY defined in sys_unix.c/sys_win32.c

// com_fullyInitialized is defined in common.c but not extern'd in ET's qcommon.h
extern qboolean com_fullyInitialized;

static char binaryPath[ MAX_OSPATH ] = { 0 };
static char installPath[ MAX_OSPATH ] = { 0 };

/*
=================
Sys_SetBinaryPath
=================
*/
void Sys_SetBinaryPath(const char *path)
{
	Q_strncpyz(binaryPath, path, sizeof(binaryPath));
}

/*
=================
Sys_BinaryPath
=================
*/
char *Sys_BinaryPath(void)
{
	return binaryPath;
}

/*
=================
Sys_SetDefaultInstallPath
=================
*/
void Sys_SetDefaultInstallPath(const char *path)
{
	Q_strncpyz(installPath, path, sizeof(installPath));
}

/*
=================
Sys_DefaultInstallPath
=================
*/
char *Sys_DefaultInstallPath(void)
{
	if (*installPath)
		return installPath;
	else
		return Sys_Cwd();
}

/*
=================
Sys_DefaultAppPath
=================
*/
char *Sys_DefaultAppPath(void)
{
	return Sys_BinaryPath();
}

/*
=================
Sys_In_Restart_f

Restart the input subsystem
=================
*/
void Sys_In_Restart_f( void )
{
#ifndef DEDICATED
	if( !SDL_WasInit( SDL_INIT_VIDEO ) )
	{
		Com_Printf( "in_restart: Cannot restart input while video is shutdown\n" );
		return;
	}

	IN_Restart( );
#endif
}

/*
=================
Sys_ConsoleInput

Handle new console input
=================
*/
char *Sys_ConsoleInput(void)
{
	return CON_Input( );
}

/*
==================
Sys_GetClipboardData
==================
*/
char *Sys_GetClipboardData(void)
{
#ifdef DEDICATED
	return NULL;
#else
	char *data = NULL;
	char *cliptext;

	if ( ( cliptext = SDL_GetClipboardText() ) != NULL ) {
		if ( cliptext[0] != '\0' ) {
			size_t bufsize = strlen( cliptext ) + 1;

			data = Z_Malloc( bufsize );
			Q_strncpyz( data, cliptext, bufsize );

			// find first listed char and set to '\0'
			strtok( data, "\n\r\b" );
		}
		SDL_free( cliptext );
	}
	return data;
#endif
}

#ifdef DEDICATED
#	define PID_FILENAME "iowolfet_server.pid"
#else
#	define PID_FILENAME "iowolfet.pid"
#endif

/*
=================
Sys_PIDFileName
=================
*/
static char *Sys_PIDFileName( const char *gamedir )
{
	const char *homePath = Cvar_VariableString( "fs_homepath" );

	if( *homePath != '\0' )
		return va( "%s/%s/%s", homePath, gamedir, PID_FILENAME );

	return NULL;
}

/*
=================
Sys_RemovePIDFile
=================
*/
void Sys_RemovePIDFile( const char *gamedir )
{
	char *pidFile = Sys_PIDFileName( gamedir );

	if( pidFile != NULL )
		remove( pidFile );
}

/*
=================
Sys_WritePIDFile

Return qtrue if there is an existing stale PID file
=================
*/
static qboolean Sys_WritePIDFile( const char *gamedir )
{
	char      *pidFile = Sys_PIDFileName( gamedir );
	FILE      *f;
	qboolean  stale = qfalse;

	if( pidFile == NULL )
		return qfalse;

	// First, check if the pid file is already there
	if( ( f = fopen( pidFile, "r" ) ) != NULL )
	{
		char  pidBuffer[ 64 ] = { 0 };
		int   pid;

		pid = fread( pidBuffer, sizeof( char ), sizeof( pidBuffer ) - 1, f );
		fclose( f );

		if(pid > 0)
		{
			pid = atoi( pidBuffer );
			if( !Sys_PIDIsRunning( pid ) )
				stale = qtrue;
		}
		else
			stale = qtrue;
	}

	if( FS_CreatePath( pidFile ) ) {
		return 0;
	}

	if( ( f = fopen( pidFile, "w" ) ) != NULL )
	{
		fprintf( f, "%d", Sys_PID( ) );
		fclose( f );
	}
	else
		Com_Printf( S_COLOR_YELLOW "Couldn't write %s.\n", pidFile );

	return stale;
}

/*
=================
Sys_InitPIDFile
=================
*/
void Sys_InitPIDFile( const char *gamedir ) {
	if( Sys_WritePIDFile( gamedir ) ) {
#ifndef DEDICATED
		char message[1024];

		Com_sprintf( message, sizeof (message), "The last time Wolf ET ran, "
			"it didn't exit properly. This may be due to inappropriate video "
			"settings. Would you like to start with \"safe\" video settings?" );

		if( Sys_Dialog( DT_YES_NO, message, "Abnormal Exit" ) == DR_YES ) {
			Cvar_Set( "com_abnormalExit", "1" );
		}
#endif
	}
}

/*
=================
Sys_Exit

Single exit point (regular exit or in case of error)
=================
*/
static __attribute__ ((noreturn)) void Sys_Exit( int exitCode )
{
	CON_Shutdown( );

#ifndef DEDICATED
	SDL_Quit( );
#endif

	if( exitCode < 2 && com_fullyInitialized )
	{
		// Normal exit
		const char *gamedir = Cvar_VariableString( "fs_game" );
		if ( gamedir[0] )
			Sys_RemovePIDFile( gamedir );
		else
			Sys_RemovePIDFile( BASEGAME );
	}

	NET_Shutdown( );

	Sys_PlatformExit( );

	exit( exitCode );
}

/*
=================
Sys_Quit
=================
*/
void Sys_Quit( void )
{
	Sys_Exit( 0 );
}

/*
=================
Sys_GetProcessorFeatures
=================
*/
cpuFeatures_t Sys_GetProcessorFeatures( void )
{
	cpuFeatures_t features = 0;

#ifndef DEDICATED
	if( SDL_HasRDTSC( ) )	features |= CF_RDTSC;
	if( SDL_Has3DNow( ) )	features |= CF_3DNOW;
	if( SDL_HasMMX( ) )	features |= CF_MMX;
	if( SDL_HasSSE( ) )	features |= CF_SSE;
	if( SDL_HasSSE2( ) )	features |= CF_SSE2;
	if( SDL_HasAltiVec( ) )	features |= CF_ALTIVEC;
#endif

	return features;
}

/*
=================
Sys_Init
=================
*/
void Sys_Init(void)
{
	Cmd_AddCommand( "in_restart", Sys_In_Restart_f );
	Cvar_Set( "arch", OS_STRING " " ARCH_STRING );
	Cvar_Set( "username", Sys_GetCurrentUser( ) );
	com_ansiColor = Cvar_Get( "com_ansiColor", "0", CVAR_ARCHIVE );
}

/*
=================
Sys_AnsiColorPrint

Transform Q3 colour codes to ANSI escape sequences
=================
*/
void Sys_AnsiColorPrint( const char *msg )
{
	static char buffer[ MAXPRINTMSG ];
	int         length = 0;
	static int  q3ToAnsi[ 8 ] =
	{
		30, // COLOR_BLACK
		31, // COLOR_RED
		32, // COLOR_GREEN
		33, // COLOR_YELLOW
		34, // COLOR_BLUE
		36, // COLOR_CYAN
		35, // COLOR_MAGENTA
		0   // COLOR_WHITE
	};

	while( *msg )
	{
		if( Q_IsColorString( msg ) || *msg == '\n' )
		{
			// First empty the buffer
			if( length > 0 )
			{
				buffer[ length ] = '\0';
				fputs( buffer, stderr );
				length = 0;
			}

			if( *msg == '\n' )
			{
				// Issue a reset and then the newline
				fputs( "\033[0m\n", stderr );
				msg++;
			}
			else
			{
				// Print the color code
				Com_sprintf( buffer, sizeof( buffer ), "\033[%dm",
						q3ToAnsi[ ColorIndex( *( msg + 1 ) ) ] );
				fputs( buffer, stderr );
				msg += 2;
			}
		}
		else
		{
			if( length >= MAXPRINTMSG - 1 )
				break;

			buffer[ length ] = *msg;
			length++;
			msg++;
		}
	}

	// Empty anything still left in the buffer
	if( length > 0 )
	{
		buffer[ length ] = '\0';
		fputs( buffer, stderr );
	}
}

/*
============
COM_CompareExtension

string compare the end of the strings and return qtrue if strings match
============
*/
qboolean COM_CompareExtension(const char *in, const char *ext)
{
	int inlen, extlen;

	inlen = strlen(in);
	extlen = strlen(ext);

	if(extlen <= inlen)
	{
		in += inlen - extlen;

		if(!Q_stricmp(in, ext))
			return qtrue;
	}

	return qfalse;
}

/*
=================
Sys_Print
=================
*/
void Sys_Print( const char *msg )
{
	CON_LogWrite( msg );
	CON_Print( msg );
}

/*
=================
Sys_Error
=================
*/
void Sys_Error( const char *error, ... )
{
	va_list argptr;
	char    string[1024];

	va_start (argptr,error);
	Q_vsnprintf (string, sizeof(string), error, argptr);
	va_end (argptr);

	Sys_ErrorDialog( string );

	Sys_Exit( 3 );
}

#if 0
/*
=================
Sys_Warn
=================
*/
static __attribute__ ((format (printf, 1, 2))) void Sys_Warn( char *warning, ... )
{
	va_list argptr;
	char    string[1024];

	va_start (argptr,warning);
	Q_vsnprintf (string, sizeof(string), warning, argptr);
	va_end (argptr);

	CON_Print( va( "Warning: %s", string ) );
}
#endif

/*
============
Sys_FileTime

returns -1 if not present
============
*/
int Sys_FileTime( char *path )
{
	struct stat buf;

	if (stat (path,&buf) == -1)
		return -1;

	return buf.st_mtime;
}

/*
=================
Sys_UnloadDll
=================
*/
void Sys_UnloadDll( void *dllHandle )
{
	if( !dllHandle )
	{
		Com_Printf("Sys_UnloadDll(NULL)\n");
		return;
	}

	Sys_UnloadLibrary(dllHandle);
}

#ifdef __EMSCRIPTEN__
/*
=================
Sys_LoadDll

Emscripten: resolve statically linked game modules
=================
*/
typedef intptr_t (QDECL *vmMainProc)(int callNum, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11);

#ifndef DEDICATED
extern void dllEntry_cgame(intptr_t (QDECL *)(intptr_t, ...));
extern intptr_t QDECL vmMain_cgame(int, int, int, int, int, int, int, int, int, int, int, int, int);
#endif
extern void dllEntry_qagame(intptr_t (QDECL *)(intptr_t, ...));
extern intptr_t QDECL vmMain_qagame(int, int, int, int, int, int, int, int, int, int, int, int, int);
#ifndef DEDICATED
extern void dllEntry_ui(intptr_t (QDECL *)(intptr_t, ...));
extern intptr_t QDECL vmMain_ui(int, int, int, int, int, int, int, int, int, int, int, int, int);
#endif

void * QDECL Sys_LoadDll( const char *name, char *fqpath,
	vmMainProc *entryPoint,
	intptr_t (QDECL *systemcalls)(intptr_t, ...) )
{
	*fqpath = 0;

#ifndef DEDICATED
	if (strstr(name, "cgame")) {
		dllEntry_cgame(systemcalls);
		*entryPoint = vmMain_cgame;
	} else
#endif
	if (strstr(name, "qagame")) {
		dllEntry_qagame(systemcalls);
		*entryPoint = vmMain_qagame;
	}
#ifndef DEDICATED
	else if (strstr(name, "ui")) {
		dllEntry_ui(systemcalls);
		*entryPoint = vmMain_ui;
	}
#endif
	else {
		return NULL;
	}
	Com_Printf("Sys_LoadDll(%s) resolved statically\n", name);
	return (void *)0x1;
}
#else
/*
=================
Sys_LoadDll

Used to load a development dll instead of a virtual machine
fqpath param added 2/15/02 by T.Ray - Sys_LoadDll is only called in vm.c at this time
=================
*/
void * QDECL Sys_LoadDll( const char *name, char *fqpath,
	intptr_t (QDECL **entryPoint)(int, ...),
	intptr_t (QDECL *systemcalls)(intptr_t, ...) )
{
	void *libHandle;
	void ( *dllEntry )( intptr_t ( *syscallptr )( intptr_t, ... ) );
	char fname[MAX_OSPATH];
	char *basepath;
	char *homepath;
	char *gamedir;
	char *fn;

	*fqpath = 0;

	assert( name );

	Q_strncpyz( fname, Sys_GetDLLName( name ), sizeof( fname ) );

	basepath = Cvar_VariableString( "fs_basepath" );
	homepath = Cvar_VariableString( "fs_homepath" );
	gamedir = Cvar_VariableString( "fs_game" );

	// try homepath first
	fn = FS_BuildOSPath( homepath, gamedir, fname );
	Com_Printf( "Sys_LoadDll(%s)... ", fn );
	libHandle = Sys_LoadLibrary( fn );

	if ( !libHandle ) {
		Com_Printf( "\nSys_LoadDll(%s) failed:\n\"%s\"\n", fn, Sys_LibraryError() );

		// try basepath
		fn = FS_BuildOSPath( basepath, gamedir, fname );
		Com_Printf( "Sys_LoadDll(%s)... ", fn );
		libHandle = Sys_LoadLibrary( fn );
		if ( !libHandle ) {
			Com_Printf( "\nSys_LoadDll(%s) failed:\n\"%s\"\n", fn, Sys_LibraryError() );
		} else {
			Com_Printf( "ok\n" );
		}

		// not found, bail
		if ( !libHandle ) {
			Com_Printf( "Sys_LoadDll(%s) failed dlopen() completely!\n", name );
			return NULL;
		}
	} else {
		Com_Printf( "ok\n" );
	}

	Q_strncpyz( fqpath, fn, MAX_QPATH );

	dllEntry = ( void ( * )( intptr_t ( * )( intptr_t, ... ) ) )Sys_LoadFunction( libHandle, "dllEntry" );
	*entryPoint = ( intptr_t ( QDECL * )( int, ... ) )Sys_LoadFunction( libHandle, "vmMain" );

	if ( !*entryPoint || !dllEntry ) {
		Com_Printf( "Sys_LoadDll(%s) failed to find vmMain function:\n\"%s\" !\n", name, Sys_LibraryError() );
		Sys_UnloadLibrary( libHandle );
		return NULL;
	}

	Com_Printf( "Sys_LoadDll(%s) found **vmMain** at %p\n", name, *entryPoint );
	dllEntry( systemcalls );
	Com_Printf( "Sys_LoadDll(%s) succeeded!\n", name );

	return libHandle;
}
#endif /* !__EMSCRIPTEN__ */

/*
=================
Sys_ParseArgs
=================
*/
void Sys_ParseArgs( int argc, char **argv )
{
	if( argc == 2 )
	{
		if( !strcmp( argv[1], "--version" ) ||
				!strcmp( argv[1], "-v" ) )
		{
#ifdef DEDICATED
			fprintf( stdout, Q3_VERSION " dedicated server (%s)\n", __DATE__ );
#else
			fprintf( stdout, Q3_VERSION " client (%s)\n", __DATE__ );
#endif
			Sys_Exit( 0 );
		}
	}
}

#ifndef DEFAULT_BASEDIR
#	ifdef __EMSCRIPTEN__
#		define DEFAULT_BASEDIR "."
#	elif defined(__APPLE__)
#		define DEFAULT_BASEDIR Sys_StripAppBundle(Sys_BinaryPath())
#	else
#		define DEFAULT_BASEDIR Sys_BinaryPath()
#	endif
#endif

/*
=================
Sys_SigHandler
=================
*/
void Sys_SigHandler( int signal )
{
	static qboolean signalcaught = qfalse;

	if( signalcaught )
	{
		fprintf( stderr, "DOUBLE SIGNAL FAULT: Received signal %d, exiting...\n",
			signal );
	}
	else
	{
		char *msg = va( "Received signal %d", signal );

		signalcaught = qtrue;
		fprintf( stderr, "%s, shutting down...\n", msg );
#ifndef DEDICATED
		CL_Shutdown();
#endif
		SV_Shutdown( msg );
	}

	if( signal == SIGTERM || signal == SIGINT )
		Sys_Exit( 1 );
	else
		Sys_Exit( 2 );
}

/*
=================
ET stub functions
=================
*/
qboolean Sys_IsNumLockDown( void )
{
	return qfalse;
}

void Sys_DisplaySystemConsole( qboolean show )
{
}

void Sys_SetErrorText( const char *text )
{
}

void Sys_ShowConsole( int level, qboolean quitOnClose )
{
}

void Sys_BeginStreamedFile( fileHandle_t f, int readAhead )
{
}

void Sys_EndStreamedFile( fileHandle_t f )
{
}

int Sys_StreamedRead( void *buffer, int size, int count, fileHandle_t f )
{
	return FS_Read( buffer, size * count, f );
}

void Sys_StreamSeek( fileHandle_t f, int offset, int origin )
{
	FS_Seek( f, offset, origin );
}

int Sys_GetProcessorId( void )
{
	return 0;
}

unsigned int Sys_ProcessorCount( void )
{
	return 1;
}

float Sys_GetCPUSpeed( void )
{
	return 0;
}

qboolean Sys_CheckCD( void )
{
	return qtrue;
}

void *Sys_InitializeCriticalSection( void )
{
	return NULL;
}

void Sys_EnterCriticalSection( void *ptr )
{
}

void Sys_LeaveCriticalSection( void *ptr )
{
}

/*
========================================================================

EVENT LOOP

========================================================================
*/

#define MAX_QUED_EVENTS     256
#define MASK_QUED_EVENTS    ( MAX_QUED_EVENTS - 1 )

static sysEvent_t eventQue[MAX_QUED_EVENTS];
static int eventHead = 0;
static int eventTail = 0;
static byte sys_packetReceived[MAX_MSGLEN];

/*
================
Sys_QueEvent

A time of 0 will get the current time
Ptr should either be null, or point to a block of data that can
be freed by the game later.
================
*/
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr )
{
	sysEvent_t *ev;

	ev = &eventQue[ eventHead & MASK_QUED_EVENTS ];

	if ( eventHead - eventTail >= MAX_QUED_EVENTS ) {
		Com_Printf( "Sys_QueEvent: overflow\n" );
		if ( ev->evPtr ) {
			Z_Free( ev->evPtr );
		}
		eventTail++;
	}

	eventHead++;

	if ( time == 0 ) {
		time = Sys_Milliseconds();
	}

	ev->evTime = time;
	ev->evType = type;
	ev->evValue = value;
	ev->evValue2 = value2;
	ev->evPtrLength = ptrLength;
	ev->evPtr = ptr;
}

/*
================
Sys_GetEvent
================
*/
sysEvent_t Sys_GetEvent( void )
{
	sysEvent_t ev;
	char *s;
	msg_t netmsg;
	netadr_t adr;

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// pump the message loop
	Sys_SendKeyEvents();

	// check for console commands
	s = Sys_ConsoleInput();
	if ( s ) {
		char *b;
		int len;

		len = strlen( s ) + 1;
		b = Z_Malloc( len );
		strcpy( b, s );
		Sys_QueEvent( 0, SE_CONSOLE, 0, 0, len, b );
	}

	// check for network packets
	MSG_Init( &netmsg, sys_packetReceived, sizeof( sys_packetReceived ) );
	if ( Sys_GetPacket( &adr, &netmsg ) ) {
		netadr_t *buf;
		int len;

		// copy out to a separate buffer for queuing
		len = sizeof( netadr_t ) + netmsg.cursize;
		buf = Z_Malloc( len );
		*buf = adr;
		memcpy( buf + 1, netmsg.data, netmsg.cursize );
		Sys_QueEvent( 0, SE_PACKET, 0, 0, len, buf );
	}

	// return if we have data
	if ( eventHead > eventTail ) {
		eventTail++;
		return eventQue[ ( eventTail - 1 ) & MASK_QUED_EVENTS ];
	}

	// create an empty event to return
	memset( &ev, 0, sizeof( ev ) );
	ev.evTime = Sys_Milliseconds();

	return ev;
}

/*
================
Sys_SendKeyEvents

Platform-specific key event pump (called by Sys_GetEvent).
Input processing is handled by IN_Frame, so this is a no-op.
Dedicated server uses the definition in null_input.c instead.
================
*/
#ifndef DEDICATED
void Sys_SendKeyEvents( void )
{
}
#endif

/*
================
Sys_SnapVector
================
*/
void Sys_SnapVector( float *v )
{
	v[0] = rint(v[0]);
	v[1] = rint(v[1]);
	v[2] = rint(v[2]);
}

/*
================
Sys_BeginProfiling
================
*/
void Sys_BeginProfiling( void )
{
}

/*
=================
main
=================
*/
int main( int argc, char **argv )
{
	int   i;
	char  commandLine[ MAX_STRING_CHARS ] = { 0 };

#ifndef DEDICATED
	// SDL version check

	// Compile time
#	if !SDL_VERSION_ATLEAST(MINSDL_MAJOR,MINSDL_MINOR,MINSDL_PATCH)
#		error A more recent version of SDL is required
#	endif

	// Run time
	SDL_version ver;
	SDL_GetVersion( &ver );

#define MINSDL_VERSION \
	XSTRING(MINSDL_MAJOR) "." \
	XSTRING(MINSDL_MINOR) "." \
	XSTRING(MINSDL_PATCH)

	if( SDL_VERSIONNUM( ver.major, ver.minor, ver.patch ) <
			SDL_VERSIONNUM( MINSDL_MAJOR, MINSDL_MINOR, MINSDL_PATCH ) )
	{
		Sys_Dialog( DT_ERROR, va( "SDL version " MINSDL_VERSION " or greater is required, "
			"but only version %d.%d.%d was found. You may be able to obtain a more recent copy "
			"from http://www.libsdl.org/.", ver.major, ver.minor, ver.patch ), "SDL Library Too Old" );

		Sys_Exit( 1 );
	}
#endif

	Sys_PlatformInit( );

	// Set the initial time base
	Sys_Milliseconds( );

#ifdef __APPLE__
	// This is passed if we are launched by double-clicking
	if ( argc >= 2 && Q_strncmp ( argv[1], "-psn", 4 ) == 0 )
		argc = 1;
#endif

	Sys_ParseArgs( argc, argv );
	Sys_SetBinaryPath( Sys_Dirname( argv[ 0 ] ) );
	Sys_SetDefaultInstallPath( DEFAULT_BASEDIR );

	// Concatenate the command line for passing to Com_Init
	for( i = 1; i < argc; i++ )
	{
		const qboolean containsSpaces = strchr(argv[i], ' ') != NULL;
		if (containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), argv[ i ] );

		if (containsSpaces)
			Q_strcat( commandLine, sizeof( commandLine ), "\"" );

		Q_strcat( commandLine, sizeof( commandLine ), " " );
	}

	CON_Init( );
	Com_Init( commandLine );
	NET_Init( );

#ifndef __EMSCRIPTEN__
	signal( SIGILL, Sys_SigHandler );
	signal( SIGFPE, Sys_SigHandler );
	signal( SIGSEGV, Sys_SigHandler );
	signal( SIGTERM, Sys_SigHandler );
	signal( SIGINT, Sys_SigHandler );
#endif

#ifdef __EMSCRIPTEN__
#ifdef DEDICATED
	emscripten_set_main_loop(Com_Frame, 30, 1);
#else
	emscripten_set_main_loop(Com_Frame, 0, 1);
#endif
#else
	while( 1 )
	{
		Com_Frame( );
	}
#endif

	return 0;
}
