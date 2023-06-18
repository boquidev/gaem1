
#define NODE(x) x##_Node
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
		UNTIL(i, index){                                                            \
			result = result->next_node;                                                            \
		}                                                                                          \
		return result;                                                                                    \
	}                                                            \
}

/////////////////////////////////////////////////////////////

struct testruct{
	u32 filler;
	testruct* list [3];
};

#define NEW_LIST(type, var_name) type var_name[3]
#define LIST_LAST(l) l[1]
#define LIST_LENGTH(l) l[2]
#define NEXT_ELEM(e) {void** cast = &(void*)e;*cast = *((void**)(e+1));}
#define LIST_GET(l,index, node) node = l[0];UNTIL(i,index){NEXT_ELEM(node);}

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
	out = l[1];\
}


testruct test_function(testruct* l [3], Memory_arena* arena)
{
	testruct* testing;
	PUSH_BACK(l, arena, testing);

	testruct* iterator;
	LIST_GET(l, 2, iterator);
	LIST_GET(l,1, iterator);
	
	return {0};
}
