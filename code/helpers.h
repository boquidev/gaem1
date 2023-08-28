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
typedef Memory_arena Data_stream;

internal u8*
arena_push_size(Memory_arena* arena, u32 size){
	ASSERT(size+arena->used < arena->size);
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

struct Buffer{
	void* data;
	u32 size;
};
typedef Buffer File_data;


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


// FIXED SIZE ARRAYS


#define ARRAY(type, name, size, arena) \
	*(ARENA_PUSH_STRUCT(arena, u32)) = size;\
	type* name = ARENA_PUSH_STRUCTS(arena, type, size);

#define ARRAYLEN(array) *(((u32*)array)-1)




// TYPESAFE LINKED LISTS WITH MACRO FUNCTIONS


#define LIST(type, var_name) type* var_name[3]
#define CLEAR_LIST(l) l[0] = 0; l[1] = 0; l[2] = 0;
#define LIST_LAST(l) l[1]
#define LIST_SIZE(l) ((u32)(l[2]))
#define NEXT_ELEM(node) *((void**)(node+1))
#define SKIP_ELEM(node) *(&(void*)node) = *((void**)(node+1))
#define LIST_GET(l,index, node) node = l[0];ASSERT(index<LIST_SIZE(l));UNTIL(unique_index##__LINE__,(index)){SKIP_ELEM(node);}
#define FOREACH(type, node, list) \
	for( type* node = list[0],*i##__LINE__=0; \
	(*((u32*)&i##__LINE__))<LIST_SIZE(list); \
	(*((u32*)&i##__LINE__))++, SKIP_ELEM(node))

#define LIST_POP_FRONT(l) l[0]; *(&(void*)l[0]) = *(void**)(l[0]+1); *((u32*)&(l[2])) -= 1

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
//TODO: push front 


// TEMPORARILY ORPHAN 

//CUSTOM BITFIELD ENUMS
#define FLAGS_ARRAY32(name) u32 name [32] = {\
		1,name[0]*2,name[1]*2,name[2]*2,name[3]*2,name[4]*2,name[5]*2,\
		name[6]*2,name[7]*2,name[8]*2,name[9]*2,name[10]*2,\
		name[11]*2,name[12]*2,name[13]*2,name[14]*2,name[15]*2,\
		name[16]*2,name[17]*2,name[18]*2,name[19]*2,name[20]*2,\
		name[21]*2,name[22]*2,name[23]*2,name[24]*2,name[25]*2,\
		name[26]*2,name[27]*2,name[28]*2,name[29]*2,name[30]*2\
	};
	
#define FLAGS_ARRAY64(name) u64 name [64] = {\
		1,name[0]*2,name[1]*2,name[2]*2,name[3]*2,name[4]*2,name[5]*2,\
		name[6]*2,name[7]*2,name[8]*2,name[9]*2,name[10]*2,\
		name[11]*2,name[12]*2,name[13]*2,name[14]*2,name[15]*2,\
		name[16]*2,name[17]*2,name[18]*2,name[19]*2,name[20]*2,\
		name[21]*2,name[22]*2,name[23]*2,name[24]*2,name[25]*2,\
		name[26]*2,name[27]*2,name[28]*2,name[29]*2,name[30]*2,\
		name[31]*2,name[32]*2,name[33]*2,name[34]*2,name[35]*2,\
		name[36]*2,name[37]*2,name[38]*2,name[39]*2,name[40]*2,\
		name[41]*2,name[42]*2,name[43]*2,name[44]*2,name[45]*2,\
		name[46]*2,name[47]*2,name[48]*2,name[49]*2,name[50]*2,\
		name[51]*2,name[52]*2,name[53]*2,name[54]*2,name[55]*2,\
		name[56]*2,name[57]*2,name[58]*2,name[59]*2,name[60]*2,\
		name[61]*2,name[62]*2\
	}


// TODO: find a way to do bigger than 64 bitfields
static union {
	FLAGS_ARRAY64(GLOBAL_BITFIELDS_64);
	struct { // ENTITY FLAGS
		u64 
		E_VISIBLE, 
		E_SELECTABLE, 
		E_SKIP_UPDATING,

		E_HAS_COLLIDER,
		E_DETECT_COLLISIONS,
		E_RECEIVES_DAMAGE,
		E_DOES_DAMAGE,
		// E_HEALTH_IS_DAMAGE,
		E_DIE_ON_COLLISION,

		E_UNCLAMP_Y,
		E_UNCLAMP_XZ,
		E_CAN_MANUALLY_MOVE,
		E_NOT_MOVE,
		
		E_AUTO_AIM_BOSS,
		E_AUTO_AIM_CLOSEST,
		E_CANNOT_AIM,
		E_NOT_TARGETABLE,
		E_FOLLOW_TARGET,

		// E_LOOK_TARGET_WHILE_MOVING,
		E_LOOK_IN_THE_MOVING_DIRECTION,

		E_SHRINK_WITH_VELOCITY,
		E_SHRINK_WITH_LIFETIME,

		E_SKIP_DYNAMICS,
		E_SKIP_ROTATION,

		E_STICK_TO_ENTITY,
		
		E_SKIP_PARENT_COLLISION,
		E_IGNORE_ALLIES,

		// ACTIONS

		E_SHOOT,
		E_MELEE_ATTACK,
			E_MELEE_HITBOX,
		E_SPAWN_ENTITIES,
		E_HEALER,
		E_GENERATE_RESOURCE,

		E_HAS_SHIELD,
			P_SHIELD,

		// ACTION EXTRA PROPERTIES

		E_FREEZING_ACTIONS,
		E_HIT_SPAWN_GRAVITY_FIELD,
			P_FREEZING,
			P_PROJECTILE_EXPLODE, 
			P_SPAWN_GRAVITY_FIELD,
		E_IS_AFFECTED_BY_GRAVITY,

		// SHOOTER FLAGS

		E_PROJECTILE_JUMPS_BETWEEN_TARGETS,
			P_JUMP_BETWEEN_TARGETS,
		E_PROJECTILE_PIERCE_TARGETS,
		E_HOMING_PROJECTILES,
		
		E_PROJECTILE_EXPLODE,
			E_EXPLOSION,

		E_SMOKE_SCREEN,


		// MELEE FLAGS
		E_LIFE_STEAL,
			A_STEAL_LIFE,
		
		E_TOXIC_EMITTER,
		E_TOXIC_DAMAGE_INMUNE,
		

		E_LAST_FLAG;
	};

	struct { // UI FLAGS
		u64
		UI_ACTIVE,

		UI_LAST_FLAG;
	};
};

// THIS APPLIES TO ANY TEXTURE NOT JUST FONTS
struct Tex_info{
	u32 texview_uid;
	s32 w, h;
	s32 xoffset, yoffset;
	// this are normalized coordinates 0.0->1.0 with 1.0 being the atlas width/height;
	Rect texrect; 
	// the offsets are in pixels;
};

union Color
{
	struct{
		f32 r;
		f32 g;
		f32 b;
		f32 a;
	};
	// struct RGB_accessor{
	// 	f32 r;
	// 	f32 g;
	// 	f32 b;
	// } rgb;
};
internal Color
operator *(f32 scalar, Color color){
	return {scalar*color.r, scalar*color.g, scalar*color.b, scalar*color.a};
}

internal Color
colors_product(Color c1, Color c2){
	return {c1.r*c2.r, c1.g*c2.g, c1.b*c2.b, c1.a*c2.a};
}

internal Color
operator *(Color c1, Color c2){
	return colors_product(c1, c2);
}


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

struct Sound_sample{
	s16* samples;
	u32 samples_count;
	u32 channels;
};

struct Audio_playback{
	u32 sound_uid;
	u32 initial_sample_t; 
	// with a u32 i could have 12.6 hours until it overflows (maybe 25)
	b32 loop;
};



internal Audio_playback*
find_next_available_playback(Audio_playback* list){
	Audio_playback* result = 0;
	UNTIL(i, ARRAYLEN(list)){
		if(!list[i].initial_sample_t){
			result = &list[i];
			break;
		}
	}
	ASSERT(result);
	return result;
}


