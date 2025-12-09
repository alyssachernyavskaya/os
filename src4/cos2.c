//вторая реализация производной!!!

#include <math.h>

float cosdx(float a, float dx)
{
    return (cosf(a + dx) - cosf(a - dx)) / (2 * dx);
}