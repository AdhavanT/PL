#include "pl.h"
#include <Windows.h>
#include <stdio.h>

void PL_update(PL& pl)
{
	static int x = 0;
	if (x % 1000)
	{
		char ch[512];
		sprintf_s(ch, "width:%d , height:%d , pos_x:%d, pos_y:%d \n", pl.window.width, pl.window.height, pl.window.position_x, pl.window.position_y);
		OutputDebugStringA(ch);
	}
	x++;
}