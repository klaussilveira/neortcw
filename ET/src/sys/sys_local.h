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

#include "../game/q_shared.h"
#include "../qcommon/qcommon.h"

// Platform defines not present in ET's q_shared.h (from iortcw q_platform.h)
#ifndef DLL_EXT

#ifdef __EMSCRIPTEN__
#define OS_STRING "emscripten"
#ifndef ARCH_STRING
#define ARCH_STRING "wasm32"
#endif
#define DLL_EXT ".js"
#elif defined(_WIN64) || defined(__WIN64__)
#define OS_STRING "win_msvc64"
#ifndef ARCH_STRING
#define ARCH_STRING "x64"
#endif
#define DLL_EXT ".dll"
#elif defined(_WIN32) || defined(__WIN32__)
#define OS_STRING "win_msvc"
#ifndef ARCH_STRING
#define ARCH_STRING "x86"
#endif
#define DLL_EXT ".dll"
#elif defined(__APPLE__) || defined(__APPLE_CC__)
#define OS_STRING "macosx"
#ifndef ARCH_STRING
#ifdef __ppc__
#define ARCH_STRING "ppc"
#elif defined __i386__
#define ARCH_STRING "i386"
#elif defined __x86_64__
#define ARCH_STRING "x86_64"
#elif defined __aarch64__
#define ARCH_STRING "arm64"
#endif
#endif
#define DLL_EXT ".dylib"
#elif defined(__linux__)
#define OS_STRING "linux"
#ifndef ARCH_STRING
#if defined __i386__
#define ARCH_STRING "i386"
#elif defined __x86_64__
#define ARCH_STRING "x86_64"
#elif defined __powerpc64__
#define ARCH_STRING "powerpc64"
#elif defined __powerpc__
#define ARCH_STRING "powerpc"
#elif defined __aarch64__
#define ARCH_STRING "arm64"
#elif defined __s390x__
#define ARCH_STRING "s390x"
#endif
#endif
#define DLL_EXT ".so"
#elif defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__)
#if defined(__FreeBSD__)
#define OS_STRING "freebsd"
#elif defined(__OpenBSD__)
#define OS_STRING "openbsd"
#elif defined(__NetBSD__)
#define OS_STRING "netbsd"
#endif
#ifndef ARCH_STRING
#ifdef __i386__
#define ARCH_STRING "i386"
#elif defined __amd64__
#define ARCH_STRING "amd64"
#elif defined __x86_64__
#define ARCH_STRING "x86_64"
#endif
#endif
#define DLL_EXT ".so"
#endif

#endif // DLL_EXT

// Utility macros not present in ET's q_shared.h
#ifndef ARRAY_LEN
#define ARRAY_LEN(x)	(sizeof(x) / sizeof(*(x)))
#endif

#ifndef UNUSED_VAR
#ifdef __GNUC__
#define UNUSED_VAR __attribute__((unused))
#else
#define UNUSED_VAR
#endif
#endif

#ifndef STRING
#define STRING(s)		#s
#endif

#ifndef XSTRING
#define XSTRING(s)		STRING(s)
#endif

// Dialog types (not in ET's qcommon.h)
typedef enum
{
	DR_YES = 0,
	DR_NO = 1,
	DR_OK = 0,
	DR_CANCEL = 1
} dialogResult_t;

typedef enum
{
	DT_INFO,
	DT_WARNING,
	DT_ERROR,
	DT_YES_NO,
	DT_OK_CANCEL
} dialogType_t;

dialogResult_t Sys_Dialog( dialogType_t type, const char *message, const char *title );

// Event queue (declared in platform-specific files: unix_main.c, win_main.c)
void Sys_QueEvent( int time, sysEventType_t type, int value, int value2, int ptrLength, void *ptr );

// CPU feature flags (not in ET's qcommon.h)
#ifndef CF_RDTSC
typedef enum
{
	CF_RDTSC      = 1 << 0,
	CF_MMX        = 1 << 1,
	CF_MMX_EXT    = 1 << 2,
	CF_3DNOW      = 1 << 3,
	CF_3DNOW_EXT  = 1 << 4,
	CF_SSE        = 1 << 5,
	CF_SSE2       = 1 << 6,
	CF_ALTIVEC    = 1 << 7
} cpuFeatures_t;
#endif

// DLL extension check (not in ET's q_shared.h)
qboolean COM_CompareExtension( const char *in, const char *ext );
qboolean Sys_DllExtension( const char *name );

#ifndef DEDICATED
#ifdef USE_LOCAL_HEADERS
#	include "SDL_version.h"
#else
#	include <SDL_version.h>
#endif

// Require a minimum version of SDL
#define MINSDL_MAJOR 2
#define MINSDL_MINOR 0
#if SDL_VERSION_ATLEAST( 2, 0, 5 )
#define MINSDL_PATCH 5
#else
#define MINSDL_PATCH 0
#endif
#endif

// Console
void CON_Shutdown( void );
void CON_Init( void );
char *CON_Input( void );
void CON_Print( const char *message );

unsigned int CON_LogSize( void );
unsigned int CON_LogWrite( const char *in );
unsigned int CON_LogRead( char *out, unsigned int outSize );

#ifdef __APPLE__
char *Sys_StripAppBundle( char *pwd );
#endif

const char *Sys_Basename( char *path );
const char *Sys_Dirname( char *path );

void Sys_GLimpSafeInit( void );
void Sys_GLimpInit( void );
void Sys_PlatformInit( void );
void Sys_PlatformExit( void );
void Sys_SigHandler( int signal ) __attribute__ ((noreturn));
void Sys_ErrorDialog( const char *error );
void Sys_AnsiColorPrint( const char *msg );

int Sys_PID( void );
qboolean Sys_PIDIsRunning( int pid );

// ET-specific declarations
char *Sys_GetDLLName( const char *name );
void Sys_Chmod( char *file, int mode );
void *Sys_InitializeCriticalSection( void );
void Sys_EnterCriticalSection( void *ptr );
void Sys_LeaveCriticalSection( void *ptr );

void Sys_RemovePIDFile( const char *gamedir );
void Sys_InitPIDFile( const char *gamedir );

// Input (SDL platform layer)
struct SDL_Window;
void IN_Init( struct SDL_Window *windowData );
void IN_Shutdown( void );
void IN_Frame( void );
void IN_Restart( void );

// com_ansiColor cvar for console color output (not in ET's qcommon.h)
extern cvar_t *com_ansiColor;
