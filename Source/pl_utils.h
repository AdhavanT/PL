#pragma once

#include "PL_base_defs.h"

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

//Waits for thread to finish or timeout. Returns TRUE if wait is timed out, and FALSE if thread is finished
b32 wait_for_thread(const ThreadHandle* handle, uint32 timeout_in_ms);

//waits for all threads to be released. Returns TRUE if wait is timed out, and FALSE if all threads are finished
b32 wait_for_all_threads(uint32 no_of_threads, const ThreadHandle* handles, uint32 timeout_in_ms);

//Sleep current thread
void sleep_thread(uint32 timeout_in_ms);

//gets unique thread id
uint32 get_thread_id();


//------------------------------------------</THREADING>--------------------------------------------

//------------------------------------------</RANDOM>-----------------------------------------------
//gets a random uint64 number from system 
//NOTE: this entropy may be biased based on how it is implemented
uint64 get_hardware_entropy();
//------------------------------------------<RANDOM>-----------------------------------------------


//--------------------------------------<FILE I/O>------------------------------------

//Will load contents of file from beginning of file to block_to_store_into
void load_file_into(void* block_to_store_into, uint32 bytes_to_load, char* path);

//returns true if successfully created and written. false if file already exists.
b32 create_and_load_into_file(void* block_to_store, uint32 bytes_to_write, char* path);

//returns file size in bytes
uint32 get_file_size(char* path);

//--------------------------------------</FILE I/O>------------------------------------

//--------------------------------------<TIMING>---------------------------------------

uint64 get_tsc();
//--------------------------------------</TIMING>--------------------------------------


//--------------------------------------<DEBUG>----------------------------------------
#include <cstdarg>
void debug_print(const char* format, ...);
void format_print(char* buffer, uint32 buffer_size,const char* format, ...);
//--------------------------------------</DEBUG>---------------------------------------
