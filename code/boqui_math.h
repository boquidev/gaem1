#ifndef MATH_H
#define MATH_H

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
    while(math_abs(n-(r*r)) > precision)
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

internal r32 magnitude(r32 x, r32 y){return math_sqrt(x*x + y*y);}
internal r32 magnitude(V2 v){return magnitude(v.x, v.y);}
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
            return atan2f(v.y, -v.x) ;
    else 
        return atan2f(v.y, -v.x);
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
snap_to_pixel_grid(r32 value, r32 delta)
{
    r32 difference = value - (r32)((s32)(value/delta));
    return value - difference;
}

#endif