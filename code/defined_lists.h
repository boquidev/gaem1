
#define NODE(x) x##_List_node
#define LIST(x) x##_List 
#define DEFINE_LIST(type) \
struct NODE(type){NODE(type)* next_node; type* value; }; \
struct LIST(type){                              \
	NODE(type)* root;                               \
	NODE(type)* last;                              \
	u32 size;                                                              \
	type* push_back(Memory_arena* arena)                              \
	{                              \
		type* result = ARENA_PUSH_STRUCT(arena, type);                              \
		if(!size)                              \
		{                              \
			root = ARENA_PUSH_STRUCT(arena, NODE(type));                              \
			last = root;                              \
		}else{                              \
			last->next_node = ARENA_PUSH_STRUCT(arena, NODE(type));                              \
			last = last->next_node;                              \
		}                              \
		last->value = result;                              \
		size++;                              \
		return result;                              \
	}                              \
	type* push_front(Memory_arena* arena)                              \
	{                              \
		type* result = ARENA_PUSH_STRUCT(arena, type);                              \
		if(!size)                              \
		{                              \
			root = ARENA_PUSH_STRUCT(arena, NODE(type));                              \
			last = root;                              \
		}else{                              \
			NODE(type)* old_root = root;                              \
			root = ARENA_PUSH_STRUCT(arena, NODE(type));                              \
			root->next_node = old_root;                              \
		}                              \
		root->value = result;                              \
		return result;                              \
	}                              \
                              \
	NODE(type)* get(u32 index)                                                            \
	{                                                            \
		ASSERT(index<size);                                                            \
		NODE(type)* result = root;                                                             \
		until(i, index){                                                            \
			result = result->next_node;                                                            \
		}                                                                                          \
		return result;                                                                                    \
	}                                                            \
}


DEFINE_LIST(V3);