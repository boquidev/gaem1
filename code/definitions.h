#if !defined(TYPE_DEFINITIONS)
#define TYPE_DEFINITIONS

#define internal static
#define local_persist static
#define global_variable static

#define PI32    3.14159265359f
#define TAU32     6.28318530718f
#define FIRST_CHAR 32
#define LAST_CHAR 127
#define CHARS_COUNT (LAST_CHAR-FIRST_CHAR)
#define CAPS_DIFFERENCE

#define HOLDING_BUTTON_TIME 10

typedef char s8;
typedef short s16;
typedef int s32;
typedef long long s64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char b8;
typedef short b16;
typedef int b32;

typedef float r32;
typedef double r64;

#if DEBUGMODE
    #define ASSERT(expression) if(!(expression)) *(int *)0 = 0;
#else
    #define ASSERT(expression) (expression)
#endif
#define ASSERTHR(hr) ASSERT(SUCCEEDED(hr))

#define UNTIL(i, range) for(u32 i=0; i<range;i++)

// node is the name you want to use, list is a List*, i is the iterator name
#define foreach(node_name, list, i) \
    List_node* node_name = (list)->root; \
    for(s32 i=0; i<(s32)(list)->size; i++)
#define NEXTNODE(node) node = node->next_node

#define KILOBYTES(value) ((value) * 1024LL)
#define MEGABYTES(value) (KILOBYTES(value) * 1024LL)
#define GIGABYTES(value) (MEGABYTES(value) * 1024LL)
#define TERABYTES(value) (GIGABYTES(value) * 1024LL)

#define BUFFER_GET_POS(buffer) (buffer->data+buffer->pos)


#define SCAN(p, type) *(type*)p; \
    p+=sizeof(type);

#define ARRAYCOUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define CLAMP(a,x,b)    (((x)<(a))?(a):\
                        ((b)<(x))?(b):(x))

#define c_linkage extern "C"
#define c_linkage_begin extern "C"{
#define c_linkage_end }
#endif