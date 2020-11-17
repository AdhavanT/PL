#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <windowsx.h>
#include <malloc.h>
#include "pl.h"
#include <stdio.h>

struct RingBuffer
{
	void* buffer_front;
	int32 front;
	int32 size;
};

struct Win32Specific
{
	HINSTANCE hInstance;
	//Window stuff
	BITMAPINFO bmi_header;
	HDC main_monitor_DC;
	HWND wnd_handle;
	void* main_fiber;
	void* message_fiber;

	//Audio stuff
	IAudioClient *input;
	RingBuffer input_ring_buffer;
	IAudioCaptureClient* input_capture_client;
	IAudioClient *output;
};
#define WIN32_SPECIFIC(x) ((Win32Specific*)x.platform_specific)

//-------------------------------<win32 window stuff>----------------------------------------
//Also handles Keyboard and Mouse input polling. 
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
	wnd_class.hInstance = WIN32_SPECIFIC(pl)->hInstance;
	wnd_class.hCursor = LoadCursorA(NULL, MAKEINTRESOURCEA(32515));

	HRESULT s = RegisterClassA(&wnd_class);
	ASSERT(s != 0);

	WIN32_SPECIFIC(pl)->wnd_handle = CreateWindowExA(
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
	ASSERT(WIN32_SPECIFIC(pl)->wnd_handle);
	//ShowWindow(WIN32_SPECIFIC(pl)->wnd_handle, SW_SHOW);

	//This passes a pointer to pl to the wnd_proc message callback. (It's retrieved by GetWindowLongPtrA(hwnd, GWLP_USERDATA))
	SetWindowLongPtrA(WIN32_SPECIFIC(pl)->wnd_handle, GWL_USERDATA, (LONG_PTR)&pl);
	PL* re = (PL*)GetWindowLongPtrA(WIN32_SPECIFIC(pl)->wnd_handle, GWLP_USERDATA);
	
	WIN32_SPECIFIC(pl)->main_fiber = ConvertThreadToFiber(0);
	ASSERT(WIN32_SPECIFIC(pl)->main_fiber);
	WIN32_SPECIFIC(pl)->message_fiber = CreateFiber(0, (PFIBER_START_ROUTINE)wnd_message_fiber_proc, &pl);
	ASSERT(WIN32_SPECIFIC(pl)->message_fiber);

	WIN32_SPECIFIC(pl)->main_monitor_DC = GetDC(WIN32_SPECIFIC(pl)->wnd_handle);


	PL_poll_window(pl);
	

}

