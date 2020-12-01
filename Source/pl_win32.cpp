#include "pl.h"
#include "pl_config.h"
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <windowsx.h>
#include <malloc.h>
#include <stdio.h>

struct Win32Specific
{
	HINSTANCE hInstance;
	//Window stuff
	HDC main_monitor_DC;
	HWND wnd_handle;
	void* main_fiber;
	void* message_fiber;
	b32* pointer_to_pl_running;
#if PL_WINDOW_RENDERTYPE == PL_BLIT_BITMAP
	BITMAPINFO window_bmi_header;
#endif


	//Audio Capture Stuff
	void* input_fifo_buffer;
	IAudioCaptureClient* input_capture_client;
	void (*transfer_to_sink_buffer)(PL_Audio_Input& input);
	//Audio Render Stuff
	IAudioClient *output;
};

static Win32Specific *pl_specific;

//-------------------------------<win32 window stuff>----------------------------------------
LRESULT static CALLBACK wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);//actual win32 message callback
void static CALLBACK wnd_message_fiber_proc(PL_Window& window);//for switching to fiber that handles processing message callbacks
void PL_initialize_window(PL_Window& window)
{

	int window_width, window_height;
	if (window.height == 0)	//if uninitialized
	{
		window_height = CW_USEDEFAULT;
	}
	else
	{
		window_height = window.height;
	}
	if (window.width == 0)
	{
		window_width = CW_USEDEFAULT;
	}
	else
	{
		window_width = window.width;
	}
	
	if (window.title == 0)
	{
		window.title = (char*)"Win32 PL test";
	}
	
	if (window_width != CW_USEDEFAULT && window_height != CW_USEDEFAULT)
	{
		RECT window_rectangle;
		window_rectangle.left = 0;
		window_rectangle.right = window.width;
		window_rectangle.top = 0;
		window_rectangle.bottom = window.height;
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
	wnd_class.hInstance = pl_specific->hInstance;
	wnd_class.hCursor = LoadCursorA(NULL, MAKEINTRESOURCEA(32515));

	HRESULT s = RegisterClassA(&wnd_class);
	ASSERT(s != 0);

	pl_specific->wnd_handle = CreateWindowExA(
		0,
		wnd_class.lpszClassName,
		window.title,
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		window.position_x,
		window.position_y,
		window_width,
		window_height,
		0,
		0,
		0,
		0
	);
	ASSERT(pl_specific->wnd_handle);
	//ShowWindow(pl_specific->wnd_handle, SW_SHOW);

	//This passes a pointer to pl to the wnd_proc message callback. (It's retrieved by GetWindowLongPtrA(hwnd, GWLP_USERDATA))
	SetWindowLongPtrA(pl_specific->wnd_handle, GWL_USERDATA, (LONG_PTR)&window);
	
	pl_specific->main_fiber = ConvertThreadToFiber(0);
	ASSERT(pl_specific->main_fiber);
	pl_specific->message_fiber = CreateFiber(0, (PFIBER_START_ROUTINE)wnd_message_fiber_proc, &window);
	ASSERT(pl_specific->message_fiber);

	pl_specific->main_monitor_DC = GetDC(pl_specific->wnd_handle);


#if PL_WINDOW_RENDERTYPE == PL_BLIT_BITMAP
	if (window.window_bitmap.height == 0 || window.window_bitmap.width == 0)	//uninitialized
	{
		window.window_bitmap.height = window.height;
		window.window_bitmap.width = window.width;
	}
	if (window.window_bitmap.bytes_per_pixel == 0)
	{
		window.window_bitmap.bytes_per_pixel = 4;
	}
	window.window_bitmap.pitch = window.window_bitmap.bytes_per_pixel * window.window_bitmap.width;
	window.window_bitmap.size = window.window_bitmap.pitch * window.window_bitmap.height;

	if ((window.window_bitmap.buffer == 0) && (window.window_bitmap.size != 0))
	{
		window.window_bitmap.buffer = malloc(window.window_bitmap.size);
	}
	ASSERT(window.window_bitmap.buffer);

	BITMAPINFO* bmi = &pl_specific->window_bmi_header;
	bmi->bmiHeader.biSize = sizeof(bmi->bmiHeader);
	bmi->bmiHeader.biBitCount = 8 * window.window_bitmap.bytes_per_pixel;
	bmi->bmiHeader.biCompression = BI_RGB;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biHeight = window.window_bitmap.height;
	bmi->bmiHeader.biWidth = window.window_bitmap.width;
#endif

	//polling once
	SwitchToFiber(pl_specific->message_fiber);
	RECT client_rectangle = {};
	GetClientRect(pl_specific->wnd_handle, &client_rectangle);

	window.width = client_rectangle.right - client_rectangle.left;
	window.height = client_rectangle.bottom - client_rectangle.top;

	POINT window_position = {};
	ClientToScreen(pl_specific->wnd_handle, &window_position);

	window.position_x = window_position.x;
	window.position_y = window_position.y;

#if PL_WINDOW_RENDERTYPE == PL_BLIT_BITMAP
	//FIXME: This clears the window to black on resizing to avoid the extra window(outside bitmap) from copying the previous frame. 
	//It also causes random black frames while resizing.
	PatBlt(pl_specific->main_monitor_DC, 0, 0, window.width, window.height, BLACKNESS);
#endif

}

//a fiber to handle messages to keep main loop going while doing so.
//A 1ms timer alerts the message callback when to switch to the main loop fiber.
void CALLBACK wnd_message_fiber_proc(PL_Window& window)
{
	//Sets a timer for 1ms that sends a WM_TIMER message to the message queue
	SetTimer(pl_specific->wnd_handle, 1, 1, 0);

	while (*pl_specific->pointer_to_pl_running)	
	{
		MSG message;
		while (PeekMessageA(&message, 0, 0, 0, PM_REMOVE))
		{
			if (message.message == WM_QUIT)
			{
				*pl_specific->pointer_to_pl_running = FALSE;
			}
			TranslateMessage(&message);
			DispatchMessageA(&message);
		}
		SwitchToFiber(pl_specific->main_fiber);
	}
	SwitchToFiber(pl_specific->main_fiber);
} 

LRESULT static CALLBACK wnd_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;
	PL_Window* window = (PL_Window*)GetWindowLongPtrA(hwnd, GWLP_USERDATA);
	if (!window)
	{
		return DefWindowProcA(hwnd, uMsg, wParam, lParam);
	}
	switch (uMsg)
	{
		case WM_ENTERSIZEMOVE:
		{
			window->was_altered = TRUE;	
			SetTimer(hwnd, 1, 1, 0);
		}break;
		case WM_EXITSIZEMOVE:
		{
			window->was_altered = FALSE;
			KillTimer(hwnd, 1);

		}break;

		case WM_DESTROY:
		{
			*pl_specific->pointer_to_pl_running = FALSE;
			PostQuitMessage(0);
		}break;

		case WM_TIMER:
		{
			SwitchToFiber((pl_specific)->main_fiber);
		}break;

		default:
		{
			result = DefWindowProcA(hwnd, uMsg, wParam, lParam);
		}
	}
	return result;
}

