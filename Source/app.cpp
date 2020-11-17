#include "pl.h"
#include <Windows.h>
#include <stdio.h>
#include<math.h>
void PL_initialize(PL& pl)
{
	pl.running = TRUE;
	pl.initialized = FALSE;
	PL_initialize_timing(pl);
	PL_initialize_audio(pl);
	PL_initialize_window(pl);
	PL_initialize_bitmap(pl);
	pl.initialized = TRUE;
}

void PL_poll(PL& pl)
{
	PL_poll_audio(pl);
	PL_poll_window(pl);
	PL_poll_timing(pl);
}

void PL_push(PL& pl)
{
	PL_push_window(pl);
}

inline void draw_verticle_line_from_center(uint32 x, int32 height, PL& pl, uint8 r, uint8 g, uint8 b)
{
	uint32 color = (uint32)r << 16 | (uint32)g << 8 | (uint32)b << 0;
	uint32* ptr = (uint32*)pl.bitmap.buffer + (pl.bitmap.height/2 * pl.bitmap.width) + x;
	if (height >= 0)
	{
		for (int y = 0; y < height; y++)
		{
			*ptr = color;
			ptr += pl.bitmap.width;
		}
	}
	else
	{
		for (int y = 0; y < -height; y++)
		{
			*ptr = color;
			ptr -= pl.bitmap.width;
		}
	}
}

void PL_update(PL& pl)
{
	memset(pl.bitmap.buffer, 255,4 * pl.bitmap.height * pl.bitmap.width);	
	
	for (uint32 i = 0; i < pl.bitmap.width; i++)
	{
		f32 sample_pos = i / (f32)pl.bitmap.width;
		int32 audio_pos = (int32)(sample_pos * (f32)pl.audio.input.format.buffer_frame_count);
		f32 height = pl.audio.input.sink_buffer[audio_pos] * 400;
		draw_verticle_line_from_center(i, height, pl, 0, 0, 0);
	}
	/*f64 new_time; 
	LARGE_INTEGER new_t;
	QueryPerformanceCounter(&new_t);
	new_time = new_t.QuadPart / (f64)pl.time.cycles_per_second;
	while (new_time < (pl.time.fcurrent_seconds + 0.016))
	{
		LARGE_INTEGER new_t;
		QueryPerformanceCounter(&new_t);
		new_time = new_t.QuadPart / (f64)pl.time.cycles_per_second;
	}*/
	//uint8 color = 0;
	//f32 volume = 0;
	//for (uint32 i = 0; i < pl.audio.input.no_of_new_frames; i++)
	//{
	//	volume += (pl.audio.input.sink_buffer[i]);// * (pl.audio.input.sink_buffer[i]);
	//}
	//volume = volume / (f32)pl.audio.input.format.buffer_frame_count;
	//volume = sqrtf(volume);
	//color = (uint8)(volume * 255.f);
	//uint32 *ptr = (uint32*)pl.bitmap.buffer;
	//for (uint32 y = 0; y < pl.bitmap.height; y++)
	//{
	//	for (uint32 x = 0; x < pl.bitmap.width; x++)
	//	{
	//		*((uint8*)ptr) = color;
	//		ptr++;
	//	}
	//}

}

void PL_entry_point(PL& pl)
{

	pl.audio.input.format =
	{
		1,16,44100,0, 0
	};
	pl.audio.input.format.buffer_duration_seconds = 1.f / 30.f;
	pl.audio.input.is_loopback = TRUE;
	pl.window.height = 1000;
	pl.window.width = 1000;
	PL_initialize(pl);
	while (pl.running)
	{
		PL_poll(pl);
		PL_update(pl);
		PL_push(pl);
	}
}
