#include "pl.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>

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
	PL_push_audio(pl);
	PL_push_window(pl);
}

inline void draw_rectangle_from_point(uint32 from_x, uint32 from_y, uint32 to_x, uint32 to_y, PL pl, uint8 r, uint8 g, uint8 b)
{
	int32 width = to_x - from_x;
	int32 height = to_y - from_y;
	uint32 color = (uint32)r << 16 | (uint32)g << 8 | (uint32)b << 0;
	uint32* ptr = (uint32*)pl.bitmap.buffer + (from_y * pl.bitmap.width) + from_x;
	int32 sign_height = (height > 0) ? 1 : -1;
	int32 sign_width = (width > 0) ? 1 : -1;

	for (int y = 0; y < (sign_height * height); y++)
	{
		for (int x = 0; x < (sign_width * width); x++)
		{
			*ptr = color;
			ptr+= sign_width;
		}
		ptr -= width;
		ptr += sign_height * pl.bitmap.width;
	}
}

inline void draw_verticle_line_from_point( uint32 x,uint32 y, int32 height, PL pl, uint8 r, uint8 g, uint8 b)
{
	uint32 color = (uint32)r << 16 | (uint32)g << 8 | (uint32)b << 0;
	uint32* ptr = (uint32*)pl.bitmap.buffer + (y * pl.bitmap.width) + x;
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
	//clear_buffer(pl);	//memset if faster. 
	if (pl.audio.input.no_of_new_frames == 0)
	{
		return;
	}
	memset(pl.bitmap.buffer, 33,4 * pl.bitmap.height * pl.bitmap.width);	
	
	static f64 timing_refresh = 0;
	//if (pl.time.fcurrent_seconds - timing_refresh > 0.1)//refreshing at a tenth(0.1) of a second.
	{
		//int32 frame_rate = (int32)(pl.time.cycles_per_second / pl.time.delta_cycles);
		timing_refresh = pl.time.fcurrent_seconds - timing_refresh;
		char buffer[256];
		sprintf_s(buffer, "Time per sample: %.*fms \n", 2, timing_refresh * 1000);
		OutputDebugStringA(buffer);
		timing_refresh = pl.time.fcurrent_seconds;
	}

	uint8 red, green, blue;
	f32 volume = 0;
	for (uint32 i = 0; i < pl.audio.input.no_of_new_frames; i++)
	{
		volume += pl.audio.input.sink_buffer[i] * pl.audio.input.sink_buffer[i];
	}
	volume = volume / (f32)pl.audio.input.no_of_new_frames;
	volume = sqrtf(volume);
	blue = ((volume * (f32)255) <= (f32)255) ? (uint8)((f32)volume * 255.f) : 255;
	char msg[512];
	//wsprintfA(msg, "Mouse: pos_x:%i , pos_y:%i \n", pl.input.mouse.position_x, pl.input.mouse.position_y);
	wsprintfA(msg, "New_frames: %i \n", pl.audio.input.no_of_new_frames);

	OutputDebugStringA(msg);
	////draw_rectangle_from_point(pl.bitmap.width/2, pl.bitmap.height/2, pl.input.mouse.position_x,pl.input.mouse.position_y, pl, 255, 0, 0);

	for (uint32 i = 0; i < pl.bitmap.width; i++)
	{
		//2 channel graph
		//f32 sample_pos = i / ((f32)pl.bitmap.width - 1.f);
		//int32 left_audio_pos = (int32)(sample_pos * (f32)pl.audio.input.format.buffer_frame_count);
		//if (left_audio_pos % 2 != 0)
		//{
		//	left_audio_pos++;
		//}
		////int32 audio_pos = (int32)(sample_pos * (f32)pl.audio.input.format.buffer_frame_count);
		//f32 left_height = pl.audio.input.sink_buffer[left_audio_pos] * ((pl.bitmap.height - 10)/4.f);
		//f32 right_height = pl.audio.input.sink_buffer[left_audio_pos + 1] * ((pl.bitmap.height - 10) / 4.f);
		//draw_verticle_line_from_point(i, pl.bitmap.height / 4, (int32)left_height, pl, 0, 255, 0);
		//draw_verticle_line_from_point(i, (pl.bitmap.height*3)/4,(int32)right_height, pl, 0, 0, 255);

		//1 channel graph
		f32 sample_pos = i / ((f32)pl.bitmap.width - 1.f);
		int32 audio_pos = (int32)(sample_pos * (f32)pl.audio.input.format.buffer_frame_count);
		f32 audio_height = pl.audio.input.sink_buffer[audio_pos] * ((pl.bitmap.height - 10) / 2.0f);
		draw_verticle_line_from_point(i, (pl.bitmap.height / 2), (int32)audio_height, pl, 0, 0, 244);

	}
	f32 added_pos = (f32)(pl.audio.input.format.buffer_frame_count - pl.audio.input.no_of_new_frames) / (f32)pl.audio.input.format.buffer_frame_count;
	int32 added_pos_i = (int32)(added_pos * (f32)(pl.bitmap.width));
	draw_verticle_line_from_point(added_pos_i, (pl.bitmap.height / 2), (pl.bitmap.height - 10) / 2.0f, pl, 255, 0, 0);
	/*f64 new_time; 
	LARGE_INTEGER new_t;
	QueryPerformanceCounter(&new_t);
	new_time = new_t.QuadPart / (f64)pl.time.cycles_per_second;
	while (new_time < (pl.time.fcurrent_seconds + 0.0159))
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
	pl.audio.input.only_update_every_new_buffer = FALSE;
	pl.audio.input.format.no_channels = 1;
	pl.audio.input.format.samples_per_second = 44100;
	pl.audio.input.format.no_bits_per_sample = 16;
	pl.audio.input.format.buffer_duration_seconds = 1.f / 30.f;
	pl.audio.input.is_loopback = TRUE;
	pl.window.height = 720;
	pl.window.width = 1280;
	PL_initialize(pl);
	while (pl.running)
	{
		PL_poll(pl);
		PL_update(pl);
		PL_push(pl);
	}
}