void PL_poll_window(PL_Window &window)
{
	SwitchToFiber(pl_specific->message_fiber);

	if (window.was_altered)
	{
		//NOTE:not gettin device context every time window moves. Checked with multi-monitor setup, worked fine with single device context. 
		//pl_specific->main_monitor_DC = GetDC(pl_specific->wnd_handle);

		RECT client_rectangle = {};
		GetClientRect(pl_specific->wnd_handle, &client_rectangle);

		window.width = client_rectangle.right - client_rectangle.left;
		window.height = client_rectangle.bottom - client_rectangle.top;

		POINT window_position = {};
		ClientToScreen(pl_specific->wnd_handle, &window_position);

		window.position_x = window_position.x;
		window.position_y = window_position.y;

#if PL_WINDOW_RENDERTYPE == PL_BLIT_BITMAP
		//FIXME: This clears the window to black on resizing to avoid the extra window(outside bitmap) from copying the previous frame. 
		//It also causes random black frames while resizing.
		PatBlt(pl_specific->main_monitor_DC, 0, 0, window.width, window.height, BLACKNESS);
#endif
	}

}
 
//this updates the window. include PL_Timing for the window title to display frametimes, pass null for normal window title.
void PL_push_window(PL_Window& window, PL_Timing* time)
{
	//Refreshing the FPS counter in the window title bar. Comment out to turn off. 
	if (time)
	{
		static f64 timing_refresh = 0;
		if (time->fcurrent_seconds - timing_refresh > 0.1)//refreshing at a tenth(0.1) of a second.
		{
			int32 frame_rate = (int32)(time->cycles_per_second / time->delta_cycles);
			char buffer[256];
			sprintf_s(buffer, "Time per frame: %.*fms , %dFPS\n",2, (f64)time->fdelta_seconds * 1000, frame_rate);
			SetWindowTextA(pl_specific->wnd_handle, buffer);
			timing_refresh = time->fcurrent_seconds;
		}
	}

#if PL_WINDOW_RENDERTYPE == PL_BLIT_BITMAP
	//NOTE:Making sure to keep the buffer-to-monitor pixel mapping consistant. (not stretching the image to fit window.)
	//NOTE: This is assuming the window is drawing a bitmap 
	StretchDIBits(
		pl_specific->main_monitor_DC,
		0,0, window.window_bitmap.width, window.window_bitmap.height,
		0, 0, window.window_bitmap.width, window.window_bitmap.height,
		window.window_bitmap.buffer,
		&pl_specific->window_bmi_header,
		DIB_RGB_COLORS, SRCCOPY);
#endif
}

