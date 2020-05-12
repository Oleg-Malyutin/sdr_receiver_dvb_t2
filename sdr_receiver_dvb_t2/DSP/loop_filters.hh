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

template<typename T, const T &_k_p, const T &_max_integral>
class pid_controller
{
private:
    T k_p = _k_p;
    T k_i = k_p * k_p / 4.0f;
    T integral_error = 0.0;
    T out_error;
    T max_integral = _max_integral;
    T old_integral_error = 0;

public:
    T operator()(const T &_in_error, bool _reset)
    {
        if(_reset) old_integral_error = 0;
        integral_error = old_integral_error + k_i * _in_error;
        out_error = old_integral_error + k_p * _in_error;
        if(integral_error > max_integral) integral_error = max_integral;
        else if(integral_error < -max_integral) integral_error = -max_integral;
        old_integral_error = integral_error;
        return out_error;
    }
};

template<typename T, typename R, const R &_ratio>
class exponential_averager
{
private:
    R ratio = _ratio;
    T out = 0;

public:
    T operator()(const T _in)
    {
        out = out + ratio * (_in - out);
        return out;
    }
};

template<typename T, typename R, const R &_alpha>
class dc_nulling_filter
{
private:
    R alpha = _alpha;
    R beta = (1.0 + alpha) / 2.0;
    T z = 0;
    T out = 0;

public:
    T operator()(const T _in)
    {
        T temp = _in * beta;
        out = out * alpha +  temp - z;
        z = temp;
        return out;
    }
};

#endif // LOOP_FILTERS_HH