//a fiber to handle messages to keep main loop going while doing so.
//A 1ms timer alerts the message callback when to switch to the main loop fiber.
void CALLBACK wnd_message_fiber_proc(PL& pl)
{
	//Sets a timer for 1ms that sends a WM_TIMER message to the message queue
	SetTimer(WIN32_SPECIFIC(pl)->wnd_handle, 1, 1, 0);

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
		SwitchToFiber(WIN32_SPECIFIC(pl)->main_fiber);
	}
	SwitchToFiber(WIN32_SPECIFIC(pl)->main_fiber);

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
		case WM_ENTERSIZEMOVE:
		{
			pl->window.was_altered = TRUE;	
			SetTimer(hwnd, 1, 1, 0);
		}break;
		case WM_EXITSIZEMOVE:
		{
			pl->window.was_altered = FALSE;
			KillTimer(hwnd, 1);

		}break;
		case WM_MOUSEMOVE:
		{
			POINT pos;
			pos.x = GET_X_LPARAM(lParam);
			pos.y = GET_Y_LPARAM(lParam);
			pl->input.mouse.position_x = pos.x;
			pl->input.mouse.position_y = pl->window.height - pos.y;	//to be consistant with bottom left being (0,0)
			

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
	SwitchToFiber(WIN32_SPECIFIC(pl)->message_fiber);

	if (pl.window.was_altered || (pl.initialized == FALSE))
	{
		//NOTE:not gettin device context every time window moves. Checked with multi-monitor setup, worked fine with single device context. 
		//WIN32_SPECIFIC(pl)->main_monitor_DC = GetDC(WIN32_SPECIFIC(pl)->wnd_handle);

		RECT client_rectangle = {};
		GetClientRect(WIN32_SPECIFIC(pl)->wnd_handle, &client_rectangle);

		pl.window.width = client_rectangle.right - client_rectangle.left;
		pl.window.height = client_rectangle.bottom - client_rectangle.top;

		POINT window_position = {};
		ClientToScreen(WIN32_SPECIFIC(pl)->wnd_handle, &window_position);

		pl.window.position_x = window_position.x;
		pl.window.position_y = window_position.y;

		//FIXME: This clears the window to black on resizing to avoid the extra window(outside bitmap) from copying the previous frame. 
		//It also causes random black frames while resizing.
		//PatBlt(WIN32_SPECIFIC(pl)->main_monitor_DC, 0, 0, pl.window.width, pl.window.height, BLACKNESS);
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
		sprintf_s(buffer, "Time per frame: %.*fms , %dFPS\n",2, (f64)pl.time.fdelta_seconds * 1000, frame_rate);
		SetWindowTextA(WIN32_SPECIFIC(pl)->wnd_handle, buffer);
		timing_refresh = pl.time.fcurrent_seconds;
	}

	//clearing the screen to black before drawing
	//PatBlt(WIN32_SPECIFIC(pl)->main_monitor_DC, 0, 0, pl.window.width, pl.window.height, WHITENESS);

	//NOTE:Making sure to keep the buffer-to-monitor pixel mapping consistant. (not stretching the image to fit window.)
	StretchDIBits(
		WIN32_SPECIFIC(pl)->main_monitor_DC,
		0, 0, pl.bitmap.width, pl.bitmap.height,
		0, 0, pl.bitmap.width, pl.bitmap.height,
		pl.bitmap.buffer,
		&WIN32_SPECIFIC(pl)->bmi_header,
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

//-------------------------------<Input stuff>------------------------------------------------
//NOTE: For mouse input, WM_MOUSEMOVE is used. This is less precise than WM_INPUT and only changes on 
//when the mouse moves to another pixel. Consider using raw mouse data from WM_INPUT for more 
//high-definition mouse input for applications that need it. 
void PL_initialize_input(PL& pl)
{

}

void PL_poll_input(PL& pl)
{

}

//-------------------------------</Input stuff>-----------------------------------------------

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

	BITMAPINFO* bmi = &WIN32_SPECIFIC(pl)->bmi_header;
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

	IAudioClient** output_audio_client = &WIN32_SPECIFIC(pl)->output;
	IAudioClient** input_audio_client = &WIN32_SPECIFIC(pl)->input;


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

	result = output_endpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, 0, (void**)output_audio_client);
	ASSERT(result == S_OK);

	result = input_endpoint->Activate(__uuidof(IAudioClient), CLSCTX_ALL, 0, (void**)input_audio_client);
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
		pl.audio.input.format.buffer_duration_seconds = 1.0f;
	}
	else if (pl.audio.input.format.buffer_duration_seconds == 0 && pl.audio.input.format.buffer_frame_count != 0)
	{
		pl.audio.input.format.buffer_duration_seconds = REFTIMES_PER_SEC * (f32)pl.audio.input.format.buffer_frame_count / (f32)pl.audio.input.format.samples_per_second;
	}

	uint32 duration = (uint32)(pl.audio.input.format.buffer_duration_seconds * (f32)REFTIMES_PER_SEC);
	result = (*input_audio_client)->Initialize(AUDCLNT_SHAREMODE_SHARED, input_stream_flags, duration, 0, &ipf, 0);
	ASSERT(result == S_OK);

	result = (*input_audio_client)->GetBufferSize(&pl.audio.input.format.buffer_frame_count);
	ASSERT(result == S_OK);

	pl.audio.input.format.buffer_duration_seconds = (f32)pl.audio.input.format.buffer_frame_count / (f32)pl.audio.input.format.samples_per_second;
	#undef REFTIMES_PER_SEC

	result = (*input_audio_client)->GetService(__uuidof(IAudioCaptureClient), (void**)&WIN32_SPECIFIC(pl)->input_capture_client);
	ASSERT(result == S_OK);

	result = (*input_audio_client)->Start();
	ASSERT(result == S_OK);

	uint8 bytes_per_frame = pl.audio.input.format.no_channels * (pl.audio.input.format.no_bits_per_sample / 8);

	WIN32_SPECIFIC(pl)->input_ring_buffer.size = pl.audio.input.format.buffer_frame_count;
	WIN32_SPECIFIC(pl)->input_ring_buffer.buffer_front = calloc(1, (WIN32_SPECIFIC(pl)->input_ring_buffer.size * bytes_per_frame));
	WIN32_SPECIFIC(pl)->input_ring_buffer.front = 0;

	pl.audio.input.sink_buffer = (f32*)calloc(1,pl.audio.input.format.buffer_frame_count * sizeof(f32) * pl.audio.input.format.no_channels);

	PL_poll_audio(pl);
}