void PL_cleanup_window(PL_Window& window)
{
#if PL_WINDOW_RENDERTYPE == PL_BLIT_BITMAP
	free(window.window_bitmap.buffer);
#endif
}

//-------------------------------</win32 window stuff>----------------------------------------

//-------------------------------<timing stuff>-----------------------------------------------
void PL_initialize_timing(PL_Timing& time)
{
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	time.cycles_per_second = frequency.QuadPart;

	PL_poll_timing(time);	//To avoid the first frame having wierd 0 delta and current time values.
}

void PL_poll_timing(PL_Timing& time)
{
	LARGE_INTEGER new_q; 
	QueryPerformanceCounter(&new_q);

	f64 tmp_cs;
	uint64 tmp_cmil, tmp_cmic;

	tmp_cs = (f64)new_q.QuadPart / (f64)time.cycles_per_second;
	tmp_cmil = (uint64)(tmp_cs * 1000);
	tmp_cmic = (uint64)(tmp_cs * 1000000);

	time.delta_cycles = new_q.QuadPart - time.current_cycles;
	time.delta_millis = tmp_cmil - time.current_millis;
	time.delta_micros = tmp_cmic - time.current_micros;

	time.fdelta_seconds = (new_q.QuadPart - time.current_cycles) / (f32)time.cycles_per_second;

	time.fcurrent_seconds = tmp_cs;
	time.current_cycles = new_q.QuadPart;
	time.current_seconds = (uint64)tmp_cs;
	time.current_millis = tmp_cmil;
	time.current_micros = tmp_cmic;
}

//-------------------------------</timing stuff>----------------------------------------------

//-------------------------------<Input stuff>------------------------------------------------
//NOTE: For mouse input, WM_MOUSEMOVE is used. This is less precise than WM_INPUT and only changes on 
//when the mouse moves to another pixel. Consider using raw mouse data from WM_INPUT for more 
inline void update_digital_button(PL_Digital_Button& bt, b32 down)
{
	bt.pressed = !bt.down && down;
	bt.released = bt.down && !down;
	bt.down = down;
}
//high-definition mouse input for applications that need it. 
void PL_initialize_input_mouse(PL_Input_Mouse& mouse)
{
	//Initializing mouse input

}

