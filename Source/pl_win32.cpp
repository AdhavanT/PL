#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <malloc.h>
#include "pl.h"
#include <stdio.h>

struct Win32Specific
{
	BITMAPINFO bmi_header;
	HDC main_monitor_DC;
	HINSTANCE hInstance;
	HWND wnd_handle;
	void* main_fiber;
	void* message_fiber;
};


//-------------------------------<win32 window stuff>----------------------------------------
LRESULT static CALLBACK wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);//actual win32 message callback
void static CALLBACK wnd_message_fiber_proc(PL& pl);//for switching to fiber that handles processing message callbacks
void PL_initialize_window(PL& pl)
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

	HRESULT s = RegisterClassA(&wnd_class);
	ASSERT(s != 0);

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

	((Win32Specific*)pl.platform_specific)->main_monitor_DC = GetDC(((Win32Specific*)pl.platform_specific)->wnd_handle);


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
		case WM_MOVE:
		{
			pl->window.was_altered = TRUE;

		}break;

		case WM_SIZE:
		{
			pl->window.was_altered = TRUE;	
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

	if (pl.window.was_altered || (pl.initialized == FALSE))
	{
		//NOTE:not gettin device context every time window moves. Checked with multi-monitor setup, worked fine with single device context. 
		//((Win32Specific*)pl.platform_specific)->main_monitor_DC = GetDC(((Win32Specific*)pl.platform_specific)->wnd_handle);

		RECT client_rectangle = {};
		GetClientRect(((Win32Specific*)pl.platform_specific)->wnd_handle, &client_rectangle);

		pl.window.width = client_rectangle.right - client_rectangle.left;
		pl.window.height = client_rectangle.bottom - client_rectangle.top;

		POINT window_position = {};
		ClientToScreen(((Win32Specific*)pl.platform_specific)->wnd_handle, &window_position);

		pl.window.position_x = window_position.x;
		pl.window.position_y = window_position.y;

		pl.window.was_altered = FALSE;
	}
	//TODO: get mouse input and stuff
}
 
//this updates the window by bliting the bitmap to the screen. So...sorta PL_push_bitmap() also.
void PL_push_window(PL& pl)
{
	//Refreshing the FPS counter in the window title bar. Comment out to turn off. 
	static f64 timing_refresh = 0;
	if (pl.time.fcurrent_seconds - timing_refresh > 0.1)//refreshing at a tenth(0.1) of a second.
	{
		int32 frame_rate = (int32)(pl.time.cycles_per_second / pl.time.delta_cycles);
		char buffer[256];
		sprintf_s(buffer, "Time per frame: %.*fms , %dFPS\n",2, pl.time.fdelta_seconds * 1000, frame_rate);
		SetWindowTextA(((Win32Specific*)pl.platform_specific)->wnd_handle, buffer);
		timing_refresh = pl.time.fcurrent_seconds;
	}

	//NOTE:Making sure to keep the buffer to monitor pixel mapping consistant. (not stretching the image to fit window.)
	StretchDIBits(
		((Win32Specific*)pl.platform_specific)->main_monitor_DC,
		0, 0, pl.bitmap.width, pl.bitmap.height,
		0, 0, pl.bitmap.width, pl.bitmap.height,
		pl.bitmap.buffer,
		&((Win32Specific*)pl.platform_specific)->bmi_header,
		DIB_RGB_COLORS, SRCCOPY);
}

//-------------------------------</win32 window stuff>----------------------------------------

//-------------------------------<timing stuff>-----------------------------------------------
void PL_poll_timing(PL& pl)
{
	LARGE_INTEGER new_q; 
	QueryPerformanceCounter(&new_q);

	f64 tmp_cs;
	uint64 tmp_cmil, tmp_cmic;

	tmp_cs = (f64)new_q.QuadPart / (f64)pl.time.cycles_per_second;
	tmp_cmil = (uint64)(tmp_cs * 1000);
	tmp_cmic = (uint64)(tmp_cs * 1000000);

	pl.time.delta_cycles = new_q.QuadPart - pl.time.current_cycles;
	pl.time.delta_millis = tmp_cmil - pl.time.current_millis;
	pl.time.delta_micros = tmp_cmic - pl.time.current_micros;

	pl.time.fdelta_seconds = (new_q.QuadPart - pl.time.current_cycles) / (f32)pl.time.cycles_per_second;

	pl.time.fcurrent_seconds = tmp_cs;
	pl.time.current_cycles = new_q.QuadPart;
	pl.time.current_seconds = (uint64)tmp_cs;
	pl.time.current_millis = tmp_cmil;
	pl.time.current_micros = tmp_cmic;
}

void PL_initialize_timing(PL& pl)
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	pl.time.cycles_per_second = frequency.QuadPart;

	PL_poll_timing(pl);	//To avoid the first frame having wierd 0 delta and current time values.
}

//-------------------------------</timing stuff>----------------------------------------------

//-------------------------------<bitmap stuff>-----------------------------------------------
void PL_initialize_bitmap(PL& pl)
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

	BITMAPINFO* bmi = &((Win32Specific*)pl.platform_specific)->bmi_header;
	bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
	bmi->bmiHeader.biBitCount = 8 * pl.bitmap.bytes_per_pixel;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biHeight = pl.bitmap.height;
	bmi->bmiHeader.biWidth = pl.bitmap.width;

}
//-------------------------------</bitmap stuff>-----------------------------------------------

