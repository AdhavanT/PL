#pragma once

#include "PL_base_defs.h"

//-------------------------------------------<MEMORY ALLOCATION>-------------------------------------------
void pl_buffer_set(void* buffer, int32 value_to_set, size_t no_bytes_to_set);
void pl_buffer_copy(void* destination, void* from, size_t length);
//allocates new buffer and zero inits 
void* pl_buffer_alloc(size_t size);
//for massive allocations

//For memory arena---------------------
void* pl_arena_buffer_alloc(size_t size);
void* pl_arena_buffer_resize(void* block, size_t old_size, size_t new_size);
void  pl_arena_buffer_free(void* arena_buffer);
//-------------------------------------

//Reallocs to new buffer, zero inits remainder and frees previous memory. 
void* pl_buffer_resize(void* block, size_t new_size);
void pl_buffer_free(void* buffer);
void pl_buffer_move(void* destination, void* source, size_t length);

//-----------------------
// Collections
//----------------------
// DYNAMIC BUFFER
template<typename t, int32 capacity__ = 1, int32 overflow__ = 5, typename size_type = int32>
struct DBuffer
{
	size_type length = 0;
	size_type capacity = capacity__;
	size_type overflow_addon = overflow__;
	t* front = 0;

	inline t* add(t new_member)
	{
		length++;
		if (front == 0)		//Buffer was not initilized and is being initialized here. 
		{
			front = (t*)pl_buffer_alloc(capacity * sizeof(t));
		}
		if (length > capacity)
		{
			capacity = capacity + overflow_addon;
			t* temp = (t*)pl_buffer_resize(front, capacity * sizeof(t));
			ASSERT(temp);	//Not enough memory to realloc, or buffer was never initialized and realloc is trying to allocate to null pointer
			front = temp;
		}

		t* temp = front;
		temp = temp + (length - 1);
		*temp = new_member;
		return temp;
	}

	//Same as add but doesn't perform copy. (Use for BIG objects)
	inline t* add_nocpy(t& new_member)
	{
		length++;
		if (front == 0)		//Buffer was not initilized and is being initialized here. 
		{
			front = (t*)pl_buffer_alloc(capacity * sizeof(t));
		}
		if (length > capacity)
		{
			capacity = capacity + overflow_addon;
			t* temp = (t*)pl_buffer_resize(front, capacity * sizeof(t));
			ASSERT(temp);	//Not enough memory to realloc, or buffer was never initialized and realloc is trying to allocate to null pointer
			front = temp;
		}

		t* temp = front;
		temp = temp + (length - 1);
		*temp = new_member;
		return temp;
	}

	//Clears memory and resets length.
	FORCEDINLINE void clear_buffer()
	{
		if (front != 0)
		{
			pl_buffer_free(front);
			front = 0;
			length = 0;
		}
		//else
		//{
		//	ASSERT(FALSE);	//trying to free freed memory
		//}
	}

	FORCEDINLINE t& operator [](size_type index)
	{
		ASSERT(index >= 0 && index < (size_type)length);
		return (front[index]);
	}
};

// FIXED DYNAMIC BUFFER
//A wrapper for a non-resizable dynamic buffer 
template<typename t, typename size_type = int32>
struct FDBuffer
{
	size_type size = 0;
	t* front = 0;

	//Allocates memory (initilizes to default memory of the type) and returns pointer to allocation
	inline t* allocate_preserve_type_info(int32 size_)
	{
		t tmp;
		t* front_temp = allocate(size_);
		for (int i = 0; i < size_; i++)
		{
			*front_temp++ = tmp;
		}
		return front;
	}

	//Allocates memory (0 initilized) and returns pointer to allocation
	FORCEDINLINE t* allocate(size_type size_)
	{
		size = size_;
		front = (t*)pl_buffer_alloc(size * sizeof(t));
		return front;
	}
	//clears size and deallocates memory 
	FORCEDINLINE void clear()
	{
		if (front != 0)
		{
			pl_buffer_free(front);
			front = 0;
			size = 0;
		}
		//else
		//{
		//	ASSERT(FALSE);	//trying to free freed memory
		//}
	}
	FORCEDINLINE t& operator [](size_type index)
	{
		ASSERT(index >= 0 && index < size);
		return (front[index]);
	}
};

//-------------------------------------------</MEMORY ALLOCATION>-------------------------------------------

//------------------------------------------<THREADING>--------------------------------------------
//Data for a handle to a thread
struct ThreadHandle
{
	void* thread_handle;
};

//What a thread callback function looks like
typedef void (*ThreadProc)(void*);

//creates thread and returns a handle to the thread (only use functions defined in this header to operate on handle)
ThreadHandle pl_create_thread(ThreadProc proc, void* data);

//release thread handle
void pl_close_thread(const ThreadHandle* handle);

//Waits for thread to finish or timeout. Returns TRUE if wait is timed out, and FALSE if thread is finished
b32 pl_wait_for_thread(const ThreadHandle* handle, uint32 timeout_in_ms);

//waits for all threads to be released. Returns TRUE if wait is timed out, and FALSE if all threads are finished
b32 pl_wait_for_all_threads(uint32 no_of_threads, const ThreadHandle* handles, uint32 timeout_in_ms);

//Sleep current thread
void pl_sleep_thread(uint32 timeout_in_ms);

//gets unique thread id
uint32 pl_get_thread_id();


//------------------------------------------</THREADING>--------------------------------------------

//------------------------------------------</RANDOM>-----------------------------------------------
//gets a random uint64 number from system 
//NOTE: this entropy may be biased based on how it is implemented
uint64 pl_get_hardware_entropy();
//------------------------------------------<RANDOM>-----------------------------------------------


//--------------------------------------<FILE I/O>------------------------------------
//returns false if file already exists.
b32 pl_get_file_handle(char* path, void** handle);

#define PL_EOF '\0'
//Will load contents of file from beginning of file to block_to_store_into. 
//Returns false if file doesn't exist or if the bytes read isn't equal to file_size.
b32 pl_load_file_into(void* handle, void* block_to_store_into, uint32 file_size);

//returns true if successfully created and loads file_handle. false if file already exists.
b32 pl_create_file(void** file_handle, char* path);

//appends to file. returns true if successful.
b32 pl_append_to_file(void* file_handle, void* block_to_store, int32 bytes_to_append);

b32 pl_close_file_handle(void* file_handle);

//returns file size in bytes
uint64 pl_get_file_size(void* handle);

//--------------------------------------</FILE I/O>------------------------------------

//--------------------------------------<TIMING>---------------------------------------

uint64 pl_get_tsc();
//--------------------------------------</TIMING>--------------------------------------


//--------------------------------------<DEBUG>----------------------------------------
void pl_throw_error_box(const char* error);

#include <cstdarg>
void pl_debug_print(const char* format, ...);
void pl_format_print(char* buffer, uint32 buffer_size, const char* format, ...);
//--------------------------------------</DEBUG>---------------------------------------
