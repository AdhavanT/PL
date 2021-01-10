	#include "pl_utils.h"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

uint32 pl_get_thread_id()
{
	return GetCurrentThreadId();
}

uint64 pl_get_hardware_entropy()
{
	//TODO: consider using rdrand intrinsic for proper hardware entropy
	//right now, just gets current time stamp counter and multiplies with thread id (to make "thread safe" I guess)
	return __rdtsc() * GetCurrentThreadId();
}

void pl_close_thread(const ThreadHandle* handle)
{
	CloseHandle(handle->thread_handle);
}


b32 pl_wait_for_thread(const ThreadHandle* handle, uint32 timeout_in_ms)
{
	return WaitForSingleObject((HANDLE*)handle, timeout_in_ms);
}

b32 pl_wait_for_all_threads(uint32 no_of_threads, const ThreadHandle* handles, uint32 timeout_in_ms)
{
	return WaitForMultipleObjects(no_of_threads, (HANDLE*)handles, TRUE, timeout_in_ms);
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
	pl_buffer_free(new_thread_data);
	return 0;
}

static HANDLE process_heap = GetProcessHeap();

void pl_buffer_set(void* buffer, int32 value_to_set, size_t no_bytes_to_set)
{
	FillMemory(buffer, no_bytes_to_set, value_to_set);
}

void pl_buffer_copy(void* destination, void* from, size_t length)
{
	CopyMemory(destination, from, length);
}

void pl_buffer_move(void* destination, void* source, size_t length)
{
	MoveMemory(destination, source, length);
}

void* pl_buffer_alloc(size_t size)
{
	return HeapAlloc(process_heap, HEAP_ZERO_MEMORY, size);
}

//NOTE: implementation needs to perform realloc() and free previous buffer. HeapReAlloc() does both.  
void* pl_buffer_resize(void* block, size_t new_size)
{
	return HeapReAlloc(process_heap, HEAP_ZERO_MEMORY, block, new_size);
}

void pl_buffer_free(void* buffer)
{
	HeapFree(process_heap,0,buffer);
}

ThreadHandle pl_create_thread(ThreadProc proc, void* data)
{
	CreateThreadData* new_thread_data = (CreateThreadData*)pl_buffer_alloc(sizeof(CreateThreadData));
	new_thread_data->func_to_be_executed = proc;
	new_thread_data->data = data;

	DWORD thread_id;
	HANDLE new_thread_handle = CreateThread(0, 0, win32_start_thread, (void*)new_thread_data, 0, &thread_id);
	ThreadHandle handle;
	handle.thread_handle = new_thread_handle;
	return handle;
}

void pl_sleep_thread(uint32 timeout_in_ms)
{
	Sleep((DWORD)timeout_in_ms);
}


b32 pl_load_file_into(void* handle,void* block_to_store_into, uint32 file_size)
{
	DWORD read_bytes;
	b32 success;
	success = ReadFile(handle, block_to_store_into, file_size, &read_bytes, 0);
	if (success == 0 || read_bytes != file_size)
	{
		return FALSE;
	}
	return TRUE;
	//void* file;
	//errno_t error = fopen_s((FILE**)&file, path, "rb");
	//if (error != 0)
	//{
	//	ASSERT(FALSE);	//cant read file
	//}
	//fread(block_to_store_into, bytes_to_load, 1, (FILE*)file);
	//b32 result = fclose((FILE*)file);
	//if (result != 0)
	//{
	//	ASSERT(FALSE);	//cant close file!
	//}
}

b32 pl_create_file(void** file_handle,char* path)
{
	*file_handle = CreateFileA(path, GENERIC_WRITE, 0, 0, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, 0);
	if (INVALID_HANDLE_VALUE == file_handle)
	{
		return FALSE;
	}
	else if (GetLastError() == ERROR_FILE_EXISTS)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}

b32 pl_append_to_file(void* file_handle, void* block_to_store, int32 bytes_to_append)
{
	LARGE_INTEGER offset;
	offset.QuadPart = 0;
	SetFilePointerEx(file_handle, offset, 0, FILE_END);
	return WriteFile(file_handle, block_to_store, bytes_to_append, 0, 0);
}

//returns false if file already exists.
b32 pl_get_file_handle(char* path, void** handle)
{
	*handle = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (*handle == INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	return TRUE;
}

b32 pl_close_file_handle(void* file_handle)
{
	return CloseHandle(file_handle);
}

uint64 pl_get_file_size(void* handle)
{
	LARGE_INTEGER file_size;
	GetFileSizeEx(handle, &file_size);
	return file_size.QuadPart;
	//void* file;
	//errno_t error = fopen_s((FILE**)&file, path, "rb");
	//if (error != 0)
	//{
	//	ASSERT(FALSE);	//cant open file!
	//}
	//uint32 current_pos, end;
	//current_pos = ftell((FILE*)file);
	//fseek((FILE*)file, 0, SEEK_END);
	//end = ftell((FILE*)file);
	//fseek((FILE*)file, current_pos, SEEK_SET);

	//b32 result = fclose((FILE*)file);
	//if (result != 0)
	//{
	//	ASSERT(FALSE);	//cant close file!
	//}
	
	//return end;
}

uint64 pl_get_tsc()
{
	LARGE_INTEGER tsc;
	QueryPerformanceCounter(&tsc);
	return tsc.QuadPart;
}

#include <cstdio>
void pl_debug_print(const char* format, ...)
{
	static char buffer[1024];
	va_list arg_list;
	va_start(arg_list, format);
	vsprintf_s(buffer, sizeof(buffer), format, arg_list);
	va_end(arg_list);
	OutputDebugStringA(buffer);
}

void pl_format_print(char* buffer, uint32 buffer_size, const char* format, ...)
{
	va_list arg_list;
	va_start(arg_list, format);
	vsprintf_s(buffer, buffer_size, format, arg_list);
	va_end(arg_list);
}