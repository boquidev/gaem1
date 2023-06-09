#ifndef NON_PLATFORM_H
#define NON_PLATFORM_H

#include "string.h"
#include "color.h"
#include "key_codes.h"
#include "exported_functions.h"

internal void
copy_mem_backwards(void* from, void* to, u32 length)
{
    u32 i=0;
    while(length--)
    {
       *((ui8*)to + i) = *(((ui8*)from) + length);
       i++;
    }
}

internal void
copy_mem(void* from, void* to, u32 length)
{
    while(length--)
    {
       *((ui8*)to + length) = *(((ui8*)from) + length);
    }
}

internal void
set_mem(ui8* mem_pointer, u32 bytes_to_set, ui8 filler_value)
{
    while(bytes_to_set--)
    {
        *mem_pointer++ = filler_value;
    }
}

internal b8
compare_mem(ui8* p1, ui8* p2, u32 size)    
{
    for(u32 i=0; i<size; i++)
    {
        if(p1[i] != p2[i])
        {
            return 0;
        }
    }
    return 1;
}

internal b8 
compare_structs(void* struct1, void* struct2)
{
    b8 same_size = sizeof(struct1) == sizeof(struct2);
    ASSERT(same_size);
    return compare_mem((ui8*)struct1, (ui8*)struct2, sizeof(struct1));
}

struct Surface
{
  i32 width;
  i32 height;
  void* data;
};


struct Read_file_result
{
  u32  size;
  void *contents;
};

enum Mouse_flags
{
    MOUSE_HOLDING_LEFT =    1<<0,
    MOUSE_HOLDING_RIGHT =   1<<1,
    MOUSE_LEFT_DOWN =       1<<2,
    MOUSE_RIGHT_DOWN =      1<<3,
    MOUSE_LEFT_UP =         1<<4, 
    MOUSE_RIGHT_UP =        1<<5,
};

enum Keyboard_flags
{
    KEYBOARD_FLAG_LCTRL =   1<<0,
    KEYBOARD_FLAG_RCTRL =   1<<1,
    KEYBOARD_FLAG_LSHIFT =  1<<2,
    KEYBOARD_FLAG_RSHIFT =  1<<3,
    KEYBOARD_FLAG_LALT =    1<<4,
    KEYBOARD_FLAG_RALT =    1<<5,
    // KEYBOARD_FLAG_CAPS_ON = 1<<6,
};

struct Mouse_input
{
    i16 x_pos;
    i16 y_pos;
    
    i16 wheel_rotation;
    ui8 state; //mouse_flag

    ui8 keyboard_modifier; //keyboard_flag
};

struct Keyboard_input
{
    ui8 vk_code;
    ui8 modifier;
    //ui8 up;
    ui8 ascii_code;
    ui8 caps_on;
};

struct User_input
{
    //keyboard, mouse and controller input
    Mouse_input mouse;
    
    Keyboard_input keyboard_input;
    u32 placeholder;

    void push_mouse_input(ui8 flag, b8 b)
    {
        if(b) // mouse click down
        {mouse.state |= flag;}
        else // mouse click up
        {mouse.state &= (flag ^ 0xff);}
    }

    
};

#define PushStruct(arena, type) (type*)(arena->arena_push_size(sizeof(type)))

struct Memory_arena
{
    ui8* data;
    u32 total_size;
    u32 used;

    void pop_size(u32 size)
    {
        ASSERT((used-size) >= 0);
        used -= size;
        set_mem(data+used, size, 0);
    }
    ui8* current_location()
    {
        return data+used;
    }
};

internal void*
arena_push_size(Memory_arena* arena, u32 size)
{
    ASSERT((arena->used + size) <= arena->total_size);
    ui8* result = arena->data + arena->used;
    arena->used += size;
    return result;
}
internal void
pop_size(Memory_arena* arena, u32 size)
{
    ASSERT(size <= arena->used );
    arena->used -= size;
    set_mem(arena->data+arena->used, size, 0);
}

internal void*
push_data(Memory_arena* arena, void* data, u32 size)
{
    ASSERT((arena->used + size) <= arena->total_size);
    ui8* result = arena->data + arena->used;
    copy_mem(data, arena->data + arena->used, size);
    arena->used += size;
    return result;
}

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

struct Character
{
    i32 xoffset;
    i32 yoffset;
    Rect rect;
};

