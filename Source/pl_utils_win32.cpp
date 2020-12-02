#include "pl_utils.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <corecrt_malloc.h>
#include <cstdio>	//for file io stuff


uint32 get_thread_id()
{
	return GetCurrentThreadId();
}

uint64 get_hardware_entropy()
{
	//TODO: consider using rdrand intrinsic for proper hardware entropy
	//right now, just gets current time stamp counter and multiplies with thread id (to make "thread safe" I guess)
	return __rdtsc() * GetCurrentThreadId();
}

int32 interloacked_add_i32(volatile int32* data, int32 value)
{
	return InterlockedAdd((volatile long*)data, value);
}

int64 interlocked_add_i64(volatile int64* data, int64 value)
{
	return InterlockedAdd64(data, value);
}

int64 interlocked_decrement_i32(volatile int32* data)
{
	return InterlockedDecrement((volatile long*)data);
}

int64 interlocked_increment_i32(volatile int32* data)
{
	return InterlockedIncrement((volatile long*)data);
}

int64 interlocked_decrement_i64(volatile int64* data)
{
	return InterlockedDecrement64(data);
}

int64 interlocked_increment_i64(volatile int64* data)
{
	return InterlockedIncrement64(data);
}

void close_thread(const ThreadHandle* handle)
{
	CloseHandle(handle->thread_handle);
}

void close_threads(uint32 no_of_threads, const ThreadHandle* handles)
{
	for (uint32 i = 0; i < no_of_threads; i++)
	{
		CloseHandle(handles[i].thread_handle);
	}
}

void wait_for_thread(const ThreadHandle* handle, uint32 timeout_in_ms)
{
	WaitForSingleObject((HANDLE*)handle, timeout_in_ms);
}

void wait_for_all_threads(uint32 no_of_threads, const ThreadHandle* handles, uint32 timeout_in_ms)
{
	WaitForMultipleObjects(no_of_threads, (HANDLE*)handles, TRUE, timeout_in_ms);
}


struct CreateThreadData
{
	ThreadProc func_to_be_executed;
	void* data;
};

static DWORD WINAPI win32_start_thread(__in LPVOID lpParameter)
{
	CreateThreadData* new_thread_data = (CreateThreadData*)lpParameter;
	new_thread_data->func_to_be_executed(new_thread_data->data);
	buffer_free(new_thread_data);
	return 0;
}

void buffer_set(void* buffer, int32 value_to_set, int32 no_bytes_to_set)
{
	memset(buffer, value_to_set, no_bytes_to_set);
}


void buffer_copy(void* destination, void* from, uint32 length)
{
	CopyMemory(destination, from, length);
}

void* buffer_malloc(size_t size)
{
	return malloc(size);
}

void* buffer_calloc(size_t size)
{
	return calloc(1, size);
}

void* buffer_realloc(void* block, size_t new_size)
{
	return realloc(block, new_size);
}

void buffer_free(void* buffer)
{
	free(buffer);
}

ThreadHandle create_thread(ThreadProc proc, void* data)
{
	CreateThreadData* new_thread_data = (CreateThreadData*)buffer_malloc(sizeof(CreateThreadData));
	new_thread_data->func_to_be_executed = proc;
	new_thread_data->data = data;

	DWORD thread_id;
	HANDLE new_thread_handle = CreateThread(0, 0, win32_start_thread, (void*)new_thread_data, 0, &thread_id);
	ThreadHandle handle;
	handle.thread_handle = new_thread_handle;
	return handle;
}

void load_file_into(void* block_to_store_into, uint32 bytes_to_load,char* path)
{
	void* file;
	errno_t error = fopen_s((FILE**)&file, path, "rb");
	if (error != 0)
	{
		ASSERT(FALSE);	//cant read file
	}
	fread(block_to_store_into, bytes_to_load, 1, (FILE*)file);
	b32 result = fclose((FILE*)file);
	if (result != 0)
	{
		ASSERT(FALSE);	//cant close file!
	}
}

uint32 get_file_size(char* path)
{
	void* file;
	errno_t error = fopen_s((FILE**)&file, path, "rb");
	if (error != 0)
	{
		ASSERT(FALSE);	//cant open file!
	}
	uint32 current_pos, end;
	current_pos = ftell((FILE*)file);
	fseek((FILE*)file, 0, SEEK_END);
	end = ftell((FILE*)file);
	fseek((FILE*)file, current_pos, SEEK_SET);

	b32 result = fclose((FILE*)file);
	if (result != 0)
	{
		ASSERT(FALSE);	//cant close file!
	}
	
	return end;
}

void debug_print(const char* format, ...)
{
	static char buffer[1024];
	va_list arg_list;
	va_start(arg_list, format);
	vsprintf_s(buffer, sizeof(buffer), format, arg_list);
	va_end(arg_list);
	OutputDebugStringA(buffer);
}