void PL_poll_input_mouse(PL_Input_Mouse& mouse, PL_Window& main_window)
{
	//Getting mouse input
	POINT mouse_pos;
	GetCursorPos(&mouse_pos);
	ScreenToClient(pl_specific->wnd_handle, &mouse_pos);
	mouse.position_x = mouse_pos.x;
	mouse.position_y = main_window.height - mouse_pos.y;
	mouse.is_in_window = (mouse.position_x >= 0 && mouse.position_x < (int32)main_window.width) && (mouse.position_y >= 0 && mouse.position_y < (int32)main_window.height);
	SHORT lb = GetKeyState(VK_LBUTTON);
	SHORT rb = GetKeyState(VK_RBUTTON);
	update_digital_button(mouse.left, lb >> 7);
	update_digital_button(mouse.right, rb >> 7);
}

void PL_initialize_input_keyboard(PL_Input_Keyboard& keyboard)
{
	//Initializing keyboard input
}

void PL_poll_input_keyboard(PL_Input_Keyboard& keyboard)
{
	//Getting keyboard input
	b32 result;
	unsigned char kb[256];
	result = GetKeyboardState(kb);
	ASSERT(result);
	for (int i = 0; i < PL_INPUT_KEYBOARD_MAX_KEYS; i++)
	{
		update_digital_button(keyboard.keys[i], kb[i] >> 7);
	}
}

void PL_initialize_input_gamepad(PL_Input_Gamepad& gamepad)
{
	//Initializing gamepad input
}

void PL_poll_input_gamepad(PL_Input_Gamepad& gamepad)
{

}

//-------------------------------</Input stuff>-----------------------------------------------

//-------------------------------<Win32 Audio stuff>-------------------------------------------
//-------------------------------<Audio Render stuff>--------------------------------------

void PL_initialize_audio_render(PL_Audio_Output& output)
{
	CoInitializeEx(0, COINIT_MULTITHREADED);//ASSESS: whether i should use COINIT_MULTITHREADED or COINIT_APARTMENTTHREADED
	IMMDeviceEnumerator* pEnumerator = 0;
	IMMDevice* output_endpoint = 0;
	IAudioClient* output_audio_client;

	HRESULT result;
	result = CoCreateInstance(__uuidof(MMDeviceEnumerator), 0, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
	ASSERT(!FAILED(result));

	result = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &output_endpoint);
	ASSERT(result == S_OK);

	result = output_endpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, 0, (void**)&output_audio_client);
	ASSERT(result == S_OK);

	WAVEFORMATEX of = {};
	DWORD output_stream_flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
	WAVEFORMATEX* q_of;

	(output_audio_client)->GetMixFormat(&q_of);

	//setting defaults (non-initialized values in format)
	if (output.format.no_bits_per_sample == 0)
	{
		output.format.no_bits_per_sample = q_of->wBitsPerSample;
	}
	if (output.format.no_channels == 0)
	{
		output.format.no_channels = q_of->nChannels;
	}
	if (output.format.samples_per_second == 0)
	{
		output.format.samples_per_second = q_of->nSamplesPerSec;
	}

	of.wFormatTag = WAVE_FORMAT_PCM;
	of.nChannels = output.format.no_channels;
	of.nSamplesPerSec = output.format.samples_per_second;
	of.nBlockAlign = (output.format.no_channels * output.format.no_bits_per_sample) / 8;
	of.nAvgBytesPerSec = output.format.samples_per_second * of.nBlockAlign;
	of.wBitsPerSample = output.format.no_bits_per_sample;



	CoTaskMemFree(q_of);
	pEnumerator->Release();
	output_endpoint->Release();
}

