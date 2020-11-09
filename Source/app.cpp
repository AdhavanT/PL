#include "pl.h"
#include <Windows.h>
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
	PL_poll_timing(pl);
	PL_poll_window(pl);
}

void PL_push(PL& pl)
{
	PL_push_window(pl);
}

void PL_update(PL& pl)
{
	uint32 *ptr = (uint32*)pl.bitmap.buffer;
	for (int y = 0; y < pl.bitmap.height; y++)
	{
		for (int x = 0; x < pl.bitmap.width; x++)
		{
			*ptr = 0xFF893722;
			ptr++;
		}
	}

}

void PL_entry_point(PL& pl)
{
	pl.audio.input.format =
	{
		2,16,44100,0, 0.01667f
	};
	pl.audio.input.is_loopback = TRUE;
	PL_initialize(pl);
	while (pl.running)
	{
		PL_poll(pl);
		PL_update(pl);
		PL_push(pl);
	}
}
