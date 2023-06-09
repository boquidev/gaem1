#include "definitions.h"
#include "string.h"
// #include "math"
// #include "color"

struct File_data
{
	void* data;
	u32 size;
};

// Byte/Memory operations
internal void 
set_mem(void* mem, u32 size, u8 value)
{
	u8* scan = (u8*)mem;
	until(i, size)
	{
		*scan = value;
		scan++;
	}
}

internal void
copy_mem(void* from, void* to, u32 size)
{
	until(i, size)
	{
		((u8*)to)[i] = ((u8*)from)[i];
	}
}

internal bool
compare_mem(void* p1, void* p2, u32 size)
{
	until(i, size)
	{
		if(*(u8*)p1 != *(u8*)p2)
			return false;
	}
	return true;
}

// Image / Texture / Screen / RectPixels
struct Surface
{
	u32 width;
	u32 height;
	void* data;
};

// MEMORY ARENAS YEAH
struct Memory_arena
{
	u8* data;
	u32 used;
	u32 size;
};

internal u8*
arena_push_size(Memory_arena* arena, u32 size)
{
	u8* result = arena->data+arena->used;
	arena->used += size;
	return result;
}

internal void
arena_pop_size(Memory_arena* arena, u32 size)
{
	arena->used -= size;
	set_mem(arena->data+arena->used, size, 0);
}

internal u8*
arena_push_data(Memory_arena* arena, void* data, u32 size)
{
	ASSERT((arena->used + size) <= arena->size);
	u8* result = arena_push_size(arena, size);
	copy_mem(data, result, size);
	return result;
}

#define ARENA_PUSH_STRUCT(arena, type) (type*)arena_push_size(arena, sizeof(type))
#define ARENA_PUSH_STRUCTS(arena, type, count) (type*)arena_push_size(arena, count*sizeof(type))

// TODO: test if this does need the memory arena
// move it to string.h if not
internal String 
number_to_string(s32 n, Memory_arena* arena)
{
	u32 i=0;
	String result = {0};
	result.text = (char*)arena_push_size(arena, 0);
	if(n < 0)
	{ 
		arena_push_data(arena, "-", 1);
		n = -(n);
		i++;
	}
	if(!n) // if number is 0
	{
		*(char*)arena_push_size(arena, 1) = 48;
		arena_push_size(arena, 1); // 0 ending string
		return result;
	}
	u8 digits = 0;
	s32 temp = n;
	while(temp)
	{
		temp = temp/10;
		i++;
		digits++;
	}
	arena_push_size(arena, digits);
	for(;digits; digits--)
	{
		result.text[i-1] = 48 + (n%10);
		n = n/10;
		i--;
	}
	arena_push_data(arena, "\0", 1);
	return result;
}

// MY LINKED LIST IMPLEMENTATION

struct List_node
{
	List_node* next_node;
	u32 size;
	void* data;
};
struct List
{
	List_node* root;
	List_node* last;
	u32 size;
};

internal List_node*
list_get(List* list, u32 index)
{
	ASSERT(index<list->size);
	List_node* result = list->root;
	for(u32 i=0; i<index; i++)
	{
		result = result->next_node;
	}
	return result;
}
internal void*
list_get_data(List* list, u32 index)
{
	return list_get(list, index)->data;
}
#define LIST_GET_DATA_AS(list, i, type) (type*)list_get_data(list, i)

internal List_node*
list_push_back(List* list, Memory_arena* arena)
{
	List_node* result = {0};
	if(!list->size)
	{
		list->root = ARENA_PUSH_STRUCT(arena,List_node); 
		result = list->root;
		list->last = list->root;
	}else{
		List_node* current_last = list->last;
		current_last->next_node = ARENA_PUSH_STRUCT(arena, List_node);
		result = current_last->next_node;
		list->last = result;
	}
	list->size++;
	return result;
}
internal void*
list_push_back_size(List* list, u32 size, Memory_arena* arena)
{
	List_node* new_node = list_push_back(list, arena);
	new_node->data = arena_push_size(arena, size);
	new_node->size = size;
	return new_node->data;
}
#define list_push_back_struct(list, type, arena) (type*)list_push_back_size(list, sizeof(type), arena);

internal void*
list_push_back_data(List* list,void* data, u32 size, Memory_arena* arena)
{
	void* node_data = list_push_back_size(list, size, arena);
	copy_mem(data, node_data, size);
	return node_data;
}

internal List_node*
list_push_front(List* list, Memory_arena* arena)
{
	List_node* result = {0};
	if(!list->size)
	{
		list->root = ARENA_PUSH_STRUCT(arena,List_node); 
		result = list->root;
		list->last = list->root;
	}else{
		result = ARENA_PUSH_STRUCT(arena, List_node);
		result->next_node = list->root;
		list->root = result;
	}
	list->size++;
	return result;
}
internal void*
list_push_front_size(List* list, u32 size, Memory_arena* arena)
{
	List_node* new_node = list_push_front(list, arena);
	new_node->data = arena_push_size(arena, size);
	new_node->size = size;
	return new_node->data;
}
#define list_push_front_struct(list, type, arena) (type*)list_push_front_size(list, sizeof(type), arena);

internal void*
list_push_front_data(List* list,void* data, u32 size, Memory_arena* arena)
{
	void* node_data = list_push_front_size(list, size, arena);
	copy_mem(data, node_data, size);
	return node_data;
}


// TEMPORARILY ORPHAN 

struct Int2
{
	s32 x;
	s32 y;
};

struct V2
{
	r32 x;
	r32 y;
};

struct V3
{
	r32 x;
	r32 y;
	r32 z;
};

struct V4
{
	r32 x;
	r32 y;
	r32 z;
	r32 w;
};

struct Color
{
	r32 r;
	r32 g;
	r32 b;
	r32 a;
};

struct Color32
{
	u8 r;
	u8 g;
	u8 b;
	u8 a;
};

struct Vertex3d
{
	V3 pos;
	V2 texcoord;
};