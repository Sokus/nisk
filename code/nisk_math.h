/* date = December 4th 2021 7:32 pm */

#ifndef NISK_MATH_H
#define NISK_MATH_H

#include "math.h"

#define ROUNDF(a) roundf((a))
#define POWF(base, exp) powf((base), (exp))


float Lerp(float a, float b, float x)
{
    float result = (1-x)*a + x*b;
    return result;
}

float Apporach(float a, float b, float x)
{
    float pct = ABS(x / (b-a));
    float interpolation = CLAMP(0, pct, 1);
    float result = Lerp(a, b, interpolation);
    return result;
}

typedef union Vec2i
{
    struct { int x, y; };
    int e[2];
} Vec2i;

Vec2i Vec2iGet(int x, int y)
{
    Vec2i result;
    result.x = x;
    result.y = y;
    return result;
}

Vec2i Vec2iSubtract(Vec2i a, Vec2i b)
{
    Vec2i result;
    for(int i=0; i<2; ++i)
        result.e[i] = a.e[i] - b.e[i];
    return result;
}

typedef union Vec2
{
    struct { float x, y; };
    float e[2];
} Vec2;


Vec2 Vec2Get(float x, float y)
{
    Vec2 result;
    result.x = x;
    result.y = y;
    return result;
}

Vec2 Vec2Add(Vec2 a, Vec2 b)
{
    Vec2 result;
    for(int i=0; i<2; ++i)
        result.e[i] = a.e[i] + b.e[i];
    return result;
}

Vec2 Vec2Subtract(Vec2 a, Vec2 b)
{
    Vec2 result;
    for(int i=0; i<2; ++i)
        result.e[i] = a.e[i] - b.e[i];
    return result;
}

Vec2 Vec2Scale(Vec2 a, float b)
{
    Vec2 result;
    for(int i=0; i<2; ++i)
        result.e[i] = a.e[i] * b;
    return result;
}

Vec2 Vec2Divide(Vec2 a, float b)
{
    Vec2 result;
    for(int i=0; i<2; ++i)
        result.e[i] = a.e[i] / b;
    return result;
}

float Vec2Dot(Vec2 a, Vec2 b)
{
    float result = a.e[0] * b.e[0] + a.e[1] * b.e[1];
    return result;
}

float Vec2Mag2(Vec2 a)
{
    float result = Vec2Dot(a, a);
    return result;
}

float Vec2Mag(Vec2 a)
{
    float result = sqrtf(Vec2Mag2(a));
    return result;
}

Vec2 Vec2Normalize(Vec2 a)
{
    float magnitude = Vec2Mag(a);
    Vec2 result = Vec2Divide(a, magnitude);
    return result;
}

Vec2 Vec2Lerp(Vec2 a, Vec2 b, float x)
{
    Vec2 result;
    for(int i=0; i<2; i++)
        result.e[i] = Lerp(a.e[i], b.e[i], x);
    return result;
}

#endif //NISK_MATH_H
