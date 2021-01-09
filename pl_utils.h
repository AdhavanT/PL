#pragma once

#include "PL_base_defs.h"

//-------------------------------------------<MEMORY ALLOCATION>-------------------------------------------
void pl_buffer_set(void* buffer, int32 value_to_set, int32 no_bytes_to_set);
void pl_buffer_copy(void* destination, void* from, uint32 length);
//allocates new buffer and zero inits 
void* pl_buffer_alloc(size_t size);
//Reallocs to new buffer, zero inits remainder and frees previous memory. 
void* pl_buffer_resize(void* block, size_t new_size);
void pl_buffer_free(void* buffer);
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
#include <cstdarg>
void pl_debug_print(const char* format, ...);
void pl_format_print(char* buffer, uint32 buffer_size,const char* format, ...);
//--------------------------------------</DEBUG>---------------------------------------
