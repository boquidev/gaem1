#define POW(a, b) math_pow(a, b)
#define SQRT(a) math_sqrt(a)
#define ABS(x) math_abs(x)
#define SINF(a) sinf(a)
#define COSF(a) cosf(a)
#define TANF(a) tanf(a)
#define ASINF(a) asinf(a)
#define ACOSF(a) acosf(a)
#define ATANF(a) atanf(a)
#define ATAN2F(y,x) atan2f(y,x)

internal u32
round_to_ui32(r32 n)
{
    return (u32)(n+0.5f);
}
internal u8
round_to_ui8(r32 n)
{
    return (u8)(n+0.5f);
}
internal s32
round_to_i32(r32 n)
{
    return(s32)(n+0.5f);
}

internal s32 math_max(s32 n1, s32 n2){return ((n1 < n2) ? n2 :n1);}
internal u32 math_max(u32 n1, u32 n2){return ((n1 < n2) ? n2 :n1);}
internal r32 math_max(r32 n1, r32 n2){return ((n1 < n2) ? n2 :n1);}

internal s32 math_min(s32 n1, s32 n2){return ((n1 < n2) ? n1 : n2);}
internal u32 math_min(u32 n1, u32 n2){return ((n1 < n2) ? n1 : n2);}
internal r32 math_min(r32 n1, r32 n2){return ((n1 < n2) ? n1 : n2);}

internal r32
math_pow(r32 n, u32 e)
{
    r32 p = 1;
    while(e>0)
    {
        p = p*n;
        e--;
    }
    return p;
}

internal u32
math_pow(u32 base, u32 exponent)
{
    ASSERT(exponent <= 64);
    u32 result = 1;
    while(exponent > 0)
    {
        result *= base;
        exponent--;
    }
    return result;
}

internal r32 math_abs(r32 n) {return ((n > 0) ? n : -1*n);} 
internal s32 math_abs(s32 n) {return ((n > 0) ? n : -1*n);}

internal r32
math_sqrt(r32 n)
{
    ASSERT(n >= 0);
    r32 r = n/2;
    r32 precision = n/1000;
    while(ABS(n-(r*r)) > precision)
    {
        r = (r+(n/r))/2;
    }
    return r;
}

union V2
{
    struct
    {
        r32 x;
        r32 y;
    };
    struct
    {
        r32 u;
        r32 v;
    };
};
struct Int2
{
    s32 x;
    s32 y;
};

internal r32 magnitude(r32 x, r32 y){return SQRT(x*x + y*y);}
internal r32 v2_magnitude(V2 v){return magnitude(v.x, v.y);}
internal r32 magnitude(Int2 v){return magnitude((r32)v.x, (r32)v.y);}

internal V2 normalize(r32 x, r32 y)
{
    r32 vlength = magnitude(x,y);
    if(vlength) return {x/vlength, y/vlength};
    else return {0,0};
}
internal V2 normalize(V2 v){return normalize(v.x, v.y);}
internal V2 normalize(Int2 v){return normalize((r32)v.x, (r32)v.y);}

internal r32
dot(V2 v1, V2 v2)
{
    return (v1.x*v2.x) + (v1.y*v2.y);
}

struct Int3
{
    int x;
    int y;
    int z;
};

union V3
{
    struct {
        r32 r;
        r32 g;
        r32 b;
    };
    struct {
        r32 x;
        r32 y;
        r32 z;
    };
    Int3 i;
    V2 v2;
};
internal V3
v3_addition(V3 v1, V3 v2)
{
    return {v1.x+v2.x, v1.y+v2.y, v1.z+v2.z};
}
internal V3
operator +(V3 v1, V3 v2)
{
    return v3_addition(v1, v2);
}

internal V3
v3_difference(V3 v1, V3 v2)
{
    V3 result = {0};
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    // {v1.x-v2.x, v1.y-v2.y, v1.z-v2.z}
    return result;
}
internal V3
operator -(V3 v1, V3 v2)
{
    return v3_difference(v1, v2);
}
internal V3
operator *(r32 e, V3 v)
{
    return {e * v.x, e * v.y, e*v.z};
}

internal r32 v3_magnitude(r32 x, r32 y, r32 z){return SQRT(x*x + y*y + z*z);}
internal r32 v3_magnitude(V3 v){return v3_magnitude(v.x, v.y, v.z);}
internal V3 
v3_normalize(r32 x, r32 y, r32 z)
{
    r32 vlength = v3_magnitude(x,y,z);
    if(vlength) return {x/vlength, y/vlength, z/vlength};
    else return {0,0,0};
}
internal V3 v3_normalize(V3 v){return v3_normalize(v.x, v.y, v.z);}

struct Int4
{
    int x;
    int y;
    int z;
    int w;
};

union V4
{   
    struct{
        r32 x;
        r32 y;
        r32 z;
        r32 w;
    };
    Int4 i;
    V2 v2;
    V3 v3;
    
};

internal r32
v2_angle(V2 v)
{
    if(v.x < 0)
        // if(v.y < 0)
        //     return atanf(-v.y / v.x);
        // else
            return ATAN2F(v.y, -v.x) ;
    else 
        return ATAN2F(v.y, -v.x);
}


internal V2
v2_addition(V2 v1, V2 v2)
{
    return {v1.x + v2.x, v1.y + v2.y};
}

internal V2
v2_difference(V2 v1, V2 v2)
{
    return {v1.x - v2.x, v1.y - v2.y};
}