void PL_push_audio_render(PL_Audio_Output& pl)
{
	//TODO: push out audio buffer to output device
}
void PL_cleanup_audio_render(PL_Audio_Output& pl)
{
	pl_specific->output->Stop();
	pl_specific->output->Release();
}
//-------------------------------</Audio Render stuff>--------------------------------------

//-------------------------------<Audio Capture stuff>--------------------------------------
//functions responsible for shifting the sink_buffer and adding the new frames polled from the fifo buffer
static void transfer_capture_16bit_2channel(PL_Audio_Input& input);
static void transfer_capture_16bit_1channel(PL_Audio_Input& input);
static void transfer_capture_32bit_2channel(PL_Audio_Input& input);
static void transfer_capture_32bit_1channel(PL_Audio_Input& input);

void PL_initialize_audio_capture(PL_Audio_Input& input)
{
	CoInitializeEx(0, COINIT_MULTITHREADED);//ASSESS: whether i should use COINIT_MULTITHREADED or COINIT_APARTMENTTHREADED
	IMMDeviceEnumerator* pEnumerator = 0;

	IMMDevice* input_endpoint = 0;

	IAudioClient* input_audio_client;

	HRESULT result;
	result = CoCreateInstance(__uuidof(MMDeviceEnumerator), 0, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&pEnumerator);
	ASSERT(!FAILED(result));

	if (input.is_loopback)
	{
		result = pEnumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &input_endpoint);//It's eRender instead of eCapture cause input is loopback.
		ASSERT(result == S_OK);
	}
	else
	{
		result = pEnumerator->GetDefaultAudioEndpoint(eCapture, eMultimedia, &input_endpoint);
		ASSERT(result == S_OK);
	}

	

	result = input_endpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, 0, (void**)&input_audio_client);
	ASSERT(result == S_OK);


	WAVEFORMATEX ipf = {};
	DWORD input_stream_flags = AUDCLNT_STREAMFLAGS_RATEADJUST | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM;
	WAVEFORMATEX *q_ipf;
	(input_audio_client)->GetMixFormat(&q_ipf);

	//setting defaults (non-initialized values in format)
	if (input.format.no_bits_per_sample == 0)
	{
		input.format.no_bits_per_sample = q_ipf->wBitsPerSample;
	}
	if (input.format.no_channels == 0)
	{
		input.format.no_channels = q_ipf->nChannels;
	}
	if (input.format.samples_per_second == 0)
	{
		input.format.samples_per_second = q_ipf->nSamplesPerSec;
	}

	if (input.is_loopback)
	{
		input_stream_flags = input_stream_flags | AUDCLNT_STREAMFLAGS_LOOPBACK;
	}

	ipf.wFormatTag = WAVE_FORMAT_PCM;
	ipf.nChannels = input.format.no_channels;
	ipf.nSamplesPerSec = input.format.samples_per_second;
	ipf.nBlockAlign = (input.format.no_channels * input.format.no_bits_per_sample) / 8;
	ipf.nAvgBytesPerSec = input.format.samples_per_second * ipf.nBlockAlign;
	ipf.wBitsPerSample = input.format.no_bits_per_sample;

	
	// REFERENCE_TIME time units per second and per millisecond
	#define REFTIMES_PER_SEC  10000000

	if (input.format.buffer_duration_seconds == 0 && input.format.buffer_frame_count == 0)
	{
		input.format.buffer_duration_seconds = 1.0f;
	}
	else if (input.format.buffer_duration_seconds == 0 && input.format.buffer_frame_count != 0)
	{
		input.format.buffer_duration_seconds = REFTIMES_PER_SEC * (f32)input.format.buffer_frame_count;
		input.format.buffer_duration_seconds /= (f32)input.format.samples_per_second;
	}

	uint32 duration = (uint32)(input.format.buffer_duration_seconds * (f32)REFTIMES_PER_SEC);
	result = (input_audio_client)->Initialize(AUDCLNT_SHAREMODE_SHARED, input_stream_flags, duration, 0, &ipf, 0);
	ASSERT(result == S_OK);

	result = (input_audio_client)->GetBufferSize(&input.format.buffer_frame_count);
	ASSERT(result == S_OK);

	input.format.buffer_duration_seconds = (f32)input.format.buffer_frame_count / (f32)input.format.samples_per_second;
	#undef REFTIMES_PER_SEC

	result = (input_audio_client)->GetService(__uuidof(IAudioCaptureClient), (void**)&pl_specific->input_capture_client);
	ASSERT(result == S_OK);

	result = (input_audio_client)->Start();
	ASSERT(result == S_OK);

	uint8 bytes_per_frame = input.format.no_channels * (input.format.no_bits_per_sample / 8);
	
	pl_specific->input_fifo_buffer = calloc(1, (input.format.buffer_frame_count * bytes_per_frame));

	input.sink_buffer = (f32*)calloc(1,input.format.buffer_frame_count * sizeof(f32) * input.format.no_channels);

	CoTaskMemFree(q_ipf);
	pEnumerator->Release();
	input_endpoint->Release();
	input_audio_client->Release();

	if (input.format.no_bits_per_sample == 16 && input.format.no_channels == 2)
	{
		pl_specific->transfer_to_sink_buffer = transfer_capture_16bit_2channel;
	}
	else if(input.format.no_bits_per_sample == 16 && input.format.no_channels == 1)
	{
		pl_specific->transfer_to_sink_buffer = transfer_capture_16bit_1channel;
	}
	else if (input.format.no_bits_per_sample == 32 && input.format.no_channels == 2)
	{
		pl_specific->transfer_to_sink_buffer = transfer_capture_32bit_2channel;
	}
	else if (input.format.no_bits_per_sample == 32 && input.format.no_channels == 1)
	{
		pl_specific->transfer_to_sink_buffer = transfer_capture_32bit_1channel;
	}
	else
	{
		ASSERT(FALSE);	//non-supported audio bit-rate, channel format
	}
	PL_poll_audio_capture(input);
}

