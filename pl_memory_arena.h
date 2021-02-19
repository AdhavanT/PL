#pragma once

#ifndef PL_MEMORY_ARENA_UTILS
//gets #define MONITOR_ARENA_USAGE and pl_arena_buffer functions from pl_utils. Both can be easily replaced by defining MONITOR_ARENA_USAGE and using VirtualAlloc (or malloc)
#include "pl_utils.h"
#endif



#ifdef MONITOR_ARENA_USAGE
struct ArenaOwnerNode
{
	char* type_name;
	size_t size;
};

struct ArenaOwnerStack
{
	ArenaOwnerNode* front = 0;
	int32 length = 0;
	FORCEDINLINE void push_node(ArenaOwnerNode& new_node)
	{
		if (front[length - 1].type_name == new_node.type_name)
		{
			front[length - 1].size += new_node.size;
		}
		else
		{
			front[length] = new_node;
			length++;
		}
		if (length > ARENAOWNERLIST_CAPACITY)
		{
			ERRORBOX("Arena Lock Stack overflowed! Too many arena pushes...")
		}
	}

	FORCEDINLINE void pop_node(ArenaOwnerNode* new_node)
	{
#ifdef ARENA_MONITOR_CHECK_FOR_POPS
		if (front[length - 1].type_name != new_node->type_name || front[length - 1].size != new_node->size)
		{
			ERRORBOX("Incorrect Arena pop occured! Trying to pop Node that isn't at top of stack.");
		}
#endif
		length--;
		if (length < 0)
		{
			ERRORBOX("Arena Lock Stack underflowed! More arena pops than pushes...")
		}
	}
};
#endif


#ifndef PL_MEMORY_ARENA_DEFINED
struct MArena
{
#ifdef MONITOR_ARENA_USAGE
	ArenaOwnerStack allocations;
#endif
	void* base = 0;
	size_t capacity = 0;
	size_t overflow_addon_size = 0;
	size_t top = 0;
};
#define PL_MEMORY_ARENA_DEFINED
#endif


#ifndef PL_MEMORY_ARENA_UTILS
static inline void init_memory_arena(MArena* arena, size_t capacity, void* base)
{
#ifdef MONITOR_ARENA_USAGE
	arena->allocations.front = (ArenaOwnerNode*)pl_buffer_alloc(sizeof(ArenaOwnerNode) * ARENAOWNERLIST_CAPACITY);
#endif
	arena->base = base;
	arena->capacity = capacity;
	arena->top = 0;
}

static inline void init_memory_arena(MArena* arena, size_t capacity)
{
#ifdef MONITOR_ARENA_USAGE
	arena->allocations.front = (ArenaOwnerNode*)pl_buffer_alloc(sizeof(ArenaOwnerNode) * ARENAOWNERLIST_CAPACITY);
#endif
	arena->base = pl_arena_buffer_alloc(capacity);
	arena->capacity = capacity;
	arena->top = 0;
}

static inline void cleanup_memory_arena(MArena* arena)
{
#ifdef MONITOR_ARENA_USAGE
	pl_buffer_free(arena->allocations.front);
#endif
	ASSERT(arena->base != 0);
	pl_arena_buffer_free(arena->base);
	arena->base = 0;
	arena->top = 0;
}

//-------------------------<MARENA_PUSH>--------------------------------------------------------------
FORCEDINLINE void* marena_push(MArena* arena, size_t room_to_make)
{
	if (arena->top + room_to_make > arena->capacity)
	{
#ifdef PL_DO_MEMORY_ARENA_ADDON
		//To grow the arena:
		if (arena->overflow_addon_size == 0)
		{
			ERRORBOX("MArena has overflowed! Can't do addon!")
		}
		size_t extra = arena->top + room_to_make - arena->capacity;
		size_t multiplier = (extra + (arena->overflow_addon_size - 1)) / arena->overflow_addon_size;
		size_t new_capacity = arena->capacity + multiplier * arena->overflow_addon_size;
		arena->base = pl_arena_buffer_resize(arena->base, arena->capacity, new_capacity);
		arena->capacity = new_capacity;
#else
		ERRORBOX("MArena has overflowed! Addon is turned off!")
#endif
	}
	void* top = (uint8*)arena->base + arena->top;
	arena->top += room_to_make;
	return top;
}

