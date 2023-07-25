#include "definitions.h"
#include "json_parser.h"

#define STRING_GL_CHAR "5120"
#define STRING_GL_UCHAR "5121"
#define STRING_GL_SHORT "5122"
#define STRING_GL_USHORT "5123"
#define STRING_GL_INT "5124"
#define STRING_GL_UINT "5125"
#define STRING_GL_FLOAT "5126"

internal s32
gltf_get_component_type_size(String type)
{
    bool is_char = compare_strings(type, STRING_GL_CHAR);
    bool is_uchar = compare_strings(type, STRING_GL_UCHAR);
    bool is_short = compare_strings(type, STRING_GL_SHORT);
    bool is_ushort = compare_strings(type, STRING_GL_USHORT);
    bool is_int = compare_strings(type, STRING_GL_INT);
    bool is_uint = compare_strings(type, STRING_GL_UINT);
    bool is_float = compare_strings(type, STRING_GL_FLOAT);
    if(is_char || is_uchar)
        return 1;
    else if(is_short || is_ushort)
        return 2;
    else if(is_int || is_uint || is_float)
        return 4;
    ASSERT(!"it is not a valid component type");
    return 0;
}

struct GLB
{
    void* json_chunk;
    u32 json_size;

    void* bin_chunk;
    u32 bin_size;
};

struct Mesh_material
{
    bool double_sided;
    Color base_color; // to get this i will need to read string as float
};

enum PRIMITIVE_INDICES
{
    VERTICES_I,
    NORMALS_I,
    TEXCOORDS_I,
    JOINTS_I,
    WEIGHTS_I,
    INDICES_I,
    COLORS_I ,
    MATERIAL_I,
    // 7
};

union Gltf_primitive
{
    struct { // DONT MOVE THE ORDER AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
        V3* vertices;
        V3* normals;
        V2* texcoords;
        V4_u8* joints; // V4 of UCHARS u8
        V4* weights;
        u16* indices;
        Color_u16* colors; // V4 of USHORTS u16

        Mesh_material* material; //TODO: i am skipping this for now

        u32 vertices_count;
        u32 indices_count;
    };
    struct {
        void* properties[7]; //TODO: update this each time a new var is added to the struct above
    };
    //TODO:
    // V3* material; 
};

struct Gltf_mesh
{
    String name;
    Gltf_primitive* primitives;
    u32 primitives_count;
    V3* skely;
};

internal s32
gltf_get_elements_size( String type)
{
    if(compare_strings(type, "VEC3"))
        return 3;
    else if(compare_strings(type, "VEC2"))
        return 2;
    else if(compare_strings(type, "VEC4"))
        return 4;
    else if(compare_strings(type, "SCALAR"))
        return 1;
    else if(compare_strings(type, "MAT4"))
        return 16;
    else ASSERT(!"Undefined type");
    return 0;
}
internal s32
gltf_get_property_index(String p)
{
    if(compare_strings(p, "POSITION"))
        return VERTICES_I;
    else if(compare_strings(p, "NORMAL"))
        return NORMALS_I;
    else if(compare_strings(p, "TEXCOORD_0"))
        return TEXCOORDS_I;
    else if(compare_strings(p, "JOINTS_0"))
        return JOINTS_I;
    else if(compare_strings(p, "WEIGHTS_0"))
        return WEIGHTS_I;
    else if(compare_strings(p, "indices"))
        return INDICES_I;
    else if(compare_strings(p, "COLOR_0"))
        return COLORS_I;
    else if(compare_strings(p, "material"))
        return MATERIAL_I;
    else ASSERT(!"Undefined primitive property");       
    return 0;
}


static bool 
glb_get_chunks(void* file_data, GLB* glb)
{
    if(!compare_mem((char*)file_data, "glTF", 4))
    {
        ASSERT(!"Not a valid gltf file");
        return false;
    }
    u8* scan = (u8*)file_data;
    scan+=4;
    u32 version = SCAN(scan,u32);
    u32 filesize = SCAN(scan,u32);

    //JSON HEADER
    u32 json_chunk_size = SCAN(scan,u32);
    
    if(!compare_mem((char*)scan, "JSON", 4))
    {
        ASSERT(!"did not find json string where it should be");
        return false;
    }
    scan+=4;

    void* json_chunk = scan;
    scan+=json_chunk_size;
    
    //BIN HEADER
    u32 bin_chunk_size = SCAN(scan,u32);

    if(!compare_mem((char*)scan, "BIN", 3))
    {
        ASSERT(!"did not find BIN string where it should be");
        return false;
    }
    scan+=4;
    // do this at the end to make sure everything is correct
    glb->bin_chunk = scan;
    glb->json_chunk = json_chunk;
    glb->json_size = json_chunk_size;
    glb->bin_size = bin_chunk_size;
    return true;
}