//functions responsible for shifting the sink_buffer and adding the new frames polled from the fifo buffer
static void transfer_capture_16bit_2channel(PL_Audio_Input& input)
{
	f32* sink_front = input.sink_buffer;
	void* rb = pl_specific->input_fifo_buffer;

	//shifting existing buffer to right to make room for new frames that are added at beginning of buffer
	int32 no_bytes_per_frame = sizeof(f32) * input.format.no_channels;
	void* sink_end = (uint8*)sink_front + (no_bytes_per_frame * input.no_of_new_frames);
	memmove(sink_end, (void*)sink_front, (input.format.buffer_frame_count - input.no_of_new_frames) * no_bytes_per_frame);

	int16* left_channel = (int16*)rb + ((input.format.buffer_frame_count - 1) * 2);
	int16* right_channel = left_channel + 1;
	for (uint32 i = 0; i < input.no_of_new_frames; i++)
	{
		*sink_front = ((f32)*left_channel) / 32767.f;
		sink_front++;
		*sink_front = ((f32)*right_channel) / 32767.f;
		sink_front++;
		left_channel -= 2;
		right_channel -= 2;
	}
}
static void transfer_capture_16bit_1channel(PL_Audio_Input& input)
{
	f32* sink_front = input.sink_buffer;
	void* rb = pl_specific->input_fifo_buffer;

	//shifting existing buffer to right to make room for new frames that are added at beginning of buffer
	int32 no_bytes_per_frame = sizeof(f32) * input.format.no_channels;
	void* sink_end = (uint8*)sink_front + (no_bytes_per_frame * input.no_of_new_frames);
	memmove(sink_end, (void*)sink_front, (input.format.buffer_frame_count - input.no_of_new_frames) * no_bytes_per_frame);

	int16* single_channel = (int16*)rb + (input.format.buffer_frame_count - 1);
	for (uint32 i = 0; i < input.no_of_new_frames; i++)
	{
		*sink_front = ((f32)*single_channel) / 32767.f;
		sink_front++;
		single_channel--;
	}
}
static void transfer_capture_32bit_2channel(PL_Audio_Input& input)
{
	f32* sink_front = input.sink_buffer;
	void* rb = pl_specific->input_fifo_buffer;

	//shifting existing buffer to right to make room for new frames that are added at beginning of buffer
	int32 no_bytes_per_frame = sizeof(f32) * input.format.no_channels;
	void* sink_end = (uint8*)sink_front + (no_bytes_per_frame * input.no_of_new_frames);
	memmove(sink_end, (void*)sink_front, (input.format.buffer_frame_count - input.no_of_new_frames) * no_bytes_per_frame);

	int32* left_channel = (int32*)rb + ((input.format.buffer_frame_count - 1) * 2);
	int32* right_channel = left_channel + 1;
	for (uint32 i = 0; i < input.no_of_new_frames; i++)
	{
		*sink_front = (f32)(((f64)*left_channel) / 2147483647.0);
		sink_front++;
		*sink_front = (f32)(((f64)*right_channel) / 2147483647.0);
		sink_front++;
		left_channel -= 2;
		right_channel -= 2;
	}
}
static void transfer_capture_32bit_1channel(PL_Audio_Input& input)
{
	f32* sink_front = input.sink_buffer;
	void* rb = pl_specific->input_fifo_buffer;

	//shifting existing buffer to right to make room for new frames that are added at beginning of buffer
	int32 no_bytes_per_frame = sizeof(f32) * input.format.no_channels;
	void* sink_end = (uint8*)sink_front + (no_bytes_per_frame * input.no_of_new_frames);
	memmove(sink_end, (void*)sink_front, (input.format.buffer_frame_count - input.no_of_new_frames) * no_bytes_per_frame);

	int32* single_channel = (int32*)rb + (input.format.buffer_frame_count - 1);
	for (uint32 i = 0; i < input.no_of_new_frames; i++)
	{
		*sink_front = (f32)(((f64)*single_channel) / 2147483647.0);
		sink_front++;
		single_channel--;
	}
}