enum Input_history_Flags
{
    INPUT_HISTORY_EMPTY = 0x00,
    INPUT_HISTORY_BACKSPACE = 0xf0,
    INPUT_HISTORY_DELETE,
    INPUT_HISTORY_INSERT,
    INPUT_HISTORY_BATCH_BACKSPACE,
    INPUT_HISTORY_BATCH_DELETE,
    INPUT_HISTORY_BATCH_INSERT,
    INPUT_HISTORY_BEGINNING = 0xff,
};

struct Text_input_header
{
    ui8* data; //this is just placeholder for the byte indicating beginning
    u32 size;

    u32 cursor_init_pos;
    u32 cursor_end_pos;
    
    ui8 record_type; //backspace, delete, character_insertion
    b8 can_undo;
    ui16 padding; // this is true if the input was an insertion and false if it was a deletion 
};

struct Text_buffer
{
    //this is just the necessary things for functionality
    ui8* data;
    i32 cursor_pos;
    i32 total_used;
    i32 current_column;

    ui8* history_data;
    Text_input_header headers[2000];
    ui16 current_header;
    ui16 current_max_header;

    i32 mark_pos;

    ui8* temp_buffer;

    void undo()
    {
        Text_input_header* header = &headers[current_header];
        if(!header->can_undo)
        {
            if(current_header == 0)
            {
                return;
            }
            current_header--;
            header = &headers[current_header];
        }
        cursor_pos = header->cursor_end_pos;
        header->can_undo = false;
        switch(header->record_type)
        {
            case INPUT_HISTORY_BACKSPACE:
                insert_characters_backwards(header->data, header->size, 0);
            break;
            case INPUT_HISTORY_BATCH_BACKSPACE:
            case INPUT_HISTORY_BATCH_DELETE:
            case INPUT_HISTORY_DELETE:
                insert_characters(header->data, header->size, 0);
            break;
            case INPUT_HISTORY_INSERT:
                cursor_pos = header->cursor_init_pos;
                delete_characters(header->size, 0);
            break;
            default:
            break;
        }
    }

    void redo()
    {
        Text_input_header* header = &headers[current_header];
        if(header->can_undo)
        {
            if(current_header == current_max_header)
            {
                return;
            }
            current_header++;
            header = &headers[current_header];
        }
        cursor_pos = header->cursor_init_pos;
        header->can_undo = true;
        switch(header->record_type)
        {
            case INPUT_HISTORY_BACKSPACE:
                cursor_pos = header->cursor_end_pos;
                delete_characters(header->size, 0);
            break;
            case INPUT_HISTORY_BATCH_BACKSPACE:
            case INPUT_HISTORY_BATCH_DELETE:
            case INPUT_HISTORY_DELETE:
                cursor_pos = header->cursor_end_pos;
                delete_characters(header->size, 0);
            break;
            case INPUT_HISTORY_INSERT:
                cursor_pos = header->cursor_init_pos;
                insert_characters(header->data, header->size, 0);
            break;
            default:
            break;
        }
    }

    void history_record_change(ui8* change, u32 size, ui8 record_type)//, b8 is_inserting
    {
        Text_input_header* header = &headers[current_header];
        if(header->record_type != record_type || //different types 
            record_type == INPUT_HISTORY_BATCH_BACKSPACE ||
            record_type == INPUT_HISTORY_BATCH_DELETE ||
            record_type == INPUT_HISTORY_BATCH_INSERT ||
            !header->can_undo //has been undone already
            )
        {
            if(header->can_undo)
            {
                history_commit_input(); // this advances current_header
                header = &headers[current_header];
            }
            //init buffer for a new record
            header->record_type = record_type;
            header->size = 0;
            header->cursor_init_pos = cursor_pos;
            header->cursor_end_pos = cursor_pos;
            header->can_undo = 1;
            current_max_header = current_header;
        }
        copy_mem(change, header->data + header->size, size);
        header->size += size;

        header->cursor_end_pos = cursor_pos; //maybe not move the cursor here 
        // but in the insertion part
    }

    void insert_characters_backwards(ui8* chars_buffer, u32 size, ui8 record_type)
    {
        if(record_type)
        {
            history_record_change(chars_buffer, size, record_type);
        }
        ASSERT(total_used >= cursor_pos);
        copy_mem(data+cursor_pos, temp_buffer, total_used - cursor_pos);
        copy_mem_backwards(chars_buffer, data+cursor_pos, size);
        copy_mem(temp_buffer, data+cursor_pos+size, total_used-cursor_pos);
        cursor_pos += size;
        total_used += size;
        
    }