static Gltf_mesh* 
gltf_get_meshes(
    GLB* glb,
    Memory_arena* arena,
    u32* meshes_count 
)
{
    Json_buffer json_buffer = {(char*)glb->json_chunk, 0, glb->json_size};
    char* bin_buffer = (char*)glb->bin_chunk; 
    Json_var json_structure = get_json_structure(&json_buffer, arena);
    Json_var* meshes_var = get_json_var(&json_structure, string("meshes"));
    Json_var** accessors_list = get_json_var(&json_structure, string("accessors"))->list;
    Json_var** buffer_views_list = get_json_var(&json_structure, string("bufferViews"))->list;
    Json_var** buffers_list = get_json_var(&json_structure, string("buffers"))->list;

    *meshes_count = LIST_SIZE(meshes_var->list);
    Gltf_mesh* meshes_list = ARENA_PUSH_STRUCTS(arena, Gltf_mesh, LIST_SIZE(meshes_var->list));
    UNTIL(m, LIST_SIZE(meshes_var->list))
    {
        Json_var* current_mesh_var; LIST_GET(meshes_var->list, m, current_mesh_var);
        {
            Json_var* name_var = get_json_var(current_mesh_var, string("name"));
            meshes_list[m].name = name_var->value_data; //TODO: be careful when popping the arena
        } //TODO: primitives is an array, 
            /*
                Each mesh primitive has a 
                rendering mode, which is
                a constant indicating whether
                it should be rendered as
                POINTS, LINES, or TRIANGLES
            */
        Json_var* primitives_var = get_json_var(current_mesh_var, string("primitives"));
        meshes_list[m].primitives = ARENA_PUSH_STRUCTS(arena, Gltf_primitive, LIST_SIZE(primitives_var->list));
        meshes_list[m].primitives_count = LIST_SIZE(primitives_var->list);
        UNTIL(p,LIST_SIZE(primitives_var->list))
        {
            Json_var* current_primitive_var; LIST_GET(primitives_var->list, p, current_primitive_var);

            u32 values_count = 0;
            Json_pair* primitive_values = json_var_get_all_values(
                current_primitive_var, 
                arena,
                &values_count
                );      
            UNTIL(v, values_count)
            {
                s32 primitive_property_index = gltf_get_property_index(primitive_values[v].key);
                if(primitive_property_index == MATERIAL_I) // TODO:
                    continue;

                s32 accessor_i = string_to_int(primitive_values[v].value);
                Json_var* accessor_var; LIST_GET(accessors_list, (u32)accessor_i, accessor_var);
                
                String component_type = get_json_var(accessor_var, string("componentType"))->value_data;
                // TODO: normalize if normalized
                bool normalized = get_json_value_as_bool(accessor_var, string("normalized")); 

                s32 component_type_size  = gltf_get_component_type_size( component_type );
                
                s32 elements_count = get_json_value_as_int(accessor_var, string("count"));
                String elements_type = get_json_var(accessor_var, string("type"))->value_data;
                s32 elements_size = gltf_get_elements_size( elements_type );

                s32 buffer_view_index = get_json_value_as_int(accessor_var, string("bufferView"));
                Json_var* buffer_view_var; 
                LIST_GET(buffer_views_list, (u32)buffer_view_index, buffer_view_var);
                u32 buffer_view_length = get_json_value_as_int(buffer_view_var,string("byteLength"));
                ASSERT((s32)buffer_view_length >= (elements_count * elements_size * component_type_size));
                u32 byte_offset = get_json_value_as_int(buffer_view_var, string("byteOffset"));

                // s32 buffer_index = get_json_value_as_int(buffer_view_var, "buffer");
                //TODO: here i should get the corresponding buffer with the buffer_index but in the glb
                // there might be just 1 buffer

                meshes_list[m].primitives[p].properties[primitive_property_index] = (bin_buffer+byte_offset);
                if(primitive_property_index == VERTICES_I)
                    meshes_list[m].primitives[p].vertices_count = elements_count;
                else if(primitive_property_index == INDICES_I)
                    meshes_list[m].primitives[p].indices_count = elements_count;
            }
        }
    }

    return meshes_list;
}

internal Mesh_primitive
gltf_primitives_to_mesh_primitives(Memory_arena* arena, Gltf_primitive* primitive)
{
    Mesh_primitive result = {0};
    result.vertex_size = sizeof(Vertex3d);
    result.vertices = ARENA_PUSH_STRUCTS(arena,Vertex3d, primitive->vertices_count);
    Vertex3d* vertices = (Vertex3d*)result.vertices;
    V2* texcoords = primitive->texcoords;
    for(u32 i = 0; i<primitive->vertices_count; i++)
    {
        vertices[i].pos.x = primitive->vertices[i].x;
        vertices[i].pos.y = primitive->vertices[i].y;
        vertices[i].pos.z = -primitive->vertices[i].z;
        if(texcoords)
            vertices[i].texcoord = texcoords[i];
        Color_u16* colors = primitive->colors;

        vertices[i].normal.x = primitive->normals[i].x;
        vertices[i].normal.y = primitive->normals[i].y;
        vertices[i].normal.z = -primitive->normals[i].z;
        // I WON'T USE VERTEX COLORING
        // if(colors)
        // {
        //     result.vertices[i].color.r = colors[i].r / (f32)0xffff;
        //     result.vertices[i].color.g = colors[i].g / (f32)0xffff;
        //     result.vertices[i].color.b = colors[i].b / (f32)0xffff;
        // }else{
        //     result.vertices[i].color = {1.0f, 1.0f, 1.0f};
        // }
    }
    result.indices = ARENA_PUSH_STRUCTS(arena, u16, primitive->indices_count);

    for(u32 i=0; i<primitive->indices_count; i++)
    {
        result.indices[i] = primitive->indices[primitive->indices_count-1-i];
    }
    result.vertex_count = primitive->vertices_count;
    result.indices_count = primitive->indices_count;
    return result;
}
