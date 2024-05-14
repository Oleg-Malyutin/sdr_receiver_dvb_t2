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

template<typename T, typename F>
class interpolator_farrow
{
private:
    const F k_1_2       = 1.0 / 2.0;
    const F k_1_4       = 1.0 / 4.0;
    const F k_1_8       = 1.0 / 8.0;
    const F k_1_16      = 1.0 / 16.0;
    const F k_3_2       = 3.0 / 2.0;
    const F k_9_16      = 9.0 / 16.0;
    const F k_11_8      = 11.0 / 8.0;
    T delay_data_1 = 0;
    T delay_data_2 = 0;
    T delay_data_3 = 0;
    const F start = -0.5;
    const F tr = 1.0 + start;
    const F next = 1.0;
    F x1 = start;

public:
    interpolator_farrow() {}
    ~interpolator_farrow() {}

    void operator()(int len_in_, T* in_, double &arbitrary_resample_, int &len_out_, T* out_)
    {
        const int len_in = len_in_;
        int idx_out = 0;
        const F delay_x = arbitrary_resample_;
        for(int i = 0; i < len_in; ++i) {
            T in = in_[i];
            T even1 = delay_data_3 + in;
            T even2 = delay_data_2 + delay_data_1;
            T odd1 = delay_data_3 - in;
            T odd2 = delay_data_2 - delay_data_1;
            T a0 = k_9_16 * even2 - k_1_16 * even1;
            T a1 = k_1_8 * odd1 - k_11_8 * odd2;
            T a2 = k_1_4 * (even1 - even2);
            T a3 = k_3_2 * odd2 - k_1_2 * odd1;
            while(x1 < tr){
                F x2 = x1 * x1;
                F x3 = x2 * x1;
                out_[idx_out++] =  a3 * x3 + a2 * x2 + a1 * x1 + a0;
                x1 += delay_x;
            }
            x1 -= next;
            delay_data_3 = delay_data_2;
            delay_data_2 = delay_data_1;
            delay_data_1 = in;
        }
        len_out_ = idx_out;
    }

};

#endif // INTERPOLATOR_FARROW_HH
