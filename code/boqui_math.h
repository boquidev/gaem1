
#define SQRT(a) fast_sqrt(a)
#define ABS(x) math_abs(x)
#define SINF(a) sinf(a)
#define COSF(a) cosf(a)
#define TANF(a) tanf(a)
#define ASINF(a) asinf(a)
#define ACOSF(a) acosf(a)
#define ATANF(a) atanf(a)
#define ATAN2F(y,x) atan2f(y,x)

internal u32
round_to_ui32(f32 n)
{
    return (u32)(n+0.5f);
}
internal u8
round_to_ui8(f32 n)
{
    return (u8)(n+0.5f);
}
internal s32
round_to_i32(f32 n)
{
    return(s32)(n+0.5f);
}

internal s32 math_max(s32 n1, s32 n2){return ((n1 < n2) ? n2 :n1);}
internal u32 math_max(u32 n1, u32 n2){return ((n1 < n2) ? n2 :n1);}
internal f32 math_max(f32 n1, f32 n2){return ((n1 < n2) ? n2 :n1);}

internal s32 math_min(s32 n1, s32 n2){return ((n1 < n2) ? n1 : n2);}
internal u32 math_min(u32 n1, u32 n2){return ((n1 < n2) ? n1 : n2);}
internal f32 math_min(f32 n1, f32 n2){return ((n1 < n2) ? n1 : n2);}

internal f32
r32_pow(f32 n, u32 e)
{
    f32 p = 1;
    while(e>0)
    {
        p = p*n;
        e--;
    }
    return p;
}

internal u32
u32_pow(u32 base, u32 exponent)
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

internal f32 math_abs(f32 n) {return ((n > 0) ? n : -1*n);} 
internal s32 math_abs(s32 n) {return ((n > 0) ? n : -1*n);}

// sqrt 

// john carmack's fast square root
internal f32
fast_sqrt(f32 n)
{
    ASSERT(n >= 0);
    f32 x = n * 0.5f;
    f32 y = n; // just to not use the address of the parameter i guess

    s32 i = *(s32*)&y; // casting the float to an integer
    i = 0x5f3759df - (i >> 1); // this is complete sorcery
    y = *(float*)&i; // re casting to a float

    y = y * (1.5f - (x * y * y)); // newton pass
    y = y * (1.5f - (x * y * y)); // optional second newton pass for more precision

    
    return n*y;
}

// newton's algorithm for square root
internal f32
newton_sqrt(f32 n)
{
    ASSERT(n >= 0);
    f32 r = n/2;
    f32 precision = n/1000;
    for(u32 i=0;(ABS(n-(r*r)) > precision && i < 10); i++)
    {
        r = (r+(n/r))/2;
    }
    return r;
}


struct V2
{
    f32 x;
    f32 y;
};
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
operator /(V2 v1, f32 a)
{
    return {v1.x/a, v1.y/a};
}
//Escalar multiplication
internal V2
v2_scalar_product(f32 e, V2 v){
    return {e*v.x, e*v.y};
}
internal V2
v2_scalar_product(s32 e, V2 v){
    return {e*v.x, e*v.y};
}
internal V2
operator *(f32 e, V2 v)
{
    return {e * v.x, e * v.y};
}
internal V2
operator *(s32 e, V2 v)
{
    return {e * v.x, e * v.y};
}
internal V2
operator *(V2 v, f32 e)
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

struct Int2
{
    s32 x;
    s32 y;
};
internal Int2
operator -(Int2 i1, Int2 i2){
    return {i1.x-i2.x, i1.y-i2.y};
}
internal V2
operator /(Int2 v, f32 d){
    return {v.x/d, (s32)v.y/d};
}

internal f32 v2_magnitude(V2 v){return SQRT(v.x*v.x + v.y*v.y);}
internal f32 v2_magnitude(f32 x, f32 y){return v2_magnitude({x,y});}
internal f32 int2_magnitude(Int2 v){return v2_magnitude({(f32)v.x, (f32)v.y});}


internal V2 v2_normalize(V2 v){
    f32 vlength = v2_magnitude(v);
    if(vlength) return v/vlength;
    return {0,0};
}

internal V2 v2_normalize(f32 x, f32 y){return v2_normalize({x,y});}
internal V2 int2_normalize(Int2 v){return v2_normalize((f32)v.x, (f32)v.y);}