void PL_poll_audio_capture(PL_Audio_Input& input)
{
	uint8 bytes_per_frame = input.format.no_channels * (input.format.no_bits_per_sample / 8);

	void* rb = pl_specific->input_fifo_buffer;

	//Polling audio frames
	uint32 packet_length;
	HRESULT result = pl_specific->input_capture_client->GetNextPacketSize(&packet_length);
	ASSERT(result == S_OK);

	uint8* input_buffer;
	DWORD flags;
	uint32 no_frames_polled = 0;
	uint32 no_frames_in_packet;
	b32 packet_is_silence = FALSE;
	
	while (packet_length != 0)
	{
		HRESULT result = pl_specific->input_capture_client->GetBuffer(&input_buffer, &no_frames_in_packet, &flags, NULL, NULL);
		if (result == AUDCLNT_S_BUFFER_EMPTY)	//device buffer is empty
		{
			ASSERT(FALSE);
			break;
		}
		ASSERT(result == S_OK);

		if (input.is_loopback == TRUE)
		{
			if ((flags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY))
			{
				packet_is_silence = TRUE;
				input_buffer = NULL;
				//This flag is raised if on loopback and nothing is playing. Loopback is just silence. Treat as if silence.  
			}
			else
			{
				packet_is_silence = FALSE;
			}
		}
		else if (flags & AUDCLNT_BUFFERFLAGS_SILENT)
		{
			packet_is_silence = TRUE;
			input_buffer = NULL;  // writing 0 (silence).
			//write silence
		}
		else
		{
			packet_is_silence = FALSE;
		}

		if (packet_is_silence == TRUE)		//writing 0 to the ring buffer
		{
			//shifting buffer left to make room;	
			int32 bytes_to_new_front = (input.format.buffer_frame_count - no_frames_in_packet) * bytes_per_frame;
			memmove(rb, (uint8*)rb + (bytes_per_frame * no_frames_in_packet), bytes_to_new_front);
			//setting packets at end to 0
			memset((uint8*)rb + bytes_to_new_front, 0, no_frames_in_packet * bytes_per_frame);
			no_frames_polled += no_frames_in_packet;
		}
		else
		{	
			//shifting left
			int32 bytes_to_new_front = (input.format.buffer_frame_count - no_frames_in_packet) * bytes_per_frame;
			memmove(rb, (uint8*)rb + (bytes_per_frame * no_frames_in_packet), bytes_to_new_front);
			//copying packet to end of fifo buffer
			memcpy((uint8*)rb + bytes_to_new_front, input_buffer, no_frames_in_packet * bytes_per_frame);
			no_frames_polled += no_frames_in_packet;
		}
		//copy to input_buffer to cyclic buffer 

		result = pl_specific->input_capture_client->ReleaseBuffer(no_frames_in_packet);
		ASSERT(result == S_OK);

		result = pl_specific->input_capture_client->GetNextPacketSize(&packet_length);
		ASSERT(result == S_OK);
	}
	ASSERT(no_frames_polled <= input.format.buffer_frame_count);	//it spent more time polling than the buffer_duration_seconds
	input.no_of_new_frames = no_frames_polled;

	static uint32 frames_polled_since_last_buffer_fill = 0;
	if (input.only_update_every_new_buffer)
	{
		frames_polled_since_last_buffer_fill += no_frames_polled;
		if (frames_polled_since_last_buffer_fill >= input.format.buffer_frame_count)
		{
			frames_polled_since_last_buffer_fill = 0;
			input.no_of_new_frames = input.format.buffer_frame_count;
		}
		else
		{
			return;
		}
	}
	

	//Adding polled audio frames into floating-point sink_buffer
	if (input.no_of_new_frames != 0)	//adding the new frames to the sink_buffer
	{
		//Adding the new frames to the sink_buffer from the fifo buffer
		pl_specific->transfer_to_sink_buffer(input);
	}
	
}
void PL_cleanup_audio_capture(PL_Audio_Input& input)
{
	free(pl_specific->input_fifo_buffer);
	free(input.sink_buffer);
	pl_specific->input_capture_client->Release();
}
//-------------------------------</Audio Capture stuff>----------------------------------

