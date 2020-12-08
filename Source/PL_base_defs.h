#pragma once

#define PL_WINDOWS

#ifdef _MSC_VER
#define PL_COMPILER_MSVC 1
#endif

#ifdef _DEBUG
#define ASSERT(x) if(!(x)) __debugbreak();
#else
#define ASSERT(X)
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
//-----------------------------------------------