internal f32
v2_dot(V2 v1, V2 v2){
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
        f32 x;
        f32 y;
        f32 z;
    };
    struct {
        f32 r;
        f32 g;
        f32 b;
    };
    Int3 i3;
    V2 v2;
};
internal f32
v3_dot(V3 v1, V3 v2){
    return (v1.x*v2.x) + (v1.y*v2.y) + (v1.z*v2.z);
}

internal V3
v3_addition(V3 v1, V3 v2){
    return {v1.x+v2.x, v1.y+v2.y, v1.z+v2.z};
}
internal V3
operator +(V3 v1, V3 v2){
    return v3_addition(v1, v2);
}
internal b32
operator ==(V3 v1, V3  v2){
    return (v1.x==v2.x && v1.y==v2.y && v1.z==v2.z);
}

internal V3
v3_difference(V3 v1, V3 v2){
    V3 result = {0};
    result.x = v1.x - v2.x;
    result.y = v1.y - v2.y;
    result.z = v1.z - v2.z;
    // {v1.x-v2.x, v1.y-v2.y, v1.z-v2.z}
    return result;
}
internal V3
operator -(V3 v1, V3 v2){
    return v3_difference(v1, v2);
}
internal V3
v3_multiply(f32 e, V3 v){
    return {e * v.x, e * v.y, e*v.z};
}
internal V3
operator *(f32 e, V3 v){
    return v3_multiply(e, v);
}
internal V3
operator /(V3 v, f32 x){
    return {v.x / x, v.y / x, v.z /x};
}


internal f32 
v3_magnitude(V3 v){return SQRT(v.x*v.x + v.y*v.y + v.z*v.z);}
internal f32 
v3_magnitude(f32 x, f32 y, f32 z){return v3_magnitude({x, y, z});}

internal V3 v3_normalize(V3 v){
    f32 vlength = v3_magnitude(v);
    if(vlength) return (v/vlength);
    return {0,0,0};
}
internal V3
v3_normalize(f32 x, f32 y, f32 z){return v3_normalize({x, y, z});}

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
        f32 x;
        f32 y;
        f32 z;
        f32 w;
    };
    Int4 i;
    V2 v2;
    V3 v3;
    
};

internal f32
v2_angle(V2 v)
{
    return ATAN2F(v.y, v.x);
}
internal f32
v2_angle(f32 x, f32 y){return v2_angle({x,y});}

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
        f32 xf;
        f32 yf;
        f32 wf;
        f32 hf;
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
    return {(f32)px, (f32)py, (f32)sx, (f32)sy};
}

internal f32
snap_to_grid(f32 value, f32 delta)
{
    f32 difference = value - (f32)((s32)(value/delta));
    return value - difference;
}

internal V3
line_intersect_y0(V3 line_0, V3 line_d){
    f32 t = (-line_0.y / line_d.y);
    f32 x = line_0.x + (t*line_d.x);
    f32 z = line_0.z + (t*line_d.z);
    return {x,0,z};
}

bool line_vs_sphere(V3 line_0, V3 line_v, V3 sphere_center, f32 sphere_radius, f32* closest_t) {
    f32 a = r32_pow(line_v.x, 2) + r32_pow(line_v.y, 2) + r32_pow(line_v.z, 2);
    f32 b = 2 * (line_v.x * (line_0.x - sphere_center.x) +
                    line_v.y * (line_0.y - sphere_center.y) +
                    line_v.z * (line_0.z - sphere_center.z));
    f32 c = r32_pow(line_0.x - sphere_center.x, 2) +
               r32_pow(line_0.y - sphere_center.y, 2) +
               r32_pow(line_0.z - sphere_center.z, 2) -
               r32_pow(sphere_radius, 2);

    f32 discriminant = r32_pow(b, 2) - 4 * a * c;

    b32 result = false;
    if (0 <= discriminant)
    {
        result = true;

        f32 discriminant_sqrt = SQRT(discriminant);

        f32 t1 = (-b + discriminant_sqrt) / (2 * a);
        f32 t2 = (-b - discriminant_sqrt) / (2 * a);

        *closest_t = MIN(t1,t2);
    }
    return result;
}

