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
#ifndef BUFFERS_HH
#define BUFFERS_HH

template<typename T, int LEN>
class sum_of_buffer
{
private:
    const int len = LEN;
    T buffer[LEN];
    int idx_write = 0;
    int idx_read = 1;
    T sum = 0;

public:
    sum_of_buffer() { reset();}

    T operator()(const T &_in)
    {
        buffer[idx_write++] = _in;                // append value to buffer
        if(idx_write == len) idx_write = 0;       // wrap around pointer
        sum = sum - buffer[idx_read++]  + _in;
        if(idx_read == len) idx_read = 0;         // wrap around pointer
        return sum /*/ static_cast<float>(len)*/;
    }
    void reset(){ memset(buffer, 0, sizeof(T) * static_cast<unsigned int>(len));}
};

template<typename T, int DELAY>
class delay_buffer
{
private:
    const int len = DELAY + 1;
    T buffer[DELAY + 1];
    int idx_write = 0;
    int idx_read = 1;

public:
    delay_buffer() { reset();}

    T operator()(const T &_in)
    {
        buffer[idx_write++] = _in;                // append value to buffer
        if(idx_write == len) idx_write = 0;       // wrap around pointer
        if(idx_read == len) idx_read = 0;         // wrap around pointer
        return buffer[idx_read++];
    }
    void reset(){ memset(buffer, 0, sizeof(T) * static_cast<unsigned int>(len));}
};

template<typename T, int LEN>
class save_buffer
{
private:
    const int len = LEN;
    T buffer[LEN];
    int idx = 0;

public:
    save_buffer(){ reset();}
//    ~save_buffer();

    void write(T &_in)
    {
        if(idx == len) idx = 0;     // wrap around pointer
        buffer[idx++] = _in;        // append value to buffer
    }

    T* read()
    {
        const int len_copy = len - idx;
        T temp[len_copy];
        memcpy(temp, buffer + idx, sizeof(T) * static_cast<unsigned int>(len_copy));
        memmove(buffer + len_copy, buffer, sizeof(T) * static_cast<unsigned int>(idx));
        memcpy(buffer, temp, sizeof(T) * static_cast<unsigned int>(len_copy));
        return buffer;

//        memcpy(_out, buffer + idx, sizeof(T) * static_cast<unsigned int>(len_copy));
//        memcpy(_out + len_copy, buffer, sizeof(T) * static_cast<unsigned int>(idx));
    }
    void reset(){ memset(buffer, 0, sizeof(T) * static_cast<unsigned int>(len));}
};

#endif // BUFFERS_HH