#ifdef MONITOR_ARENA_USAGE
FORCEDINLINE void* monitored_push_arena(MArena* arena, size_t room_to_make, char* name)
{

	ArenaOwnerNode new_node = { name, room_to_make };
	arena->allocations.push_node(new_node);
	void* ret = marena_push(arena, room_to_make);
	return ret;
}
#endif
#ifdef MONITOR_ARENA_USAGE
#define MARENA_PUSH(arena, size, name) monitored_push_arena(arena, size, (char*)name)
#else
#define MARENA_PUSH(arena, size, name) marena_push(arena, size)
#endif
//-------------------------</MARENA_PUSH>-------------------------------------------------------------

//-------------------------<MARENA_POP>--------------------------------------------------------------
FORCEDINLINE void marena_pop(MArena* arena, size_t amount_to_pop)
{
	if (amount_to_pop > arena->top)
	{
		ERRORBOX("MArena has underflowed!")
	}
	arena->top -= amount_to_pop;
}

#ifdef MONITOR_ARENA_USAGE
FORCEDINLINE void monitored_pop_arena(MArena* arena, size_t amount_to_pop, char* name)
{

	ArenaOwnerNode pop_node = { name, amount_to_pop };
	arena->allocations.pop_node(&pop_node);
	marena_pop(arena, amount_to_pop);
}
#endif

#ifdef MONITOR_ARENA_USAGE
//Pops the top of the arena stack and pops arena owner node.
#define MARENA_POP(arena, size, name) monitored_pop_arena(arena, size, (char*)name)
#else
//Pops the top of the arena stack. 
#define MARENA_POP(arena, size, name) marena_pop(arena, size)
#endif
//-------------------------</MARENA_POP>-------------------------------------------------------------

//-------------------------<MARENA_TOP>-------------------------------------------------------------
FORCEDINLINE void* marena_top(MArena* arena) { return (uint8*)arena->base + arena->top; }
#define MARENA_TOP(arena) marena_top(arena)
//-------------------------</MARENA_TOP>------------------------------------------------------------

//Memory Slice. A buffer that acts on a memory arena. 
template <typename t, typename size_type = size_t>
struct MSlice
{
#ifdef MONITOR_ARENA_USAGE
	char* name = 0;
#endif

	t* front = 0;
	size_type size = 0;

#ifdef MONITOR_ARENA_USAGE
	FORCEDINLINE void init(MArena* arena,const char* _name)
	{
		name = (char*)_name;
		ASSERT(front == 0);
		front = (t*)MARENA_TOP(arena);
	}
#else
	FORCEDINLINE void init(MArena* arena,const char* _name)
	{
		ASSERT(front == 0);
		front = (t*)MARENA_TOP(arena);
	}
#endif

#ifdef MONITOR_ARENA_USAGE
	FORCEDINLINE void init_and_allocate(MArena* arena, size_type no_of_elements, const char* _name)
	{
		size = no_of_elements;
		name = (char*)_name;
		ASSERT(front == 0);
		front = (t*)MARENA_PUSH(arena, size * sizeof(t), name);
	}
#else
	FORCEDINLINE void init_and_allocate(MArena* arena, size_type no_of_elements, const char* _name)
	{
		size = no_of_elements;
		ASSERT(front == 0);
		front = (t*)MARENA_PUSH(arena, size * sizeof(t), name);
	}
#endif

	FORCEDINLINE t* add(MArena* arena, t new_member)
	{
		ASSERT(front != 0);		//Buffer was not initilized into arena. 

		size++;
		t* temp = (t*)MARENA_PUSH(arena, sizeof(t), name);
		*temp = new_member;
		return temp;
	}

	FORCEDINLINE t* add_nocpy(MArena* arena, t& new_member)
	{
		ASSERT(front != 0);		//Buffer was not initilized into arena. 

		size++;
		t* temp = (t*)MARENA_PUSH(arena, sizeof(t), name);
		*temp = new_member;
		return temp;
	}


	FORCEDINLINE void clear(MArena* arena)
	{
		if (size > 0 && front != 0)
		{
			MARENA_POP(arena, size * sizeof(t), name);
			front = 0;
			size = 0;
		}
	}

	FORCEDINLINE t& operator [](size_type index)
	{
		ASSERT(index >= 0 && index < (size_type)size);
		return (front[index]);
	}
};

#endif