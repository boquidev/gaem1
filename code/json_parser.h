
enum JSON_VAR_TYPE
{
    JSON_VAR_NULL,
    JSON_VAR_VALUE,
    JSON_VAR_LIST,
    JSON_VAR_OBJECT,
};

enum JSON_VALUE_TYPE
{
    JSON_VALUE_NULL,
    JSON_VALUE_BOOL,
    JSON_VALUE_NUMBER,
    JSON_VALUE_STRING
};

struct Json_pair
{
    String key;
    String value;
};

struct Json_var
{
    String key;
    union 
    {
        struct {
            // even if this is a list it has just one element
            String value_data;
            u32 value_size;    // this is unnecesssary
            JSON_VALUE_TYPE value_type;
        };
        struct {
            LIST(Json_var, list);// Json_var list
        };
        struct {
            LIST(Json_var, object_keys); // Json_var list
        };
    };
    JSON_VAR_TYPE var_type;
};

struct Json_buffer
{
    char* data;
    u32 pos;
    u32 size;
};

internal String
json_next_path_name(Json_buffer* buffer)
{
    String result = {0};
    result.text = BUFFER_GET_POS(buffer); //TODO: this is gross
    while(*BUFFER_GET_POS(buffer) != '/' && buffer->pos < buffer->size)
    {
        buffer->pos++;
        result.length++;
    }
    buffer->pos++;// skipping '/'
    return result;
}

internal Json_pair*
json_var_get_all_values(Json_var* root, Memory_arena* arena, u32* count)
{
    Json_pair* result = (Json_pair*)arena_push_size(arena, 0);
    if(root->var_type == JSON_VAR_VALUE)
    {
        Json_pair* pair = ARENA_PUSH_STRUCT(arena, Json_pair);
        pair->key = root->key;
        pair->value = root->value_data;
        (*count)++;
        return result;
    }
    else if(root->var_type == JSON_VAR_OBJECT || root->var_type == JSON_VAR_LIST)
    {
        for(u32 i=0; i<LIST_SIZE(root->list); i++)
        {
            Json_var* current_var; LIST_GET(root->list, i,current_var);
            json_var_get_all_values(current_var, arena, count);
        }
    }else ASSERT(!"Null or undefined json var type");
    return result;
}

// receive a path in the form root/variable/variable and scans each word
internal Json_var* //TODO: be careful with similar named keys, example: scene, scenes
get_json_var(Json_var* root, String path_to_key) 
{
    Json_buffer json_buffer  = {path_to_key.text, 0, path_to_key.length};
    Json_buffer* buffer = &json_buffer;
    Json_var* posible_result = root;
    while(buffer->pos < buffer->size)
    {
        String key_to_find = json_next_path_name(buffer);
        switch(posible_result->var_type)
        {
            case JSON_VAR_NULL:
                ASSERT(!"wotefoc");
                break;
            case JSON_VAR_OBJECT:
            case JSON_VAR_LIST:
                {
                    for(u32 i=0; i< LIST_SIZE(posible_result->object_keys); i++)
                    {
                        Json_var* current_element; LIST_GET(posible_result->object_keys, i, current_element);
                        if(compare_strings(current_element->key, key_to_find))
                        {
                            posible_result = current_element;
                            break;
                        }
                    }
                }
                break;
            case JSON_VAR_VALUE:
                return posible_result;
                break;
            default:
                break;
        }
    }
    if(posible_result->var_type != JSON_VAR_NULL)
        return posible_result;
    ASSERT(!"did not find a final value");
    return 0; 
}

internal int
get_json_value_as_int(Json_var* root, String path_to_key)
{
    return string_to_int(get_json_var(root, path_to_key)->value_data);
}
internal int
get_json_value_as_bool(Json_var* root, String path_to_key)
{
    return string_to_bool(get_json_var(root, path_to_key)->value_data);
}

static bool
json_is_numeric(char c)
{
    return  c == '-' || c == 'e' ||
            (c >= '0' && c <= '9') ||
            c == '.';
}
static bool
json_is_letter(char c)
{
    return  (c >= 'A' && c <= 'Z') ||
            (c >= 'a' && c <= 'z');
}

// This function extracts a string value from the json file, skipping the ""
static void 
json_scan_string(Json_buffer* buffer, Memory_arena* arena, String* result)
{
    result->length = 0;
    buffer->pos++;
    result->text = (char*)arena_push_size(arena,0);//TODO: TEST WITHOUT USING ARENA
    while(*BUFFER_GET_POS(buffer) != '\"')
    {
        arena_push_data(arena, BUFFER_GET_POS(buffer), 1);
        buffer->pos++;
        result->length++;
    }
    arena_push_size(arena, 1); // 0 ending string
    buffer->pos++; //skipping the second quotation mark ""
}

// This function extracts a numeric value from the json file
static void 
json_scan_number(Json_buffer* buffer, Memory_arena* arena, String* result)
{
    while(!json_is_numeric(*BUFFER_GET_POS(buffer)))
        buffer->pos++;
    result->text = (char*)arena_push_size(arena,0);
    while(json_is_numeric(*BUFFER_GET_POS(buffer)))
    {
        arena_push_data(arena, BUFFER_GET_POS(buffer), 1);
        buffer->pos++;
        result->length++;
    }
    arena_push_size(arena, 1); // 0 ending string
}
// This function extracts a boolean value from the json file
static void
json_scan_bool(Json_buffer* buffer, Memory_arena* arena, String* result)
{
    while(!json_is_letter(*BUFFER_GET_POS(buffer)))
        buffer->pos++;
    result->text = (char*)arena_push_size(arena,0);
    while(json_is_letter(*BUFFER_GET_POS(buffer)))
    {
        arena_push_data(arena, BUFFER_GET_POS(buffer), 1);
        buffer->pos++;
        result->length++;
    }
    arena_push_size(arena, 1); // 0 ending string
}

