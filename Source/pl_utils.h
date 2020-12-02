#pragma once

#include "pl.h"
//-------------------------------------------<MEMORY ALLOCATION>-------------------------------------------
void buffer_set(void* buffer, int32 value_to_set, int32 no_bytes_to_set);
void buffer_copy(void* destination, void* from, uint32 length);
void* buffer_malloc(size_t size);
void* buffer_calloc(size_t size);
void* buffer_realloc(void* block, size_t new_size);
void buffer_free(void* buffer);
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
ThreadHandle create_thread(ThreadProc proc, void* data);

//release thread handle
void close_thread(const ThreadHandle* handle);

//release thread handles
void close_threads(uint32 no_of_threads, const ThreadHandle* handles);

//Waits for thread to finish or timeout
void wait_for_thread(const ThreadHandle* handle, uint32 timeout_in_ms);

//waits for all threads to be released
void wait_for_all_threads(uint32 no_of_threads, const ThreadHandle* handles, uint32 timeout_in_ms);

//gets unique thread id
uint32 get_thread_id();

//performs atomic add and returns result(for int64 value)
int64 interlocked_add_i64(volatile int64*, int64);
//performs atomic add and returns result(for int32 value)
int32 interloacked_add_i32(volatile int32* data, int32 value);


//returns resulting incremented value after performing locked increment(for int64 value)
int64 interlocked_increment_i64(volatile int64*);
//returns resulting decremented value after performing locked decrement(for int32 value)
int64 interlocked_increment_i32(volatile int32* data);


//returns resulting decremented value after performing locked decrement(for int64 value)
int64 interlocked_decrement_i64(volatile int64* data);
//returns resulting decremented value after performing locked decrement(for int32 value)
int64 interlocked_decrement_i32(volatile int32* data);

//------------------------------------------</THREADING>--------------------------------------------

//------------------------------------------</RANDOM>-----------------------------------------------
//gets a random uint64 number from system 
//NOTE: this entropy may be biased based on how it is implemented
uint64 get_hardware_entropy();
//------------------------------------------<RANDOM>-----------------------------------------------


//--------------------------------------<FILE I/O>------------------------------------

//Will load contents of file from beginning of file to block_to_store_into
void load_file_into(void* block_to_store_into, uint32 bytes_to_load, char* path);

//returns file size in bytes
uint32 get_file_size(char* path);

//--------------------------------------</FILE I/O>------------------------------------


//--------------------------------------<DEBUG>----------------------------------------
#include <cstdarg>
void debug_print(const char* format, ...);
//--------------------------------------</DEBUG>---------------------------------------