    void insert_characters(ui8* chars_buffer, u32 size, ui8 record_type)
    {
        if(record_type)
        {
            history_record_change(chars_buffer, size, record_type);
        }// if !record_type, it means i am undoing or something like that
        ASSERT(total_used >= cursor_pos);
        copy_mem(data+cursor_pos, temp_buffer, total_used - cursor_pos);
        copy_mem(chars_buffer, data+cursor_pos, size);
        copy_mem(temp_buffer, data+cursor_pos+size, total_used-cursor_pos);
        
        total_used += size;
        move_cursor_h(size);
    }

    // this was needed because of the CRLF bullshit
    // void custom_delete(u32 size)
    // {
    //     u32 temp_cursor = cursor_pos;
    //     while(*(data+temp_cursor+size))
    //     {
    //         *(data+temp_cursor) = *(data+temp_cursor+size);
    //         temp_cursor++;
    //     }
    //     total_used -= size;
    // }
    
    void delete_characters(u32 size, ui8 record_type)
    {
        if(cursor_pos < total_used)
        {
            u32 temp_cursor = cursor_pos;
            if(record_type)
            {
                history_record_change((data+cursor_pos), size, record_type);
            }
            while(*(data+temp_cursor+size))
            {
                *(data+temp_cursor) = *(data+temp_cursor+size);
                temp_cursor++;
            }
            total_used -= size;
        }
    }

    b8 move_cursor_h(i32 x)
    {
        if(0<x)
        {
            if(cursor_pos >= total_used){return 0;}
            for(i32 i=0; i<x && cursor_pos<total_used; i++)
            {
                cursor_pos++;
            }
        }else{
            if(cursor_pos == 0){return 0;}
            for(i32 i=0; i>x && cursor_pos > 0; i--)
            {
                cursor_pos--;
            }
        }

        u32 line_start = find_line_start(cursor_pos);
        current_column = cursor_pos - line_start;
        return 1;
    }

    void move_cursor_v(i32 y) // can i move more than one line at a time?
    {
        i32 line_start = find_line_start(cursor_pos);
        i32 line_end = find_line_end(cursor_pos);
        if(y < 0)
        {
            while(line_start  && y)
            {
                cursor_pos = line_start - 1;
                line_start = find_line_start(cursor_pos);
                y++;
            }
        }else{
            line_end = find_line_end(cursor_pos);
            while(line_end < total_used && y)
            {
                cursor_pos = line_end + 1;
                line_end = find_line_end(cursor_pos);
                y--;
            }
        }
        line_start = find_line_start(cursor_pos);
        line_end = find_line_end(cursor_pos);

        cursor_pos = ClampTop(line_start + current_column, line_end);        
    }

    u32 find_line(u32 pos)
    {
        u32 lines_count = 0;
        for(u32 i=0; i<pos; i++)
        {
            if(*(data+i) == ASCII_LINE_FEED)
            {
                lines_count++;
            }
        }
        return lines_count;
    }

    u32 find_line_start(u32 pos)
    {
        u32 line_start = pos;
        while(line_start)
        {
            if(*(data+line_start-1) == ASCII_LINE_FEED)
            {
                break;
            }
            line_start--;
        }
        return line_start;
    }

    i32 find_line_end(i32 pos)
    {
        i32 line_end = pos;
        while(line_end < total_used)
        {
            if(*(data+line_end) == ASCII_LINE_FEED)
            {
                break;
            }
            line_end++;
        }
        return line_end;
    }

    void history_commit_input() // for now just the move functions use this in the shortcuts part
    {
        if(headers[current_header].can_undo)
        {
            headers[current_header+1].data = headers[current_header].data +headers[current_header].size;
            current_header++;
        }
    }
};

internal i32
ui32_get_index_in_array(u32 value, u32 a_array[], u32 array_length)
{
    for(u32 i=0; i<array_length; i++)
    {
        if(value == a_array[i])
        {
            return i;
        }
    }
    return -1;
}

struct Array
{
    ui8* data;
    ui16 element_size;
    u32 length;

    void* get(u32 i)
    {
        ASSERT(i<length);
        return ((ui8*)data)+ (i*element_size);
    }

    i32 get_first_index_of(void* value)
    {
        i32 result = -1;
        ui8* value_byte = (ui8*)value;

        for(u32 i=0; i<length; i++)
        {
            ui8* element_byte = data + i*element_size;
            b8 same_value = true;
            for(u32 j=0; j<element_size; j++)
            {
                if(*(value_byte+j) != *(element_byte+j))
                {
                    same_value = false;
                    break;
                }
            }
            if(same_value)
            {
                result = i;
            }
        }
        return result;
    }
};

struct Array2d
{
    void* data;
    ui16 element_size;
    ui8 x_length;
    ui8 y_length;
    
