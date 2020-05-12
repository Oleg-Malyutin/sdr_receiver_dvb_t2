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
#ifndef FILTER_DECIMATOR_H
#define FILTER_DECIMATOR_H

#include <QDebug>
#include <complex>
#include <immintrin.h>

typedef std::complex<float> complex;

#define DECIMATION_STEP 2

class filter_decimator
{
private:
    complex* buffer;
    unsigned int idx_write_buffer;
    unsigned int idx_read_buffer;
    unsigned int wrap_point;
    unsigned int begin_point;
    unsigned int len_copy;
    float* h_complex;
    // for 8mHz bandwidht
    const static int len_fir_filter = 64;
    const double h_fir[len_fir_filter] =
    {
        1.0694e-03,  7.2820e-05, -1.6808e-03, -6.3236e-04,  2.2880e-03,  1.5991e-03, -2.7372e-03, -3.0163e-03,  2.8288e-03,  4.8670e-03, -2.3308e-03,
          -7.0571e-03,  9.9638e-04,  9.4028e-03,  1.4167e-03, -1.1624e-02, -5.1319e-03,  1.3342e-02,  1.0354e-02, -1.4063e-02, -1.7303e-02,  1.3142e-02,
           2.6314e-02, -9.6341e-03, -3.8137e-02,  1.8353e-03,  5.4861e-02,  1.4521e-02, -8.4012e-02, -5.6837e-02,  1.7511e-01,  4.2051e-01,  4.2051e-01,
           1.7511e-01, -5.6837e-02, -8.4012e-02,  1.4521e-02,  5.4861e-02,  1.8353e-03, -3.8137e-02, -9.6341e-03,  2.6314e-02,  1.3142e-02, -1.7303e-02,
          -1.4063e-02,  1.0354e-02,  1.3342e-02, -5.1319e-03, -1.1624e-02,  1.4167e-03,  9.4028e-03,  9.9638e-04, -7.0571e-03, -2.3308e-03,  4.8670e-03,
           2.8288e-03, -3.0163e-03, -2.7372e-03,  1.5991e-03,  2.2880e-03, -6.3236e-04, -1.6808e-03,  7.2820e-05,  1.0694e-03
    };

//    const static int len_fir_filter = 64;//65
//    const double h_fir[len_fir_filter] =
//    {
//        7.9906e-04,  8.9851e-04, -9.9497e-04, -1.6862e-03,  9.5775e-04,  2.7191e-03, -5.3570e-04, -3.9165e-03, -4.2425e-04,  5.1293e-03,  2.0545e-03, -6.1372e-03, -4.4439e-03,
//           6.6526e-03,  7.6168e-03, -6.3278e-03, -1.1518e-02,  4.7604e-03,  1.6007e-02, -1.4824e-03, -2.0861e-02, -4.0936e-03,  2.5791e-02,  1.2846e-02, -3.0466e-02, -2.6456e-02,
//           3.4549e-02,  4.9216e-02, -3.7724e-02, -9.7072e-02,  3.9739e-02,  3.1524e-01,  4.5957e-01,  3.1524e-01,  3.9739e-02, -9.7072e-02, -3.7724e-02,  4.9216e-02,  3.4549e-02,
//          -2.6456e-02, -3.0466e-02,  1.2846e-02,  2.5791e-02, -4.0936e-03, -2.0861e-02, -1.4824e-03,  1.6007e-02,  4.7604e-03, -1.1518e-02, -6.3278e-03,  7.6168e-03,  6.6526e-03,
//          -4.4439e-03, -6.1372e-03,  2.0545e-03,  5.1293e-03, -4.2425e-04, -3.9165e-03, -5.3570e-04,  2.7191e-03,  9.5775e-04, -1.6862e-03, -9.9497e-04,  8.9851e-04,/*  7.9906e-04,*/
//    };
//    const static int len_fir_filter = 96;
//    const double h_fir[len_fir_filter] =
//    {
//        -6.0282e-05, -1.5342e-04,  6.2524e-05,  2.9080e-04, -1.0115e-05, -4.6543e-04, -1.2936e-04,  6.5387e-04,  3.8674e-04, -8.1487e-04, -7.8395e-04,
//           8.8950e-04,  1.3256e-03, -8.0450e-04, -1.9904e-03,  4.7912e-04,  2.7245e-03,  1.6452e-04, -3.4371e-03, -1.1885e-03,  3.9999e-03,  2.6249e-03,
//          -4.2515e-03, -4.4616e-03,  4.0062e-03,  6.6298e-03, -3.0660e-03, -8.9954e-03,  1.2345e-03,  1.1354e-02,  1.6712e-03, -1.3426e-02, -5.8147e-03,
//           1.4857e-02,  1.1351e-02, -1.5200e-02, -1.8464e-02,  1.3865e-02,  2.7482e-02, -9.9718e-03, -3.9165e-02,  1.8721e-03,  5.5642e-02,  1.4658e-02,
//          -8.4490e-02, -5.7001e-02,  1.7529e-01,  4.2056e-01,  4.2056e-01,  1.7529e-01, -5.7001e-02, -8.4490e-02,  1.4658e-02,  5.5642e-02,  1.8721e-03,
//          -3.9165e-02, -9.9718e-03,  2.7482e-02,  1.3865e-02, -1.8464e-02, -1.5200e-02,  1.1351e-02,  1.4857e-02, -5.8147e-03, -1.3426e-02,  1.6712e-03,
//           1.1354e-02,  1.2345e-03, -8.9954e-03, -3.0660e-03,  6.6298e-03,  4.0062e-03, -4.4616e-03, -4.2515e-03,  2.6249e-03,  3.9999e-03, -1.1885e-03,
//          -3.4371e-03,  1.6452e-04,  2.7245e-03,  4.7912e-04, -1.9904e-03, -8.0450e-04,  1.3256e-03,  8.8950e-04, -7.8395e-04, -8.1487e-04,  3.8674e-04,
//           6.5387e-04, -1.2936e-04, -4.6543e-04, -1.0115e-05,  2.9080e-04,  6.2524e-05, -1.5342e-04, -6.0282e-05
//    };
//    const double h_fir_odd = h_fir[0];
    const static unsigned int len_filter_complex = len_fir_filter * 2;//for complex numbers
    const static unsigned int len_buffer = len_fir_filter * 2;        // min 2 x len_fir_filter

public:
    filter_decimator()
    {
        if(len_fir_filter % 2 != 0 || len_fir_filter < 16){
            printf("Only even filter and minimum 16 taps /n");
            exit(1);
        }
        h_complex = static_cast<float*>(_mm_malloc(len_filter_complex * sizeof(float), 32));
        int j = 0;
        for (unsigned int i = 0; i < len_fir_filter; ++i) {
            h_complex[j++] = static_cast<float>(h_fir[i]);
            h_complex[j++] = static_cast<float>(h_fir[i]);
        }
        buffer = static_cast<complex*>(_mm_malloc(len_buffer * sizeof(complex), 32));
        for (unsigned int i = 0; i < len_buffer; ++i) buffer[i] = {0.0f, 0.0f};
        wrap_point = len_buffer / 2 + 1;
        begin_point = len_buffer / 2 - 1;
        idx_write_buffer = begin_point;
        idx_read_buffer = 0;
        len_copy = sizeof(complex) * begin_point;
    }
    //-----------------------------------------------------------------------------------------------
    ~filter_decimator()
    {
        _mm_free(h_complex);
        _mm_free(buffer);
    }
    //-----------------------------------------------------------------------------------------------
    void execute(int _len_in, complex* _in, int &_len_out, complex* _out)
    {
    #ifdef RETING
            clock_t start = clock();
    #endif
        int len_in = _len_in;
        float* ptr_buffer;
        int idx_out = 0;
        static int d = 0;
        __m256 v0, v1, v2, v3;          // input vectors
        __m256 h0, h1, h2, h3;          // coefficients vectors
        __m256 m0, m1, m2, m3;
        __m256 sum0, sum1, sumt, sum;
        float st[8] __attribute__((aligned(32)));
        for (int x = 0; x < len_in; ++x) {
            if(idx_write_buffer == len_buffer){
                memmove(buffer, &buffer[wrap_point], len_copy);
                idx_write_buffer = begin_point;
                idx_read_buffer = 0;
            }
            buffer[idx_write_buffer] = _in[x];
            idx_write_buffer++;
            ptr_buffer = reinterpret_cast<float*>(buffer + idx_read_buffer);
            ++idx_read_buffer;
            ++d;
            if(d == DECIMATION_STEP){
                d = 0;
                sum = _mm256_setzero_ps();
                for (unsigned int i = 0; i < len_filter_complex; i += 32){
                    // load inputs into register (unaligned)
                    v0 = _mm256_loadu_ps(ptr_buffer + i);
                    v1 = _mm256_loadu_ps(ptr_buffer + i + 8);
                    v2 = _mm256_loadu_ps(ptr_buffer + i + 16);
                    v3 = _mm256_loadu_ps(ptr_buffer + i + 24);
                    // load coefficients into register (aligned)
                    h0 = _mm256_load_ps(h_complex + i);
                    h1 = _mm256_load_ps(h_complex + i + 8);
                    h2 = _mm256_load_ps(h_complex + i + 16);
                    h3 = _mm256_load_ps(h_complex + i + 24);
                    // compute multiplication
                    m0 = _mm256_mul_ps(v0, h0);
                    m1 = _mm256_mul_ps(v1, h1);
                    m2 = _mm256_mul_ps(v2, h2);
                    m3 = _mm256_mul_ps(v3, h3);
                    // parallel addition
                    sum0 = _mm256_add_ps(m0, m1);
                    sum1 = _mm256_add_ps(m2, m3);
                    sumt = _mm256_add_ps(sum0, sum1);
                    sum = _mm256_add_ps(sum, sumt);
                }
                _mm256_store_ps(st, sum);
                float real_out = st[0] + st[2] + st[4] + st[6];
                float imag_out = st[1] + st[3] + st[5] + st[7];
//                                real_out += static_cast<float>(h_fir_odd) * _in[x].real();
//                                imag_out += static_cast<float>(h_fir_odd) * _in[x].imag();
                // set return value
                _out[idx_out].real(real_out);
                _out[idx_out].imag(imag_out);
                ++idx_out;
            }
        }
        _len_out = idx_out;
    #ifdef RETING
        clock_t end = clock();
        float seconds = (float)(end - start)/* / (CLOCKS_PER_SEC)*/;
        qDebug() << "filter_decimator " << seconds;
    #endif
    }
};

#endif // FILTER_DECIMATOR_H
