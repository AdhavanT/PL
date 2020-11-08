#include <windows.h>
#include <malloc.h>
#include "pl.h"

struct Win32Specific
{
	HINSTANCE hInstance;
	HWND wnd_handle;
	void* main_fiber;
	void* message_fiber;
};


//-------------------------------<win32 window stuff>----------------------------------------
void PL_poll_window(PL& pl);
LRESULT static CALLBACK wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);//actual win32 message callback
void static CALLBACK wnd_message_fiber_proc(PL& pl);//for switching to fiber that handles processing message callbacks
static void PL_initialize_window(PL& pl)
{
	int window_width, window_height;
	if (pl.window.height == 0)	//uninitialized
	{
		window_height = CW_USEDEFAULT;
	}
	else
	{
		window_height = pl.window.height;
	}
	if (pl.window.width == 0)
	{
		window_width = CW_USEDEFAULT;
	}
	else
	{
		window_width = pl.window.width;
	}
	
	if (pl.window.title == 0)
	{
		pl.window.title = (char*)"Win32 PL test";
	}

	if (window_width != CW_USEDEFAULT && window_height != CW_USEDEFAULT)
	{
		RECT window_rectangle;
		window_rectangle.left = 0;
		window_rectangle.right = pl.window.width;
		window_rectangle.top = 0;
		window_rectangle.bottom = pl.window.height;
		if (AdjustWindowRect(&window_rectangle, WS_OVERLAPPEDWINDOW, 0)) 
		{
			window_width = window_rectangle.right - window_rectangle.left;
			window_height = window_rectangle.bottom - window_rectangle.top;
		}
	}

	WNDCLASSA wnd_class = {};
	wnd_class.lpfnWndProc = wnd_proc;
	wnd_class.lpszClassName = "pl_window_class";
	wnd_class.style = CS_VREDRAW | CS_HREDRAW;
	wnd_class.hInstance = ((Win32Specific*)pl.platform_specific)->hInstance;

	ASSERT(RegisterClassA(&wnd_class));

	((Win32Specific*)pl.platform_specific)->wnd_handle = CreateWindowExA(
		0,
		wnd_class.lpszClassName,
		pl.window.title,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		pl.window.position_x,
		pl.window.position_y,
		window_width,
		window_height,
		0,
		0,
		0,
		0
	);
	ASSERT(((Win32Specific*)pl.platform_specific)->wnd_handle);
	//ShowWindow(((Win32Specific*)pl.platform_specific)->wnd_handle, SW_SHOW);

	//This passes a pointer to pl to the wnd_proc message callback. (It's retrieved by GetWindowLongPtrA(hwnd, GWLP_USERDATA))
	SetWindowLongPtrA(((Win32Specific*)pl.platform_specific)->wnd_handle, GWL_USERDATA, (LONG_PTR)&pl);
	PL* re = (PL*)GetWindowLongPtrA(((Win32Specific*)pl.platform_specific)->wnd_handle, GWLP_USERDATA);
	
	((Win32Specific*)pl.platform_specific)->main_fiber = ConvertThreadToFiber(0);
	ASSERT(((Win32Specific*)pl.platform_specific)->main_fiber);
	((Win32Specific*)pl.platform_specific)->message_fiber = CreateFiber(0, (PFIBER_START_ROUTINE)wnd_message_fiber_proc, &pl);
	ASSERT(((Win32Specific*)pl.platform_specific)->message_fiber);

	//TODO: get and set device contex for opengl stuff maybe

	PL_poll_window(pl);
	

}