bool ray_vs_sphere(V3 line_0, V3 line_v, V3 sphere_center, f32 sphere_radius, V3* closest_point) {
    f32 a = r32_pow(line_v.x, 2) + r32_pow(line_v.y, 2) + r32_pow(line_v.z, 2);
    f32 b = 2 * (line_v.x * (line_0.x - sphere_center.x) +
                    line_v.y * (line_0.y - sphere_center.y) +
                    line_v.z * (line_0.z - sphere_center.z));
    f32 c = r32_pow(line_0.x - sphere_center.x, 2) +
               r32_pow(line_0.y - sphere_center.y, 2) +
               r32_pow(line_0.z - sphere_center.z, 2) -
               r32_pow(sphere_radius, 2);

    f32 discriminant = r32_pow(b, 2) - 4 * a * c;

    b32 result = false;
    if (0 <= discriminant)
    {
        f32 discriminant_sqrt = SQRT(discriminant);

        f32 t1 = (-b + discriminant_sqrt) / (2 * a);
        f32 t2 = (-b - discriminant_sqrt) / (2 * a);

        if( t1 > 0 && t2 > 0)
        {
            result = true;
            f32 t = MIN(t1,t2);
            *closest_point = {
                line_0.x + t*line_v.x,
                line_0.y + t*line_v.y,
                line_0.z + t*line_v.z
            };
        }
    }
    return result;
}

/*  returns a vector that represents the distance between 
    the closest point of the box to the center of the sphere
    Check if the magnitude is less than or equal to the sphere radius */
internal V3
sphere_vs_box(V3 sc, V3 bmin, V3 bmax){
    // Calculate the closest point on the box to the sphere
    V3 closest_point;
    closest_point.x = MAX(bmin.x, MIN(sc.x, bmax.x));
    closest_point.y = MAX(bmin.y, MIN(sc.y, bmax.y));
    closest_point.z = MAX(bmin.z, MIN(sc.z, bmax.z));

    // Calculate the distance between the closest point and the sphere center
    V3 distance = sc-closest_point;
    return distance;
}

// return value = overlap, if overlap < 0 then they don't overlap
internal f32 
sphere_vs_sphere(V3 c1,f32 r1, V3 c2, f32 r2){
    return ((r1+r2) - v3_magnitude(c1-c2));
}

internal V2
v2_rotate(V2 vector, f32 angle){
    f32 cos_angle = COSF(angle);
    f32 sin_angle = SINF(angle);

    return {
        vector.x * cos_angle - vector.y * sin_angle,
        vector.x * sin_angle + vector.y * cos_angle
    };
}

// Function to rotate a vector using a rotation matrix
internal V3 
v3_rotate_x(V3 vector, f32 angle) 
{
    f32 cos_angle = COSF(angle);
    f32 sin_angle = SINF(angle);
    
    V3 result = { 
        vector.x,
        vector.y * cos_angle - vector.z * sin_angle,
        vector.y * sin_angle + vector.z * cos_angle
    };
    return result;
}
    
    
internal V3
v3_rotate_y(V3 vector, f32 angle)
{
    f32 cos_angle = COSF(angle);
    f32 sin_angle = SINF(angle);
    
    V3 result = { 
        vector.x * cos_angle + vector.z * sin_angle,
        vector.y,
        -vector.x * sin_angle + vector.z * cos_angle
    };
    return result;
}

internal V3
v3_rotate_z(V3 vector, f32 angle)
{
    f32 cos_angle = COSF(angle);
    f32 sin_angle = SINF(angle);
    
    V3 result = { 
        vector.x * cos_angle - vector.y * sin_angle,
        vector.x * sin_angle + vector.y * cos_angle,
        vector.z
    };

    return result;
}

//TODO: move this to helpers

internal f32
rng_lcg(u32 seed){
    return ((f32)((8121 * seed + 28411) % 134456) / 134456);
}

struct RNG{
    u32 last_seed;

    f32 rng_next(){
        u32 bits_flipped = (last_seed ^ 0xffffffff); // this is my own addition so i may be introducing some problem i don't know
        last_seed =  ((8121 * bits_flipped + 28411) % 134456);
        return (f32)last_seed / 134456;
    }
};

internal f32
rng_rand(u32 seed){
    seed = (seed << 13) ^ seed;
    return (f32)((seed * (seed * seed * 15731 + 789221) + 1376312589) & 0x7fffffff) / 0x7fffffff;
}