void PL_poll_audio(PL& pl)
{

	uint8 bytes_per_frame = pl.audio.input.format.no_channels * (pl.audio.input.format.no_bits_per_sample / 8);

	RingBuffer* rb = &WIN32_SPECIFIC(pl)->input_ring_buffer;
	f32* sink_front = pl.audio.input.sink_buffer;

	//Polling audio framess
	uint32 packet_length;
	HRESULT result = WIN32_SPECIFIC(pl)->input_capture_client->GetNextPacketSize(&packet_length);
	ASSERT(result == S_OK);

	uint8* input_buffer;
	DWORD flags;
	uint32 no_frames_polled = 0;
	uint32 no_frames_in_packet;
	b32 packet_is_silence = FALSE;
	while (packet_length != 0)
	{
		HRESULT result = WIN32_SPECIFIC(pl)->input_capture_client->GetBuffer(&input_buffer, &no_frames_in_packet, &flags, NULL, NULL);
		if (result == AUDCLNT_S_BUFFER_EMPTY)	//device buffer is empty
		{
			ASSERT(FALSE);
			break;
		}
		ASSERT(result == S_OK);

		if (pl.audio.input.is_loopback == TRUE)
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
			int32 new_front = no_frames_in_packet + rb->front;
			b32 fits = new_front <= rb->size;
			if (fits)
			{
				void* to = (uint8*)rb->buffer_front + (bytes_per_frame * rb->front);
				memset(to, 0, no_frames_in_packet * bytes_per_frame);
				rb->front = new_front % rb->size;
			}
			else
			{
				uint32 fill_buffer = rb->size - rb->front;
				void* to = (uint8*)rb->buffer_front + (rb->front * bytes_per_frame);
				memset(to, 0, fill_buffer * bytes_per_frame);			//filling to end of buffer
				rb->front = new_front - rb->size;
				memset(rb->buffer_front, 0, bytes_per_frame * rb->front);	//filling remaining
			}

			no_frames_polled += no_frames_in_packet;
		}
		else
		{
			//adding it to the ring buffer
			int32 new_front = no_frames_in_packet + rb->front;
			b32 fits = new_front <= rb->size;
			if (fits)
			{
				void* to = (uint8*)rb->buffer_front + (bytes_per_frame * rb->front);
				memcpy(to, input_buffer, no_frames_in_packet * bytes_per_frame);
				rb->front = new_front % rb->size;
			}
			else
			{
				uint32 fill_buffer = rb->size - rb->front;
				void* to = (uint8*)rb->buffer_front + (rb->front * bytes_per_frame);
				memcpy(to, input_buffer, fill_buffer * bytes_per_frame);			//filling to end of buffer
				rb->front = new_front - rb->size;
				memcpy(rb->buffer_front, input_buffer, bytes_per_frame * rb->front);	//filling remaining
			}
			no_frames_polled += no_frames_in_packet;
		}
		//copy to input_buffer to cyclic buffer 

		result = WIN32_SPECIFIC(pl)->input_capture_client->ReleaseBuffer(no_frames_in_packet);
		ASSERT(result == S_OK);

		result = WIN32_SPECIFIC(pl)->input_capture_client->GetNextPacketSize(&packet_length);
		ASSERT(result == S_OK);
	}
	ASSERT(no_frames_polled <= pl.audio.input.format.buffer_frame_count);	//it spent more time polling than the buffer_duration_seconds
	pl.audio.input.no_of_new_frames = no_frames_polled;

	static uint32 polled_since_last_frame = 0;
	if (pl.audio.input.only_update_every_new_buffer)
	{
		polled_since_last_frame += no_frames_polled;
		if (polled_since_last_frame >= pl.audio.input.format.buffer_frame_count)
		{
			polled_since_last_frame = 0;
			pl.audio.input.no_of_new_frames = pl.audio.input.format.buffer_frame_count;
		}
		else
		{
			return;
		}
	}

	//Adding polled audio frames into floating-point sink_buffer
	if (pl.audio.input.no_of_new_frames != 0)	//adding the new frames to the sink_buffer
	{
		//shifting existing buffer to right to make room for new frames that are added at beginning of buffer
		//getting the new_end of the buffer : buffer_size - amount_to_shift - 1
		int32 no_floats_to_shift = (pl.audio.input.no_of_new_frames * pl.audio.input.format.no_channels);
		int32 new_end = (pl.audio.input.format.buffer_frame_count * pl.audio.input.format.no_channels) - no_floats_to_shift;
		for (int32 i = 0; i < new_end; i++)
		{
			sink_front[i + no_floats_to_shift] = sink_front[i];	//resverse to shift left
		}
		//NOTE: This is for a forward buffer where the newest frame is at the beginning of the sink buffer
		//converting/transfering the newly added frames into floating point sink buffer from fixed point ring buffer
		if(pl.audio.input.format.no_bits_per_sample == 16 && pl.audio.input.format.no_channels == 2)
		{
			int32 frames_from_front = rb->front - pl.audio.input.no_of_new_frames;
			if (frames_from_front >= 0)	//the amount polled is still in beginning of buffer. no need to get from cycle
			{
				int16* left_channel = (int16*)(rb->buffer_front);
				left_channel = left_channel + 2*(rb->front - 1);	//-1 to account for rb->front pointing the the last frame not, the first. 
				int16* right_channel = left_channel + 1;
				for (int32 i = pl.audio.input.no_of_new_frames; i > 0; i--)
				{
					*sink_front = ((f32)*left_channel) / 32767.f;
					sink_front++;
					*sink_front = ((f32)*right_channel) / 32767.f;
					sink_front++;
					left_channel-=2;
					right_channel -= 2;
				}
			}
			else   //need to read front front of ring buffer then cycle back to end to front agian. 
			{
				int16* left_channel = (int16*)(rb->buffer_front);
				left_channel = left_channel + 2 * (rb->front - 1);	//-1 to account for rb->front pointing the the last frame not, the first. 
				int16* right_channel = left_channel + 1;
				for (int32 i = rb->front; i > 0; i--)
				{
					*sink_front = ((f32)*left_channel) / 32767.f;
					sink_front++;
					*sink_front = ((f32)*right_channel) / 32767.f;
					sink_front++;
					left_channel -= 2;
					right_channel -= 2;
				}

				left_channel = (int16*)(rb->buffer_front);
				left_channel = left_channel + 2*(rb->size - 1);
				right_channel = left_channel + 1;
				
				for (int32 i = -frames_from_front; i > 0; i--)	//-frames_from_front is the remaining new_frames after sampling from front
				{
					*sink_front = ((f32)*left_channel) / 32767.f;
					sink_front++;
					*sink_front = ((f32)*right_channel) / 32767.f;
					sink_front++;
					left_channel -= 2;
					right_channel -= 2;
				}
				
			}
		}
		else if(pl.audio.input.format.no_bits_per_sample == 16 && pl.audio.input.format.no_channels == 1)
		{
			int32 frames_from_front = rb->front - pl.audio.input.no_of_new_frames;
			if (frames_from_front >= 0)	//the amount polled is still in beginning of buffer. no need to get from cycle
			{
			    int16* single_channel = (int16*)(rb->buffer_front);
				single_channel = single_channel + rb->front - 1;	//-1 to account for rb->front pointing the the last frame not, the first. 
				for (int32 i = pl.audio.input.no_of_new_frames; i > 0; i--)
				{
					*sink_front = ((f32)*single_channel) / 32767.f;
					single_channel--;
					sink_front++;
				}
			}
			else   //need to read front front of ring buffer then cycle back to end to front agian. 
			{
				int16* single_channel = (int16*)(rb->buffer_front);
				single_channel = single_channel + rb->front - 1;	//-1 to account for rb->front pointing the the last frame not, the first. 
				for (int32 i = rb->front; i > 0; i--)
				{
					*sink_front = ((f32)*single_channel) / 32767.f;
					single_channel--;
					sink_front++;
				}
				single_channel = (int16*)(rb->buffer_front);
				single_channel = single_channel + rb->size - 1;
				{
					for (int32 i = -frames_from_front; i > 0; i--)	//-frames_from_front is the remaining new_frames after sampling from front
					{
						*sink_front = ((f32)*single_channel) / 32767.f;
						single_channel--;
						sink_front++;
					}
				}
			}
		}
		else if(pl.audio.input.format.no_bits_per_sample == 32 && pl.audio.input.format.no_channels == 1)
		{
			int32 frames_from_front = rb->front - pl.audio.input.no_of_new_frames;
			if (frames_from_front >= 0)	//the amount polled is still in beginning of buffer. no need to get from cycle
			{
				int32* single_channel = (int32*)(rb->buffer_front);
				single_channel = single_channel + rb->front - 1;	//-1 to account for rb->front pointing the the last frame not, the first. 
				for (int32 i = pl.audio.input.no_of_new_frames; i > 0; i--)
				{
					f64 for_buffer;
					for_buffer = ((f64)*single_channel) / 2147483647.0;
					*sink_front = (f32)for_buffer;
					single_channel--;
					sink_front++;
				}
			}
			else   //need to read front front of ring buffer then cycle back to end to front agian. 
			{
				int32* single_channel = (int32*)(rb->buffer_front);
				single_channel = single_channel + rb->front - 1;	//-1 to account for rb->front pointing the the last frame not, the first. 
				for (int32 i = rb->front; i > 0; i--)
				{
					f64 for_buffer;
					for_buffer = ((f64)*single_channel) / 2147483647.0;
					*sink_front = (f32)for_buffer;
					single_channel--;
					sink_front++;
				}
				single_channel = (int32*)(rb->buffer_front);
				single_channel = single_channel + rb->size - 1;
				{
					for (int32 i = -frames_from_front; i > 0; i--)	//-frames_from_front is the remaining new_frames after sampling from front
					{
						f64 for_buffer;
						for_buffer = ((f64)*single_channel) / 2147483647.0;
						*sink_front = (f32)for_buffer;
						single_channel--;
						sink_front++;
					}
				}
			}
		}
		else if(pl.audio.input.format.no_bits_per_sample == 32 && pl.audio.input.format.no_channels == 2)
		{
			int32 frames_from_front = rb->front - pl.audio.input.no_of_new_frames;
			if (frames_from_front >= 0)	//the amount polled is still in beginning of buffer. no need to get from cycle
			{
				int32* left_channel = (int32*)(rb->buffer_front);
				left_channel = left_channel + 2 * (rb->front - 1);	//-1 to account for rb->front pointing the the last frame not, the first. 
				int32* right_channel = left_channel + 1;
				for (int32 i = pl.audio.input.no_of_new_frames; i > 0; i--)
				{
					f64 for_buffer;
					for_buffer = ((f64)*left_channel) / 2147483647.0;
					*sink_front = (f32)for_buffer;
					sink_front++;
					for_buffer = ((f64)*right_channel) / 2147483647.0;
					*sink_front = (f32)for_buffer;
					sink_front++;
					left_channel -= 2;
					right_channel -= 2;
				}
			}
			else   //need to read front front of ring buffer then cycle back to end to front agian. 
			{
				int32* left_channel = (int32*)(rb->buffer_front);
				left_channel = left_channel + 2 * (rb->front - 1);	//-1 to account for rb->front pointing the the last frame not, the first. 
				int32* right_channel = left_channel + 1;
				for (int32 i = rb->front; i > 0; i--)
				{
					f64 for_buffer;
					for_buffer = ((f64)*left_channel) / 2147483647.0;
					*sink_front = (f32)for_buffer;
					sink_front++;
					for_buffer = ((f64)*right_channel) / 2147483647.0;
					*sink_front = (f32)for_buffer;
					sink_front++;
					left_channel -= 2;
					right_channel -= 2;
				}

				left_channel = (int32*)(rb->buffer_front);
				left_channel = left_channel + 2 * (rb->size - 1);
				right_channel = left_channel + 1;

				for (int32 i = -frames_from_front; i > 0; i--)	//-frames_from_front is the remaining new_frames after sampling from front
				{
					f64 for_buffer;
					for_buffer = ((f64)*left_channel) / 2147483647.0;
					*sink_front = (f32)for_buffer;
					sink_front++;
					for_buffer = ((f64)*right_channel) / 2147483647.0;
					*sink_front = (f32)for_buffer;
					sink_front++;
					left_channel -= 2;
					right_channel -= 2;
				}

			}
		}
	}
	
	
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

	SYSTEM_INFO sys_info;
	GetSystemInfo(&sys_info);
	pl.core_count = sys_info.dwNumberOfProcessors;

	PL_entry_point(pl);
}
//--------------------------------</Win32 ENTRY POINT>------------------------------------------
#undef WIN32_SPECIFIC
