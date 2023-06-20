#include "definitions.h"
#include "boqui_math.h"
// string.h IS INCLUDED IN THIS FILE AFTER MEMORY ARENAS
// maybe with GUARDS i could include it just here
// #include "math"
// #include "color"

// Byte/Memory operations
internal void 
set_mem(void* mem, u32 size, u8 value)
{
	u8* scan = (u8*)mem;
	UNTIL(i, size)
	{
		*scan = value;
		scan++;
	}
}

internal void
copy_mem(void* from, void* to, u32 size)
{
	UNTIL(i, size)
	{
		((u8*)to)[i] = ((u8*)from)[i];
	}
}

internal bool
compare_mem(void* p1, void* p2, u32 size)
{
	UNTIL(i, size)
	{
		if(*(u8*)p1 != *(u8*)p2)
			return false;
	}
	return true;
}

// MEMORY ARENAS YEAH
struct Memory_arena{
	u8* data;
	u32 used;
	u32 size;
};

internal u8*
arena_push_size(Memory_arena* arena, u32 size){
	u8* result = arena->data+arena->used;
	arena->used += size;
	return result;
}

internal void
arena_pop_back_size(Memory_arena* arena, u32 size){
	arena->used -= size;
	set_mem(arena->data+arena->used, size, 0);
}

// TODO: THIS IS ILLEGAL and very slow
// internal void
// arena_pop_size(Memory_arena* arena, void* data, u32 size){
// 	u8* first_unused_byte = arena->data+arena->used;
// 	u8* skipped_data = ((u8*)data)+size;
// 	ASSERT(data>=arena->data && skipped_data<=first_unused_byte);

// 	copy_mem(skipped_data,data, first_unused_byte-skipped_data);
// 	arena_pop_back_size(arena, size);
// }

internal u8*
arena_push_data(Memory_arena* arena, void* data, u32 size){
	ASSERT((arena->used + size) <= arena->size);
	u8* result = arena_push_size(arena, size);
	copy_mem(data, result, size);
	return result;
}

#define ARENA_PUSH_STRUCT(arena, type) (type*)arena_push_size(arena, sizeof(type))
#define ARENA_PUSH_STRUCTS(arena, type, count) (type*)arena_push_size(arena, count*sizeof(type))

#include "string.h"

struct File_data
{
	void* data;
	u32 size;
};


// Image / Texture / Screen / RectPixels
struct Surface
{
	u32 width;
	u32 height;
	void* data;
};
internal u32
find_bigger_exponent_of_2(u32 target_value){
	u32 result = 2;
	while(target_value>(result*result)){
		result = result*2;
	}
	return result;
}

// LINKED LISTS WITH MACRO FUNCTIONS

#define LIST(type, var_name) type* var_name[3]
#define LIST_LAST(l) l[1]
#define LIST_SIZE(l) ((u32)(l[2]))
#define NEXT_ELEM(e) {void** cast = &(void*)e;*cast = *((void**)(e+1));}
#define LIST_GET(l,index, node) node = l[0];UNTIL(unique_index##__LINE__,(index)){NEXT_ELEM(node);}
#define FOR_EACH(l, iterator) for(u32 iterator = 0; iterator < LIST_SIZE(l); iterator++)

#define PUSH_BACK(l, arena, out){\
	if(!l[0]){\
		void** cast = &(void*)(l[0]);\
		*cast = arena_push_size(arena, sizeof(*l[0]) + sizeof(l[0]));\
		l[1] = l[0];\
	}else{\
		void** p_last_element = (void**)(l[1]+1);\
		void** cast = &(void*)(l[1]);\
		*cast = arena_push_size(arena, sizeof(*l[0]) + sizeof(l[0]));\
		*p_last_element = l[1];\
	}\
	u32* size = (u32*)l+2;\
	*size += 1;\
	out = l[1];\
}


// TEMPORARILY ORPHAN 

struct Font_character_info{
	s32 w, h;
	s32 xoffset, yoffset;
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

struct Vertex3d{
	V3 pos;
	V2 texcoord;
	V3 normal;
};
struct Vertex_2d{
	V3 pos;
	V2 texcoord;
};


struct V4_u8
{
    u8 x;
    u8 y;
    u8 z;
    u8 w;
};

struct Color_u16
{
    u16 r;
    u16 g;
    u16 b;
    u16 a;
};

struct Mesh_primitive
{
    void* vertices;
	 u32 vertex_size;
    // V3* vertex_positions;
    // V2* vertex_texcoords;
    // V3* vertex_normals;

    u32 vertex_count;
    u16* indices;
    u32 indices_count;
};


// OLD LINKED LISTS

// struct List_node
// {
// 	List_node* next_node;
// 	u32 size;
// 	void* data;
// };
// struct List
// {
// 	List_node* root;
// 	List_node* last;
// 	u32 size;
// };

// internal List_node*
// list_get(List* list, u32 index)
// {
// 	ASSERT(index<list->size);
// 	List_node* result = list->root;
// 	for(u32 i=0; i<index; i++)
// 	{
// 		result = result->next_node;
// 	}
// 	return result;
// }
// internal void*
// list_get_data(List* list, u32 index)
// {
// 	return list_get(list, index)->data;
// }
// #define LIST_GET_DATA_AS(list, i, type) (type*)list_get_data(list, i)

// internal List_node*
// list_push_back(List* list, Memory_arena* arena)
// {
// 	List_node* result = {0};
// 	if(!list->size)
// 	{
// 		list->root = ARENA_PUSH_STRUCT(arena,List_node); 
// 		result = list->root;
// 		list->last = list->root;
// 	}else{
// 		List_node* current_last = list->last;
// 		current_last->next_node = ARENA_PUSH_STRUCT(arena, List_node);
// 		result = current_last->next_node;
// 		list->last = result;
// 	}
// 	list->size++;
// 	return result;
// }
// internal void*
// list_push_back_size(List* list, u32 size, Memory_arena* arena)
// {
// 	List_node* new_node = list_push_back(list, arena);
// 	new_node->data = arena_push_size(arena, size);
// 	new_node->size = size;
// 	return new_node->data;
// }
// #define LIST_PUSH_BACK_STRUCT(list, type, arena) (type*)list_push_back_size(list, sizeof(type), arena);

// internal void*
// list_push_back_data(List* list,void* data, u32 size, Memory_arena* arena)
// {
// 	void* node_data = list_push_back_size(list, size, arena);
// 	copy_mem(data, node_data, size);
// 	return node_data;
// }

// internal List_node*
// list_push_front(List* list, Memory_arena* arena)
// {
// 	List_node* result = {0};
// 	if(!list->size)
// 	{
// 		list->root = ARENA_PUSH_STRUCT(arena,List_node); 
// 		result = list->root;
// 		list->last = list->root;
// 	}else{
// 		result = ARENA_PUSH_STRUCT(arena, List_node);
// 		result->next_node = list->root;
// 		list->root = result;
// 	}
// 	list->size++;
// 	return result;
// }
// internal void*
// list_push_front_size(List* list, u32 size, Memory_arena* arena)
// {
// 	List_node* new_node = list_push_front(list, arena);
// 	new_node->data = arena_push_size(arena, size);
// 	new_node->size = size;
// 	return new_node->data;
// }
// #define LIST_PUSH_FRONT_STRUCT(list, type, arena) (type*)list_push_front_size(list, sizeof(type), arena);

// internal void*
// list_push_front_data(List* list,void* data, u32 size, Memory_arena* arena)
// {
// 	void* node_data = list_push_front_size(list, size, arena);
// 	copy_mem(data, node_data, size);
// 	return node_data;
// }