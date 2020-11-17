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
struct PL;
//-----------------------------------------------------<Timing>-----------------------------------------------------

struct PL_Timing
{
	uint64 cycles_per_second;
	
	uint64 current_cycles;
	uint64 current_micros;
	uint64 current_millis;
	uint64 current_seconds;
	f64 fcurrent_seconds;

	uint64 delta_cycles;
	uint64 delta_micros;
	uint64 delta_millis;

	f32 fdelta_seconds;
};
void PL_poll_timing(PL& pl);
void PL_initialize_timing(PL& pl);
//-----------------------------------------------------</Timing>----------------------------------------------------

//-----------------------------------------------------<Input>------------------------------------------------------
struct PL_Digital_Button
{
	b32 is_down;
	b32 was_released;
	b32 was_pressed;
};
struct PL_Mouse_Input
{
	b32 is_in_window;
	int32 position_x;
	int32 position_y;
	PL_Digital_Button left;
	PL_Digital_Button right;
};
struct PL_Input
{
	PL_Mouse_Input mouse;
};
void PL_poll_input(PL& pl);
void PL_initialize_input(PL& pl);
//-----------------------------------------------------</Input>-----------------------------------------------------

//-----------------------------------------------------<Audio>------------------------------------------------------
struct PL_Audio_Format
{
	uint32 no_channels;
	uint32 no_bits_per_sample;
	uint32 samples_per_second;				
	uint32 buffer_frame_count;			    //The amount of frames (frames = sample size * no of channels) in the buffer. If this is 0, time will be used to calculate and create buffer
	f32 buffer_duration_seconds;			//If this is 0, buffer_sample_size is used. if both are zero, this is 1.0(1 second).
};
 
struct PL_Audio_Output
{
	PL_Audio_Format format;
};

struct PL_Audio_Input
{
	uint32 no_of_new_frames;				//The number of incoming frames polled from last game loop
	f32* sink_buffer;						//Newest frame is at front.
	b32 is_loopback;
	b32 only_update_every_new_buffer;
	PL_Audio_Format format;
};

struct PL_Audio
{
	PL_Audio_Input input;
	PL_Audio_Output output;
};
void PL_poll_audio(PL& pl);
void PL_push_audio(PL& pl);
void PL_initialize_audio(PL& pl);
//-----------------------------------------------------</Audio>----------------------------------------------------


//-----------------------------------------------------<Bitmap>-----------------------------------------------------
struct PL_Bitmap
{
	void* buffer;
	uint32 height;
	uint32 width;
	uint32 bytes_per_pixel;
	uint32 pitch;
	uint32 size;
};
void PL_initialize_bitmap(PL& pl);
//-----------------------------------------------------</Bitmap>----------------------------------------------------

//-----------------------------------------------------<Window>-----------------------------------------------------
struct PL_Window
{
	b32 was_altered;
	int32 position_x;
	int32 position_y;
	uint32 height;
	uint32 width;
	char* title;
};
void PL_poll_window(PL& pl);
void PL_push_window(PL& pl);
void PL_initialize_window(PL& pl);
//-----------------------------------------------------</Window>----------------------------------------------------

struct PL
{
	uint32 core_count;
	b32 initialized;
	b32 running;
	PL_Input input;
	PL_Timing time;
	PL_Audio audio;
	PL_Window window;
	PL_Bitmap bitmap;
	void* platform_specific;
};

void PL_entry_point(PL& pl);
