#include "pl.h"
#include <Windows.h>
#include <stdio.h>

void PL_update(PL& pl)
{
	uint32 *ptr = (uint32*)pl.bitmap.buffer;
	for (int y = 0; y < 400; y++)
	{
		for (int x = 0; x < 300; x++)
		{
			*ptr = 0xFF893722;
			ptr++;
		}
	}

}