static void
scan_json_to_structure(Json_buffer* buffer, Memory_arena* arena, Json_var* current_var)
{
    while(buffer->pos < buffer->size)
    {
        char current_char = *(buffer->data+buffer->pos);
        if(compare_strings(current_var->key, "primitives"))//TODO: DELETE THIS
            u32 x = 0;

        if(current_var->var_type == JSON_VAR_NULL)
        {
            switch(current_char)
            {
                case '{':
                    current_var->var_type = JSON_VAR_OBJECT;
                    break;
                case '[':
                    current_var->var_type = JSON_VAR_LIST;
                    break;
                default:
                    if(
                        current_char == '\"' || 
                        json_is_numeric(current_char) ||
                        json_is_letter(current_char)
                        )
                    {
                        current_var->var_type = JSON_VAR_VALUE;
                    }
                break;
            }
        }

        if(current_var->var_type == JSON_VAR_OBJECT)
        {
            switch(current_char)
            {
                case '}':
                {
                    return ;
                    break;
                }
                case '\"':
                {
                    Json_var* new_var; PUSH_BACK(current_var->object_keys, arena, new_var);
                    json_scan_string(buffer, arena, &new_var->key);

                    scan_json_to_structure(buffer, arena, new_var);
                    if(*BUFFER_GET_POS(buffer) == '}')
                    {
                        buffer->pos++;
                        return;
                    }
                    break;
                }
                default:
                    // if(is_whitespace(current_char))
                    break;
            }
        }
        else if(current_var->var_type == JSON_VAR_VALUE)
        {
            if(current_char == '\"')
            {
                current_var->value_type = JSON_VALUE_STRING;
                json_scan_string(buffer, arena, &current_var->value_data);
                return ;
            }else if( json_is_numeric(current_char) )
            {
                current_var->value_type = JSON_VALUE_NUMBER;
                json_scan_number(buffer, arena, &current_var->value_data);
                return ;
            }else if( json_is_letter(current_char) )
            {
                current_var->value_type = JSON_VALUE_BOOL;   
                json_scan_bool(buffer, arena, &current_var->value_data);
                return ;
            }
        }
        else if(current_var->var_type == JSON_VAR_LIST)
        {
            switch(current_char)
            {
                case ']':
                    return;
                    break;
                case '{':
                {   //TODO: this is useless for now
                    // current_var->list_type = JSON_VAR_OBJECT;
                    Json_var* new_var; PUSH_BACK(current_var->list, arena, new_var); 
                    ASSERT(LIST_SIZE(current_var->list));
                    new_var->key = number_to_string(LIST_SIZE(current_var->list)-1, arena);
                    scan_json_to_structure(buffer, arena, new_var);
                    if(*BUFFER_GET_POS(buffer) == ']')
                    {
                        buffer->pos++;
                        return;
                    }
                    break;
                }   
                default:
                    if  (
                        current_char == '\"'            ||
                        json_is_numeric(current_char)   ||
                        json_is_letter(current_char)
                        )
                    { //TODO: this is useless for now
                        // current_var->list_type = JSON_VAR_VALUE;
                        Json_var* new_var; PUSH_BACK(current_var->list, arena, new_var); 
                        ASSERT(LIST_SIZE(current_var->list));
                        new_var->key = number_to_string(LIST_SIZE(current_var->list)-1, arena);
                        scan_json_to_structure(buffer, arena, new_var);
                        if(*BUFFER_GET_POS(buffer) == ']')
                        {
                            buffer->pos++;
                            return;
                        }
                        break;
                    }
                    break;
            }
        }
        else if(current_var->var_type != JSON_VAR_NULL)
        {
            ASSERT(!"Undefined var_type");
        }
        buffer->pos++;
    }
}

static Json_var
get_json_structure(Json_buffer* buffer,Memory_arena* arena)
{
    Json_var result = {0};
    result.key.text = "root";
    scan_json_to_structure(buffer, arena, &result);
    
    return result;
}

static u32
format_json_more_readable(void* json_strip, u32 json_size, void* out)
{
    u8* json_scan = (u8*)json_strip;
    u8* out_scan = (u8*)out;
    u32 i = 0;
    u32 depth = 0;
    u32 out_size = json_size;
    for(u32 json_i = 0; json_i<json_size; json_i++)
    {
        switch(json_scan[json_i])
        {
            case ',':
                out_scan[i] = json_scan[json_i];
                i++;
                out_scan[i] = '\n';
                for(u32 d=0; d<depth; d++)
                {
                    i++;
                    out_scan[i] = 9;
                }
                break;
            case '{':
                depth++;
                out_scan[i] = json_scan[json_i];
                i++;
                out_scan[i] = '\n';
                for(u32 d=0; d<depth; d++)
                {
                    i++;
                    out_scan[i] = 9;
                }
                break;
            case '[':
                depth++;
                out_scan[i] = '\n';
                for(u32 d=0; d<depth; d++)
                {
                    i++;
                    out_scan[i] = 9;
                }
                out_scan[i] = json_scan[json_i];
                i++;
                out_scan[i] = '\n';
                for(u32 d=0; d<depth; d++)
                {
                    i++;
                    out_scan[i] = 9;
                }
                break;
            case '}':
            case ']':
                depth--;
                out_scan[i] = '\n';
                for(u32 d=0; d<depth; d++)
                {
                    i++;
                    out_scan[i] = 9;
                }
                i++;
                out_scan[i] = json_scan[json_i];
                break;
            default:
                out_scan[i] = json_scan[json_i];
            break;
        }
        out_size = i;
        i++;
    }

    return out_size;
}