//-------------------------------</Win32 Audio stuff>------------------------------------------




//--------------------------------<Win32 ENTRY POINT>------------------------------------------
//A platform's PL implementation has the job of creating a PL object and a platform_specific object in the main() 
// and calling PL_entry_point to let the application handle how the initialization and game loop runs. 
int WINAPI wWinMain(_In_ HINSTANCE hInstance,
					_In_opt_ HINSTANCE hPrevInstance,
					_In_ LPWSTR    lpCmdLine,
					_In_ int       nCmdShow)
{
	Win32Specific pl_win32 = {};
	pl_win32.hInstance = hInstance;
	
	PL pl = {};
	pl_specific = &pl_win32;
	pl_specific->pointer_to_pl_running = &pl.running;

	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	pl.core_count = sys_info.dwNumberOfProcessors;

	PL_entry_point(pl);
}
//--------------------------------</Win32 ENTRY POINT>------------------------------------------

//--------temp utils--------------------
void debug_print(const char* format, ...)
{
	static char buffer[1024];
	va_list arg_list;
	va_start(arg_list, format);
	vsprintf_s(buffer, sizeof(buffer), format, arg_list);
	va_end(arg_list);
	OutputDebugStringA(buffer);
}

void set_memory(void* buffer, int32 value,int32 size)
{
	memset(buffer, value, size);
}
//--------/temp utils--------------------