//a fiber to handle messages to keep main loop going while doing so.
//A 1ms timer alerts the message callback when to switch to the main loop fiber.
void CALLBACK wnd_message_fiber_proc(PL& pl)
{
	//Sets a timer for 1ms that sends a WM_TIMER message to the message queue
	SetTimer(((Win32Specific*)pl.platform_specific)->wnd_handle, 1, 1, 0);

	while (pl.running)
	{
		MSG message;
		while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
		{
			if (message.message == WM_QUIT)
			{
				pl.running = false;
			}
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
		SwitchToFiber(((Win32Specific*)pl.platform_specific)->main_fiber);
	}
	SwitchToFiber(((Win32Specific*)pl.platform_specific)->main_fiber);

} 

LRESULT static CALLBACK wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	PL* pl = (PL*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
	if (!pl)
	{
		return DefWindowProcA(hwnd, uMsg, wParam, lParam);
	}
	switch (uMsg)
	{
		case WM_SIZE:
		{
			
			pl->window.was_resized = TRUE;
			
		}break;

		case WM_KEYUP:
		{
			uint32 VKCode = wParam;

			//NOTE: To exit the application using alt+F4
			b8 AltKeyWasDown = (lParam & (1 << 29)) != 0;

			if (VKCode == VK_F4 && AltKeyWasDown)
			{
				pl->running = false;
			}
		}break;

		case WM_DESTROY:
		{
			pl->running = FALSE;
			PostQuitMessage(0);
		}break;

		case WM_TIMER:
		{
			SwitchToFiber(((Win32Specific*)pl->platform_specific)->main_fiber);
		}break;

		default:
		{
			result = DefWindowProcA(hwnd, uMsg, wParam, lParam);
		}
	}
	return result;
}

void PL_poll_window(PL &pl)
{
	SwitchToFiber(((Win32Specific*)pl.platform_specific)->message_fiber);

	if (pl.window.was_resized || (pl.initialized == FALSE))
	{
		RECT client_rectangle = {};
		GetClientRect(((Win32Specific*)pl.platform_specific)->wnd_handle, &client_rectangle);

		pl.window.width = client_rectangle.right - client_rectangle.left;
		pl.window.height = client_rectangle.bottom - client_rectangle.top;

		POINT window_position = {};
		ClientToScreen(((Win32Specific*)pl.platform_specific)->wnd_handle, &window_position);

		pl.window.position_x = window_position.x;
		pl.window.position_y = window_position.y;

		pl.window.was_resized = false;
	}
	//TODO: get mouse input and stuff
}
//-------------------------------</win32 window stuff>----------------------------------------


//-------------------------------<bitmap stuff>-----------------------------------------------
static void PL_initialize_bitmap(PL& pl)
{
	if (pl.bitmap.height == 0)	//uninitialized
	{
		pl.bitmap.height = pl.window.height;	//assuming PL_initialized_window() happens before
	}
	if (pl.bitmap.width == 0)
	{
		pl.bitmap.width = pl.window.width;
	}
	if (pl.bitmap.bytes_per_pixel == 0)
	{
		pl.bitmap.bytes_per_pixel = 4;
	}
	pl.bitmap.pitch = pl.bitmap.bytes_per_pixel * pl.bitmap.width;
	pl.bitmap.size = pl.bitmap.pitch * pl.bitmap.height;

	if ((pl.bitmap.buffer == 0) && (pl.bitmap.size != 0))
	{
		pl.bitmap.buffer = malloc(pl.bitmap.size);
	}
	ASSERT(pl.bitmap.buffer);
}
//-------------------------------</bitmap stuff>-----------------------------------------------



//--------------------------------<Win32 ENTRY POINT>------------------------------------------
int WINAPI wWinMain(_In_ HINSTANCE hInstance,
					_In_opt_ HINSTANCE hPrevInstance,
					_In_ LPWSTR    lpCmdLine,
					_In_ int       nCmdShow)
{
	Win32Specific pl_win32 = {};
	pl_win32.hInstance = hInstance;
	
	PL pl = {};
	pl.platform_specific = &pl_win32;

	PL_initialize(pl);
	pl.initialized = TRUE;
	while (pl.running)
	{
		PL_poll(pl);
		PL_update(pl);
		PL_push(pl);
	}
}
//--------------------------------</Win32 ENTRY POINT>------------------------------------------


void PL_initialize(PL& pl)
{
	pl.running = TRUE;
	PL_initialize_window(pl);
	PL_initialize_bitmap(pl);
}

void PL_poll(PL& pl)
{
	PL_poll_window(pl);
}

void PL_push(PL &pl)
{

}
