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
#ifndef LOOP_FILTERS_HH
#define LOOP_FILTERS_HH

#include <cstdio>

template<typename T, typename F , const F &damping_ratio_, int bw_hz_, int samplerate_hz_>
class proportional_integral_loop_filter
{
private:

    F theta = ((1.0 * bw_hz_) / samplerate_hz_) / (damping_ratio_ + 1.0 / (4.0 * damping_ratio_));
    const F k_p = 4.0 * damping_ratio_ * theta / (1.0 + 2.0 * damping_ratio_ * theta + theta * theta);
    const F k_i = 4.0 * theta * theta / (1.0 + 2.0 * damping_ratio_ * theta + theta * theta );
    T integral_error = 0.0;
    T out_error;
    T old_integral_error = 0.0;


public:
    proportional_integral_loop_filter()
    {
       if(k_i < k_p * k_p / 4.0 || k_i > k_p) {
           fprintf(stderr, "Digital loop PLL unstable:  bw_hz_ = %d \n", bw_hz_);
       }
    };

    T operator()(const T &_in_error, T max_integral_)
    {
        integral_error = old_integral_error + k_i * _in_error;
        out_error = integral_error + k_p * _in_error;
        if(integral_error > max_integral_) integral_error = max_integral_;
        else if(integral_error < -max_integral_) integral_error = -max_integral_;
        old_integral_error = integral_error;
        return out_error;
    }
    void reset()
    {
        old_integral_error = 0.0;
    }
};

template<typename T, typename F, const F &_ratio>
class exponential_averager
{
private:
    F ratio = _ratio;
    T out = 0;

public:
    T operator()(const T _in)
    {
        out = out + ratio * (_in - out);
        return out;
    }
    void reset()
    {
        out = 0;
    }
};

#endif // LOOP_FILTERS_HH
