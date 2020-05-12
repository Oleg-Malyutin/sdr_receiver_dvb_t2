/*
 *  Copyright 2020 Oleg Malyutin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef FAST_MATH_H
#define FAST_MATH_H

#include <math.h>

static float look_up_table_sin[65536];
static float look_up_table_cos[65536];
static const float k_table = 32767.0f / (2.0f * M_PIf32);
//TO DO: need initialize table_sin_cos_instance.table_()
struct table_sin_cos
{
    void table_()
    {
        for (int i = -32767; i < 32768; i++){
            look_up_table_sin[i + 32767] = sinf(i / k_table);
            look_up_table_cos[i + 32767] = cosf(i / k_table);
        }
    }
} static table_sin_cos_instance;
inline float sin_lut(const float &_x){return look_up_table_sin[int(_x * k_table + 32767) & 65535];}
inline float cos_lut(const float &_x){return look_up_table_cos[int(_x * k_table + 32767) & 65535];}

inline float atan2_approx(const float &_y, const float &_x)
{
    if(_x == 0.0f){
        if(_y > 0.0f) return M_PI_2f32;
        else return -M_PI_2f32;
    }
    if(_y == 0.0f){
        if(_x > 0.0f) return 0.0f;
        else return -M_PIf32;
    }
    float abs_x = fabsf(_x);
    float abs_y = fabsf(_y);
    bool min_x = abs_x < abs_y ? true : false;
    float a = min_x ? abs_x / abs_y : abs_y / abs_x;
    float s = a * a;
    float r = ((-4.6496475e-2f * s + 1.5931422e-1f) * s - 3.2762276e-1f) * s * a + a;
    if(min_x) r = M_PI_2f32 - r;
    if(_x < 0.0f) r = M_PIf32 - r;
    if(_y < 0.0f) r = -r;
    return r;
}

#endif // FAST_MATH_H