    void* get(u32 x, u32 y)
    {
        ASSERT(x<x_length && y<y_length);
        return ((ui8*)data)+ element_size*(x + (y*x_length));
    }
};

internal char*
concat_strings(
    char* string1, char* string2, Memory_arena* arena
    )
{
    char* result = (char*)arena->current_location();
    while(*string1)
    {
        push_data(arena, string1, 1);
        string1++;
    }
    while(*string2)
    {
        push_data(arena, string2, 1);
        string2++;
    }
    arena_push_size(arena, 1); //0 ending string
    return result;
}

internal char*
number_to_string(i32 n, Memory_arena* arena)
{
    u32 i=0;
    char* result = (char*)arena->current_location();
    if(n < 0)
    { 
        push_data(arena, "-", 1);
        n = -(n);
        i++;
    }
    if(!n) // if number is 0
    {
        *(char*)arena_push_size(arena, 1) = 48;
        arena_push_size(arena, 1); // 0 ending string
        return result;
    }
    ui8 digits = 0;
    i32 temp = n;
    while(temp)
    {
        temp = temp/10;
        i++;
        digits++;
    }
    arena_push_size(arena, digits);
    for(;digits; digits--)
    {
        result[i-1] = 48 + (n%10);
        n = n/10;
        i--;
    }
    push_data(arena, "\0", 1);
    return result;
}

internal u32
wchar_string_length(wchar_t*c)
{
    u32 count;
    while(*c)
    {
        count++;
    }
    return count;
}

internal void
wchar_to_char(wchar_t* c, char* out)
{
    for(u32 i = 0; *(c+i); i++)
    {
        *(out+i) = *(char*)(c+i);
    }
}

internal void
char_to_wchar(char* c, wchar_t* out)
{
    for(u32 i = 0; *(c+i); i++)
    {
        *(out+i) = *(c+i);
    }
}
internal wchar_t*
char_to_wchar(char* c, Memory_arena* arena)
{
    wchar_t* result = (wchar_t*)arena->current_location();
    while(*c)
    {
        *(char*)arena_push_size(arena, sizeof(wchar_t)) = *c;
        c++;
    }
    *(wchar_t*)arena_push_size(arena, sizeof(wchar_t)) = 0;
    return result;
}

internal V2
normalize_screen_pos(V2 screen_pos, V2 screen_size)
{
    V2 result = {0};
    r32 half_pixel_offset = 0.1f;
    result.x =  (2*(screen_pos.x + half_pixel_offset)/screen_size.x) - 1;
    result.y =  (2*(screen_size.y - screen_pos.y - half_pixel_offset)/screen_size.y) - 1;
    return result; 
}
// TODO: FROM HERE IT SHOULD BE ON tedit.h
struct Text_visual_properties
{
    ui16 posx;
    ui16 posy;
    Color font_color;
    ui16 lines_height;
    ui16 chars_width;
    u32 tab_size;

    u32 first_visible_line;
    u32 visible_lines;
    u32 chars_per_line;
    //this too maybe
    r32 cursor_blink_state;
    r32 cursor_blink_length; 
};

struct Text_rect
{
    Rect rect;
    Text_buffer* text;
    Text_visual_properties* visuals;
};


struct Thread_context
{
    int Placeholder;
};

struct Platform_functions
{
    read_file_type(read_file);
    read_from_clipboard_type(read_from_clipboard);
    copy_in_clipboard_type(copy_in_clipboard);
    get_all_files_from_path_type(get_all_files_from_path);
    sdl_free_type(sdl_free);
};

struct Image_object
{
    Surface texture;
    Int2 pos;
    Int2 size;
    r32 angle;
    Color color;
};

struct Char_object
{
    Rect atlas_rect;
    Int2 pos;
    Int2 size;
    r32 angle;
    Color color;
};

struct Rect_object
{
    Rect rect;
    Color color;
};

struct App_memory
{
    b32 is_initialized;
    Memory_arena* permanent_arena;
    Memory_arena* temp_arena;

    Character characters[CHARS_COUNT];
    i16 debug_last_pressed_key;
    List* shortcuts;

    List read_file_q_in;
    List read_file_q_results;

    Color bg_color;
    List* render_bitmap_queue;
    List* text_render_list;
    List* rect_render_list;

    Read_file_result test;
    Platform_functions platform;

    void* draw_queue[1024];

    Text_visual_properties text_visuals;
    
    Text_buffer text_buffer_1;
    Text_rect left_panel;
    
    Text_buffer text_buffer_2;
    Text_rect right_panel;

    Text_buffer command_buffer;
    Text_rect command_line;

    Text_rect* focused_panel;
};

#endif