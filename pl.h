#pragma once

//-----------------------------------------------
#include "pl_base_defs.h"
struct PL;
typedef void(*PL_Function)(PL& pl);	//used for functions that are different for different modes. Ex: PL_push_window() is different for opengl and for bit-blitting using a bitmap. 

//-----------------------------------------------------<functionality>----------------------------------------------
struct PL_Window;
struct PL_Timing;
struct PL_Input_Gamepad;
struct PL_Input_Keyboard;
struct PL_Input_Mouse;
struct PL_Audio_Input;
struct PL_Audio_Output;

//-----------------------------------------------------</functionality>---------------------------------------------
//-----------------------------------------------------<Memory>----------------------------------------------------
#define PL_MEMORY_ARENA_DEF_ONLY
#include "pl_memory_arena.h"
#undef PL_MEMORY_ARENA_DEF_ONLY
//-----------------------------------------------------</Memory>----------------------------------------------------


//-----------------------------------------------------<Window>-----------------------------------------------------

struct PL_Window
{
#if PL_WINDOW_RENDERTYPE == PL_BLIT_BITMAP
	struct PL_Window_Bitmap_Format
	{
		void* buffer;
		uint32 height;
		uint32 width;
		uint32 bytes_per_pixel;
		uint32 pitch;
		uint32 size;
	};
	PL_Window_Bitmap_Format window_bitmap;
#elif PL_WINDOW_RENDERTYPE == PL_OPENGL
	//opengl context stuff
#endif
	b32 user_resizable;
	b32 was_altered;
	int32 position_x;
	int32 position_y;
	uint32 height;
	uint32 width;
	char* title;
};
void PL_poll_window(PL_Window& pl);
void PL_push_window(PL_Window& pl, b32 refresh_window_title);
void PL_initialize_window(PL_Window& pl);
void PL_initialize_window(PL_Window& window, MArena* arena);
void PL_cleanup_window(PL_Window& pl);
void PL_cleanup_window(PL_Window& window, MArena* arena);

//-----------------------------------------------------</Window>----------------------------------------------------

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
void PL_poll_timing(PL_Timing& pl);
void PL_initialize_timing(PL_Timing& pl);
//-----------------------------------------------------</Timing>----------------------------------------------------

//-----------------------------------------------------<Input>------------------------------------------------------
struct PL_Digital_Button
{
	b32 down;		//is currently down
	b32 released;	//was just released 
	b32 pressed;	//was just pressed down
};
struct PL_Analog_Button
{
	f32 deadzone;	//value = (abs(value) < deadzone) ? 0 : value
	f32 value;	//0.0 to 1.0
	b32 down;	//
	b32 released;
	b32 pressed;
};

struct PL_Analog_Stick
{
	f32 x;	//-1.0 to 1.0
	f32 y;	//-1.0 to 1.0 
	f32 deadzone_x;		//x = (abs(x) < deadzone_x) ? 0 : x
	f32 deadzone_y;		//y = (abs(y) < deadzone_y) ? 0 : y
};

struct PL_Input_Gamepad
{
	b32 plugged_in;

	PL_Digital_Button a;
	PL_Digital_Button b;
	PL_Digital_Button x;
	PL_Digital_Button y;

	PL_Digital_Button right_bumper;
	PL_Digital_Button left_bumper;

	PL_Digital_Button start;
	PL_Digital_Button select;

	PL_Digital_Button dpad_up;
	PL_Digital_Button dpad_down;
	PL_Digital_Button dpad_right;
	PL_Digital_Button dpad_left;

	PL_Analog_Button left_trigger;
	PL_Analog_Button right_trigger;

	PL_Analog_Stick left_stick;
	PL_Analog_Stick right_stick;

};
void PL_initialize_input_gamepad(PL_Input_Gamepad& pl);
void PL_poll_input_gamepad(PL_Input_Gamepad& pl);

struct PL_Input_Mouse
{
	b32 is_in_window;
	int32 scroll_delta;
	int32 position_x;
	int32 position_y;
	PL_Digital_Button middle;
	PL_Digital_Button left;
	PL_Digital_Button right;
};
void PL_initialize_input_mouse(PL_Input_Mouse& pl);
void PL_poll_input_mouse(PL_Input_Mouse& pl, PL_Window& main_window);

enum PL_KEY
{

	NUM_0 = 48, NUM_1, NUM_2,
	NUM_3, NUM_4, NUM_5,
	NUM_6, NUM_7, NUM_8, NUM_9,

	A = 65, B, C, D, E,
	F, G, H, I, J, K, L,
	M, N, O, P, Q, R, S,
	T, U, V, W, X, Y, Z,

	F1 = 112, F2, F3, F4,
	F5, F6, F7, F8,
	F9, F10, F11, F12,

	ESCAPE = 27,

	SPACE = 32,
	SHIFT = 16,
	CTRL = 17,
	ALT = 18,
	LEFT_SHIFT = 160,
	RIGHT_SHIFT,
	LEFT_CTRL,
	RIGHT_CTRT,
	LEFT_ALT,
	RIGHT_ALT,
};
struct PL_Input_Keyboard
{
	PL_Digital_Button keys[PL_INPUT_KEYBOARD_MAX_KEYS];
};
void PL_initialize_input_keyboard(PL_Input_Keyboard& pl);
void PL_poll_input_keyboard(PL_Input_Keyboard& pl);

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
void PL_initialize_audio_render(PL_Audio_Output& pl);
void PL_push_audio_render(PL_Audio_Output& pl);		//for playing audio
void PL_cleanup_audio_render(PL_Audio_Output& pl);

struct PL_Audio_Input
{
	uint32 no_of_new_frames;				//The number of incoming frames polled from last game loop
	f32* sink_buffer;						//Newest frame is at front.
	b32 is_loopback;
	b32 only_update_every_new_buffer;
	PL_Audio_Format format;
};
void PL_initialize_audio_capture(PL_Audio_Input& pl);
void PL_initialize_audio_capture(PL_Audio_Input& pl, MArena* arena);
void PL_poll_audio_capture(PL_Audio_Input& pl);		//for audio capture
void PL_cleanup_audio_capture(PL_Audio_Input& pl);
void PL_cleanup_audio_capture(PL_Audio_Input& input, MArena* arena);

//-----------------------------------------------------</Audio>----------------------------------------------------


struct PL_Input
{
	//TODO: Gamepad input
	PL_Input_Gamepad gamepad;
	union
	{
		PL_Digital_Button keys[PL_INPUT_KEYBOARD_MAX_KEYS];
		PL_Input_Keyboard kb;
	};
	PL_Input_Mouse mouse;
};

struct PL_Audio
{
	PL_Audio_Input input;
	PL_Audio_Output output;
};

struct PL_Memory
{
	MArena main_arena;
	MArena temp_arena;
};

struct PL
{
	uint32 core_count;
	b32 initialized;
	b32 running;
	PL_Input input;
	PL_Timing time;
	PL_Audio audio;
	PL_Window window;
	PL_Memory memory;
};

void PL_entry_point(PL& pl);
