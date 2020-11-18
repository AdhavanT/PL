#include "pl.h"
#include <windows.h>
#include <math.h>
#include <stdio.h>

void PL_initialize(PL& pl)
{
	pl.running = TRUE;
	pl.initialized = FALSE;
	PL_initialize_timing(pl);
	PL_initialize_audio_capture(pl);
	PL_initialize_window(pl);
	pl.initialized = TRUE;
}

void PL_poll(PL& pl)
{
	PL_poll_audio_capture(pl);
	PL_poll_window(pl);
	PL_poll_timing(pl);
}

void PL_push(PL& pl)
{
	//PL_push_audio_render(pl);
	PL_push_window(pl);
}

void PL_cleanup(PL& pl)
{
	PL_cleanup_audio_capture(pl);
	//PL_cleanup_audio_render(pl);
	PL_cleanup_window(pl);
}

inline void draw_rectangle_from_point(uint32 from_x, uint32 from_y, uint32 to_x, uint32 to_y, PL pl, uint8 r, uint8 g, uint8 b)
{
	int32 width = to_x - from_x;
	int32 height = to_y - from_y;
	uint32 color = (uint32)r << 16 | (uint32)g << 8 | (uint32)b << 0;
	uint32* ptr = (uint32*)pl.window.window_bitmap.buffer + (from_y * pl.window.window_bitmap.width) + from_x;
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
		ptr += sign_height * pl.window.window_bitmap.width;
	}
}

inline void draw_verticle_line_from_point( uint32 x,uint32 y, int32 height, PL pl, uint8 r, uint8 g, uint8 b)
{
	uint32 color = (uint32)r << 16 | (uint32)g << 8 | (uint32)b << 0;
	uint32* ptr = (uint32*)pl.window.window_bitmap.buffer + (y * pl.window.window_bitmap.width) + x;
	if (height >= 0)
	{
		for (int y = 0; y < height; y++)
		{
			*ptr = color;
			ptr += pl.window.window_bitmap.width;
		}
	}
	else
	{
		for (int y = 0; y < -height; y++)
		{
			*ptr = color;
			ptr -= pl.window.window_bitmap.width;
		}
	}
}

void PL_update(PL& pl)
{
	/*if (pl.audio.input.no_of_new_frames == 0)
	{
		return;
	}*/
	memset(pl.window.window_bitmap.buffer, 33,4 * pl.window.window_bitmap.height * pl.window.window_bitmap.width);
	

	//static f64 timing_refresh = 0;
	//timing_refresh = pl.time.fcurrent_seconds - timing_refresh;
	//char buffer[256];
	//sprintf_s(buffer, "Time per sample: %.*fms \n", 2, timing_refresh * 1000);
	//OutputDebugStringA(buffer);
	//timing_refresh = pl.time.fcurrent_seconds;
	

	uint8 red, green, blue;
	f32 volume = 0;
	for (uint32 i = 0; i < pl.audio.input.no_of_new_frames; i++)
	{
		volume += pl.audio.input.sink_buffer[i] * pl.audio.input.sink_buffer[i];
	}
	volume = volume / (f32)pl.audio.input.no_of_new_frames;
	volume = sqrtf(volume);
	blue = ((volume * (f32)255) <= (f32)255) ? (uint8)((f32)volume * 255.f) : 255;
	//char msg[512];
	//wsprintfA(msg, "Mouse: pos_x:%i , pos_y:%i \n", pl.input.mouse.position_x, pl.input.mouse.position_y);
	//wsprintfA(msg, "New_frames: %i \n", pl.audio.input.no_of_new_frames);

	//OutputDebugStringA(msg);
	////draw_rectangle_from_point(pl.window_bltbitmap.width/2, pl.window_bltbitmap.height/2, pl.input.mouse.position_x,pl.input.mouse.position_y, pl, 255, 0, 0);

	for (uint32 i = 0; i < pl.window.window_bitmap.width; i++)
	{
		if (pl.audio.input.format.no_channels == 2)
		{
			f32 sample_pos = i / ((f32)pl.window.window_bitmap.width - 1.f);
			int32 left_audio_pos = (int32)(sample_pos * (f32)pl.audio.input.format.buffer_frame_count);
			if (left_audio_pos % 2 != 0)
			{
				left_audio_pos++;
			}
			//int32 audio_pos = (int32)(sample_pos * (f32)pl.audio.input.format.buffer_frame_count);
			f32 left_height = pl.audio.input.sink_buffer[left_audio_pos] * ((pl.window.window_bitmap.height - 10) / 4.f);
			f32 right_height = pl.audio.input.sink_buffer[left_audio_pos + 1] * ((pl.window.window_bitmap.height - 10) / 4.f);
			draw_verticle_line_from_point(i, pl.window.window_bitmap.height / 4, (int32)left_height, pl, 0, 255, 0);
			draw_verticle_line_from_point(i, (pl.window.window_bitmap.height * 3) / 4, (int32)right_height, pl, 0, 0, 255);
		}
		

		if (pl.audio.input.format.no_channels == 1)
		{
			f32 sample_pos = i / ((f32)pl.window.window_bitmap.width - 1.f);
			int32 audio_pos = (int32)(sample_pos * (f32)pl.audio.input.format.buffer_frame_count);
			f32 audio_height = pl.audio.input.sink_buffer[audio_pos] * ((pl.window.window_bitmap.height - 10) / 2.0f);
			draw_verticle_line_from_point(i, (pl.window.window_bitmap.height / 2), (int32)audio_height, pl, 0, 0, 244);
		}

	}
	//f32 added_pos = (f32)(pl.audio.input.format.buffer_frame_count - pl.audio.input.no_of_new_frames) / (f32)pl.audio.input.format.buffer_frame_count;
	//int32 added_pos_i = (int32)(added_pos * (f32)(pl.window_bltbitmap.width));
	//draw_verticle_line_from_point(added_pos_i, (pl.window_bltbitmap.height / 2), (pl.window_bltbitmap.height - 10) / 2.0f, pl, 255, 0, 0);
}

void PL_entry_point(PL& pl)
{
	pl.audio.input.only_update_every_new_buffer = FALSE;
	pl.audio.input.format.no_channels = 2;
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
	PL_cleanup(pl);
}
