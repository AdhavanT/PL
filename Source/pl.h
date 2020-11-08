#pragma once


#ifdef _DEBUG
#define ASSERT(x) if(!(x)) __debugbreak();
#else
#define ASSERT(X)
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
#define MAX_FLOAT          3.402823466e+38F        // max value
#define MIN_FLOAT          1.175494351e-38F        // min normalized positive value
#define UINT32MAX		   0xffffffff			
#define INV_UINT32_MAX	   2.328306437e-10F


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



struct PL_Bitmap
{
	void* buffer;
	int height;
	int width;
	int bytes_per_pixel;
	int pitch;
	uint32 size;
};

struct PL_Window
{
	b32 was_resized;
	int position_x;
	int position_y;
	int height;
	int width;
	char* title;
};

struct PL
{
	b32 initialized;
	b32 running;
	PL_Window window;
	PL_Bitmap bitmap;
	void* platform_specific;
};

void PL_initialize(PL &pl); 
void PL_poll(PL &pl);
void PL_update(PL &pl);
void PL_push( PL &pl);