internal V2
operator +(V2 v1, V2 v2)
{
    return v2_addition(v1, v2);
}

internal V2
operator -(V2 v1, V2 v2)
{
    return v2_difference(v1, v2);
}

internal V2
operator /(V2 v1, r32 a)
{
    return {v1.x/a, v1.y/a};
}
//Escalar multiplication
internal V2
operator *(r32 e, V2 v)
{
    return {e * v.x, e * v.y};
}
internal V2
operator *(u32 e, V2 v)
{
    return {e * v.x, e * v.y};
}
internal V2
operator *(V2 v, r32 e)
{
    return {e * v.x, e * v.y};
}
internal V2
operator *(V2 v, u32 e)
{
    return {e * v.x, e * v.y};
}
internal b8
operator ==(V2 v1, V2 v2)
{
    return(v1.x == v2.x && v1.y == v2.y);
}
internal b8
operator !=(V2 v1, V2 v2)
{
    return(v1.x != v2.x || v1.y != v2.y);
}

union Rect
{
    struct {
        V2 pos;
        V2 size;
    };
    struct{
        s32 x;
        s32 y;
        s32 w;
        s32 h;
    };
    struct{
        r32 xf;
        r32 yf;
        r32 wf;
        r32 hf;
    };
};

internal Rect
rect(V2 pos, V2 size)
{
    return {pos, size};
};
internal Rect
rect(u32 px, u32 py, u32 sx, u32 sy)
{
    return {(r32)px, (r32)py, (r32)sx, (r32)sy};
}

internal r32
snap_to_grid(r32 value, r32 delta)
{
    r32 difference = value - (r32)((s32)(value/delta));
    return value - difference;
}

bool line_vs_sphere(V3 line_0, V3 line_v, V3 sphere_center, r32 sphere_radius, r32* closest_t) {
    r32 a = POW(line_v.x, 2) + POW(line_v.y, 2) + POW(line_v.z, 2);
    r32 b = 2 * (line_v.x * (line_0.x - sphere_center.x) +
                    line_v.y * (line_0.y - sphere_center.y) +
                    line_v.z * (line_0.z - sphere_center.z));
    r32 c = POW(line_0.x - sphere_center.x, 2) +
               POW(line_0.y - sphere_center.y, 2) +
               POW(line_0.z - sphere_center.z, 2) -
               POW(sphere_radius, 2);

    r32 discriminant = POW(b, 2) - 4 * a * c;

    b32 result = false;
    if (0 <= discriminant)
    {
        result = true;

        r32 discriminant_sqrt = SQRT(discriminant);

        r32 t1 = (-b + discriminant_sqrt) / (2 * a);
        r32 t2 = (-b - discriminant_sqrt) / (2 * a);

        *closest_t = MIN(t1,t2);
    }
    return result;
}

bool ray_vs_sphere(V3 line_0, V3 line_v, V3 sphere_center, r32 sphere_radius, V3* closest_point) {
    r32 a = POW(line_v.x, 2) + POW(line_v.y, 2) + POW(line_v.z, 2);
    r32 b = 2 * (line_v.x * (line_0.x - sphere_center.x) +
                    line_v.y * (line_0.y - sphere_center.y) +
                    line_v.z * (line_0.z - sphere_center.z));
    r32 c = POW(line_0.x - sphere_center.x, 2) +
               POW(line_0.y - sphere_center.y, 2) +
               POW(line_0.z - sphere_center.z, 2) -
               POW(sphere_radius, 2);

    r32 discriminant = POW(b, 2) - 4 * a * c;

    b32 result = false;
    if (0 <= discriminant)
    {
        r32 discriminant_sqrt = SQRT(discriminant);

        r32 t1 = (-b + discriminant_sqrt) / (2 * a);
        r32 t2 = (-b - discriminant_sqrt) / (2 * a);

        if( t1 > 0 && t2 > 0)
        {
            result = true;
            r32 t = MIN(t1,t2);
            *closest_point = {
                line_0.x + t*line_v.x,
                line_0.y + t*line_v.y,
                line_0.z + t*line_v.z
            };
        }
    }
    return result;
}
internal r32 // return value = overlap, if overlap < 0 then they don't overlap
sphere_vs_sphere(V3 c1,r32 r1, V3 c2, r32 r2){
    return ((r1+r2) - v3_magnitude(c1-c2));
}

// Function to rotate a vector using a rotation matrix
internal V3 
v3_rotate_x(V3 vector, r32 angle) 
{
    r32 cos_angle = COSF(angle);
    r32 sin_angle = SINF(angle);
    
    V3 result = { 
        vector.x,
        vector.y * cos_angle - vector.z * sin_angle,
        vector.y * sin_angle + vector.z * cos_angle
    };
    return result;
}
    
    
internal V3
v3_rotate_y(V3 vector, r32 angle)
{
    r32 cos_angle = COSF(angle);
    r32 sin_angle = SINF(angle);
    
    V3 result = { 
        vector.x * cos_angle + vector.z * sin_angle,
        vector.y,
        -vector.x * sin_angle + vector.z * cos_angle
    };
    return result;
}

internal V3
v3_rotate_z(V3 vector, r32 angle)
{
    r32 cos_angle = COSF(angle);
    r32 sin_angle = SINF(angle);
    
    V3 result = { 
        vector.x * cos_angle - vector.y * sin_angle,
        vector.x * sin_angle + vector.y * cos_angle,
        vector.z
    };

    return result;
}