//-------------------------------<Win32 Audio stuff>-------------------------------------------
//NOTE: Input audio stream is in loopback mode. Audio streams are initialized by defaults. 
void PL_initialize_audio(PL& pl)
{
	CoInitializeEx(0, COINIT_MULTITHREADED);	//ASSESS: whether i should use COINIT_MULTITHREADED or COINIT_APARTMENTTHREADED
	IMMDeviceEnumerator* pEnumerator = 0;
	IMMDevice* output_endpoint = 0;
	IMMDevice* input_endpoint = 0;

	IAudioClient* output_audio_client = 0;
	IAudioClient* input_audio_client = 0;


	HRESULT result;
	result = CoCreateInstance(__uuidof(MMDeviceEnumerator), 0, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
	ASSERT(!FAILED(result));

	result = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &output_endpoint);
	ASSERT(result == S_OK);

	if (pl.audio.input.is_loopback)
	{
		result = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &input_endpoint);	//It's eRender instead of eCapture cause input is loopback.
		ASSERT(result == S_OK);
	}
	else
	{
		result = pEnumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, &input_endpoint);
		ASSERT(result == S_OK);
	}

	result = output_endpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, 0, (void**)&output_audio_client);
	ASSERT(result == S_OK);

	result = input_endpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, 0, (void**)&input_audio_client);
	ASSERT(result == S_OK);


	WAVEFORMATEX of = {};
	DWORD output_stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
	WAVEFORMATEX ipf = {};
	DWORD input_stream_flags =  AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
	if (pl.audio.input.is_loopback)
	{
		input_stream_flags = input_stream_flags | AUDCLNT_STREAMFLAGS_LOOPBACK;
	}


	of.wFormatTag = WAVE_FORMAT_PCM;
	of.nChannels = pl.audio.output.format.no_channels;
	of.nSamplesPerSec = pl.audio.output.format.samples_per_second;
	of.nBlockAlign = (pl.audio.output.format.no_channels * pl.audio.output.format.no_bits_per_sample) / 8;
	of.nAvgBytesPerSec = pl.audio.output.format.samples_per_second * of.nBlockAlign;
	of.wBitsPerSample = pl.audio.output.format.no_bits_per_sample;

	ipf.wFormatTag = WAVE_FORMAT_PCM;
	ipf.nChannels = pl.audio.input.format.no_channels;
	ipf.nSamplesPerSec = pl.audio.input.format.samples_per_second;
	ipf.nBlockAlign = (pl.audio.input.format.no_channels * pl.audio.input.format.no_bits_per_sample) / 8;
	ipf.nAvgBytesPerSec = pl.audio.input.format.samples_per_second * ipf.nBlockAlign;
	ipf.wBitsPerSample = pl.audio.input.format.no_bits_per_sample;


	// REFERENCE_TIME time units per second and per millisecond
	#define REFTIMES_PER_SEC  10000000

	if (pl.audio.input.format.buffer_duration_seconds == 0 && pl.audio.input.format.buffer_frame_count == 0)
	{
		pl.audio.input.format.buffer_duration_seconds = 1.0f;	//ASSESS: Whether i should even allow the user not specify how big the buffer is. (should i assert here)
	}
	else if (pl.audio.input.format.buffer_duration_seconds == 0 && pl.audio.input.format.buffer_frame_count != 0)
	{
		pl.audio.input.format.buffer_duration_seconds = REFTIMES_PER_SEC * (f32)pl.audio.input.format.buffer_frame_count / (f32)pl.audio.input.format.samples_per_second;
	}

	uint32 duration = (pl.audio.input.format.buffer_duration_seconds * (f32)REFTIMES_PER_SEC);
	result = input_audio_client->Initialize(AUDCLNT_SHAREMODE_SHARED, input_stream_flags, duration, 0, &ipf, 0);
	ASSERT(result == S_OK);

	result = input_audio_client->GetBufferSize(&pl.audio.input.format.buffer_frame_count);
	ASSERT(result == S_OK);

	pl.audio.input.format.buffer_duration_seconds = REFTIMES_PER_SEC * (f32)pl.audio.input.format.buffer_frame_count / (f32)pl.audio.input.format.samples_per_second;
	#undef REFTIMES_PER_SEC

	result = input_audio_client->Start();
	ASSERT(result == S_OK);


}
void PL_poll_audio(PL& pl)
{
	//TODO: retrieve audio buffer from input device
	//TODO: make the input buffer a circular buffer that collects input packets on a seperate thread till frame end and then waits for half a frame then starts reading into buffer again.
}
void PL_push_audio(PL& pl)
{
	//TODO: push out audio buffer to output device
}

//-------------------------------</Win32 Audio stuff>------------------------------------------




//--------------------------------<Win32 ENTRY POINT>------------------------------------------
//A platform's PL implementation has the job of creating a PL object and a platform_specific object in the main() and calling PL_entry_point to let the 
//application handle how the initialization and how the game loop runs. 
int WINAPI wWinMain(_In_ HINSTANCE hInstance,
					_In_opt_ HINSTANCE hPrevInstance,
					_In_ LPWSTR    lpCmdLine,
					_In_ int       nCmdShow)
{
	Win32Specific pl_win32 = {};
	pl_win32.hInstance = hInstance;
	
	PL pl = {};
	pl.platform_specific = &pl_win32;

	PL_entry_point(pl);
}
//--------------------------------</Win32 ENTRY POINT>------------------------------------------
