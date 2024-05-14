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
#ifndef FAST_FOURIER_TRANSFORM_H
#define FAST_FOURIER_TRANSFORM_H

#include <complex>
#include <cstring>

#ifdef WIN32
#include "fftw3/fftw3.h"
#else
#include <fftw3.h>
#endif


typedef std::complex<float> complex;

class fast_fourier_transform
{
private:
    fftwf_complex* in = nullptr;
    fftwf_complex* out_fft = nullptr;
    fftwf_plan plan;
    unsigned int half_fft;
    fftwf_complex* out = nullptr;

public:
    fast_fourier_transform()
    {

    }

    ~fast_fourier_transform()
    {
        if(in != nullptr){
            fftwf_destroy_plan(plan);
            fftwf_free(out);
            fftwf_free(in);
        }
    }

    complex* init(int _len_in)
    {
        in = static_cast<fftwf_complex*>(fftwf_malloc(sizeof(fftwf_complex) * static_cast<unsigned int>(_len_in)));
        out_fft = static_cast<fftwf_complex*>(fftwf_malloc(sizeof(fftwf_complex) * static_cast<unsigned int>(_len_in)));
        plan = fftwf_plan_dft_1d(_len_in, in, out_fft, FFTW_FORWARD, FFTW_ESTIMATE);
        out = static_cast<fftwf_complex*>(fftwf_malloc(sizeof(fftwf_complex) * static_cast<unsigned int>(_len_in)));
        half_fft = static_cast<unsigned int>(_len_in / 2);
        return reinterpret_cast<complex*>(in);
    }

    complex* execute()
    {
        fftwf_execute(plan);
        std::memcpy(&out[half_fft], &out_fft[0], sizeof(complex) * half_fft);
        std::memcpy(&out[0], &out_fft[half_fft], sizeof(complex) * half_fft);
        return reinterpret_cast<complex*>(out);
    }

};

#endif // FAST_FOURIER_TRANSFORM_H
