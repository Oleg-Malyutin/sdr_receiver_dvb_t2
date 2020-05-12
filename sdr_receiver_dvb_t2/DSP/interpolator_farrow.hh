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
#ifndef INTERPOLATOR_FARROW_HH
#define INTERPOLATOR_FARROW_HH

template<typename T>
class interpolator_farrow
{
private:
    const float k_1_2       = 1 / 2.0f;
    const float k_1_4       = 1 / 4.0f;
    const float k_1_8       = 1 / 8.0f;
    const float k_1_16      = 1 / 16.0f;
    const float k_3_2       = 3 / 2.0f;
    const float k_9_16      = 9 / 16.0f;
    const float k_11_8      = 11 / 8.0f;
    T buffer[3];
    T a[4];

    const float start = -0.5f;
//    const float start = -0.685f;
    const float tr = 1.0f + start;

public:
    interpolator_farrow()
    {

    }

    ~interpolator_farrow()
    {

    }

    void operator()(int _len_in, T* _in, float _arbitrary_resample, int &_len_out, T* _out)
    {
        int len_in = _len_in;
        T* in = _in;
        const float delay_x = _arbitrary_resample;
        T even1, even2, odd1, odd2;
        static float x =  start;
        float x2, x3;
        int idx_out = 0;
        for(int i = 0; i < 3; ++i){
            switch(i)
            {
            case 0:
                even1 = buffer[0] + in[0];
                even2 = buffer[1] + buffer[2];
                odd1 = buffer[0] - in[0];
                odd2 = buffer[1] - buffer[2];
                break;
            case 1:
                even1 = buffer[1] + in[1];
                even2 = buffer[2] + in[0];
                odd1 = buffer[1] - in[1];
                odd2 = buffer[2] - in[0];
                break;
            case 2:
                even1 = buffer[2] + in[2];
                even2 = in[0] + in[1];
                odd1 = buffer[2] - in[2];
                odd2 = in[0] - in[1];
                break;
            default:
                break;
            }
            a[0] = k_9_16 * even2 - k_1_16 * even1;
            a[1] = k_1_8 * odd1 - k_11_8 * odd2;
            a[2] = k_1_4 * (even1 - even2);
            a[3] = k_3_2 * odd2 - k_1_2 * odd1;
            while(x < tr){
                x2 = x * x;
                x3 = x2 * x;
                _out[idx_out++] =  a[3] * x3 + a[2] * x2 + a[1] * x + a[0];
                x += delay_x;
            }
            x -= 1.0f;
        }

        int idx_0 = 0;
        int idx_1 = 1;
        int idx_2 = 2;
        int idx_3 = 3;
        for(int i = 3; i < len_in; ++i) {

            even1 = in[idx_0] + in[idx_3];
            even2 = in[idx_1] + in[idx_2];
            odd1 = in[idx_0++] - in[idx_3++];
            odd2 = in[idx_1++] - in[idx_2++];
            a[0] = k_9_16 * even2 - k_1_16 * even1;
            a[1] = k_1_8 * odd1 - k_11_8 * odd2;
            a[2] = k_1_4 * (even1 - even2);
            a[3] = k_3_2 * odd2 - k_1_2 * odd1;
            while(x < tr){
                x2 = x * x;
                x3 = x2 * x;
                _out[idx_out++] =  a[3] * x3 + a[2] * x2 + a[1] * x + a[0];
                x += delay_x;
            }
                x -= 1.0f;
        }
        buffer[0] = in[len_in - 3];
        buffer[1] = in[len_in - 2];
        buffer[2] = in[len_in - 1];

        _len_out = idx_out;
    }

};
#endif // INTERPOLATOR_FARROW_HH
