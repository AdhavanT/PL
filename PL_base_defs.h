
//---------------------------------------<Base Definitions>------------------------------------------------------------
#pragma once

#define PL_WINDOWS
#define PL_INTERNAL

#ifdef _MSC_VER
#define PL_COMPILER_MSVC 1
#endif

#define ERRORBOX(error) {pl_throw_error_box(error); __debugbreak();}

#ifdef _DEBUG
#define ASSERT(x) if(!(x)) __debugbreak();
#else
#define ASSERT(X)
#endif 

#ifdef PL_COMPILER_MSVC
#ifdef _M_X64
#define PL_X64
#else
#ifdef _M_IX86
#define PL_X86
#endif
#endif
#endif

#ifdef PL_COMPILER_MSVC 
#define FORCEDINLINE __forceinline
#else
#define FORCEDINLINE __attribute__((always_inline))
#endif

#ifndef TRUE
#define TRUE 1
#else
#undef TRUE 
#define TRUE 1
#endif 

#ifndef FALSE
#define FALSE 0
#else
#undef FALSE 
#define FALSE 0
#endif 

//-----------------------------------------------

typedef signed char        int8;
typedef short              int16;
typedef int                int32;
typedef long long          int64;
typedef unsigned char      uint8;
typedef unsigned short     uint16;
typedef unsigned int       uint32;
typedef unsigned long long uint64;

typedef bool b8;
typedef int b32;

typedef float f32;
typedef double f64;

#define Kilobytes(n)  (n << 10)
#define Megabytes(n)  (n << 20)
#define Gigabytes(n)  (((uint64)n) << 30)
#define Terabytes(n)  (((uint64)n) << 40)

#define MAX_FLOAT          3.402823466e+38F        // max value
#define MIN_FLOAT          1.175494351e-38F        // min normalized positive value
#define UINT32MAX		   0xffffffff			
#define INV_UINT32_MAX	   2.328306437e-10F

#define ArrayCount(array) (sizeof(array) / sizeof(array[0]))

//---------------------------------------</Base Definitions>------------------------------------------------------------


//----------------------------------------------------<Config>-----------------------------------------------------------------
//-----------------------------------------------------<Window>-----------------------------------------------------
//------------------------------------------<Render types>----------------------------
#define PL_BLIT_BITMAP 1
#define PL_OPENGL 2
//------------------------------------------</Render types>---------------------------
#define PL_WINDOW_RENDERTYPE PL_BLIT_BITMAP	//Set to the render type needed for the window 


//-----------------------------------------------------</Window>----------------------------------------------------

//-----------------------------------------------------<Memory>----------------------------------------------------
#define PL_DO_MEMORY_ARENA_ADDON
#ifdef PL_INTERNAL
#define MONITOR_ARENA_USAGE
#endif

#ifdef MONITOR_ARENA_USAGE
#define ARENA_MONITOR_CHECK_FOR_POPS
#define ARENAOWNERLIST_CAPACITY 1000
#endif
//-----------------------------------------------------</Memory>----------------------------------------------------

//-----------------------------------------------------<Input>------------------------------------------------------
#define PL_INPUT_KEYBOARD_MAX_KEYS 255
//-----------------------------------------------------</Input>-----------------------------------------------------

//----------------------------------------------------</Config>-----------------------------------------------------------------
