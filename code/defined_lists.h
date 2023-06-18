
#define LIST(x) x##_List 
#define NODE(x) LIST(x)::x##_Node

// #define DEFINE_LIST(type) \
// struct NODE(type){NODE(type)* next_node; type* value; }; \
// struct LIST(type){                              \
// 	NODE(type)* root;                               \
// 	NODE(type)* last;                              \
// 	u32 size;                                                              \
// 	type* push_back(Memory_arena* arena)                              \
// 	{                              \
// 		type* result = ARENA_PUSH_STRUCT(arena, type);                              \
// 		if(!size)                              \
// 		{                              \
// 			root = ARENA_PUSH_STRUCT(arena, NODE(type));                              \
// 			last = root;                              \
// 		}else{                              \
// 			last->next_node = ARENA_PUSH_STRUCT(arena, NODE(type));                              \
// 			last = last->next_node;                              \
// 		}                              \
// 		last->value = result;                              \
// 		size++;                              \
// 		return result;                              \
// 	}                              \
// 	type* push_front(Memory_arena* arena)                              \
// 	{                              \
// 		type* result = ARENA_PUSH_STRUCT(arena, type);                              \
// 		if(!size)                              \
// 		{                              \
// 			root = ARENA_PUSH_STRUCT(arena, NODE(type));                              \
// 			last = root;                              \
// 		}else{                              \
// 			NODE(type)* old_root = root;                              \
// 			root = ARENA_PUSH_STRUCT(arena, NODE(type));                              \
// 			root->next_node = old_root;                              \
// 		}                              \
// 		root->value = result;                              \
// 		size++;\
// 		return result;                              \
// 	}                              \
//                               \
// 	NODE(type)* get(u32 index)                                                            \
// 	{                                                            \
// 		ASSERT(index<size);                                                            \
// 		NODE(type)* result = root;                                                             \
// 		UNTIL(i, index){                                                            \
// 			result = result->next_node;                                                            \
// 		}                                                                                          \
// 		return result;                                                                                    \
// 	}                                                            \
// }

// #define NODE_STRUCT(type, name) struct name{ \
// 	type value; \
// 	void* next_node;}

// #define TEST_DEFINE_LIST(type) struct LIST(type){ \
// 	NODE_STRUCT(type)* root;\
// 	NODE_STRUCT(type)* last;\
// 	u32 size;\
// \
// 	NODE_STRUCT(type)* push_back(Memory_arena* arena)                              \
// 	{                              \
// 		NODE_STRUCT(type) node_size;\
// 		NODE_STRUCT(type)* result = arena_push_size(arena,sizeof(node_size));       \
// 	}           \
// }
// #define LIST_PUSH_BACK(list, arena) 


#define DEFINE_NEW_LIST(type) struct LIST(type){\
	struct NODE(type){\
		type* value;\
		NODE(type)* next_node;\
	};\
\
	NODE(type)* root;\
	NODE(type)* last;\
	u32 size;\
\
	type* push_back(Memory_arena* arena){\
		type* result = ARENA_PUSH_STRUCT(arena, type);\
		if(!size){\
			root = ARENA_PUSH_STRUCT(arena, NODE(type));\
			last = root;\
		}else{\
			last->next_node = ARENA_PUSH_STRUCT(arena, NODE(type));\
			last = last->next_node;\
		}\
		last->value = result;\
		size++;\
		return result;\
	}\
\
	type* push_front(Memory_arena* arena){\
		type* result = ARENA_PUSH_STRUCT(arena, type);\
		if(!size){\
			root = ARENA_PUSH_STRUCT(arena, NODE(type));\
			last = root;\
		}else{\
			NODE(type)* old_root = root;\
			root = ARENA_PUSH_STRUCT(arena, NODE(type));\
			root->next_node = old_root;\
		}\
		root->value = result;\
		size++;\
		return result;\
	}\
\
	NODE(type)* get(u32 index){\
		ASSERT(index<size);\
		NODE(type)* result = root;\
		UNTIL(i, index){\
			result = result->next_node;\
		}\
		return result;\
	}\
}

internal void
test_function(Memory_arena* arena){
 	{
		DEFINE_NEW_LIST(File_data) test_file_list;
		
		struct Foo{
			struct Foo_List{
				struct Foo_List_node{
					Foo* value;
					Foo_List_node* next_node;
				};
				Foo_List_node* root;
				Foo_List_node* last;
				u32 size;
			}* list;
			u32 aaaaaa;
		} test_foo;

		NODE(File_data)* new_node = (NODE(File_data)*)test_file_list.push_back(arena);
	}
	struct File_data_list{};
}


		// if(!size)                              \
		// {                              \
		// 	last = result;                              \
		// 	root = result;                              \
		// }else{                              \
		// 	last->next_node = result;               \
		// 	NEXTNODE(last);                              \
		// }                                                  \
		// size++;                              \
		// return result;                              \