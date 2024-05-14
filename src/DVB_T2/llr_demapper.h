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
#ifndef LLR_DEMAPPER_H
#define LLR_DEMAPPER_H

#include <QObject>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>
#include <complex>

#include "dvbt2_definition.h"
#include "ldpc_decoder.h"

typedef std::complex<float> complex;

class llr_demapper : public QObject
{
    Q_OBJECT
public:
    explicit llr_demapper(QWaitCondition *_signal_in, QMutex* _mutex, QObject *parent = nullptr);
    ~llr_demapper();
    ldpc_decoder* decoder;

signals:
    void signal_noise_ratio(float _snr);
    void soft_multiplexer_de_twist(int* _idx_plp_simd, l1_postsignalling _l1_post, int _len_out, int8_t* _out);
    void stop_decoder();
    void finished();

public slots:
    void execute(int _ti_block_size, complex* _time_deint_cell,
                 int _plp_id, l1_postsignalling _l1_post);
    void stop();

private:
    QWaitCondition* signal_in;
    QThread* thread;
    QWaitCondition* signal_out;
    QMutex* mutex_in;
    QMutex* mutex_out;
    l1_postsignalling l1_post;
    int8_t* buffer_a = nullptr;
    int8_t* buffer_b = nullptr;
    bool swap_buffer_start = true;
    bool swap_buffer = true;
    complex derotate_qpsk;
    complex derotate_qam16;
    complex derotate_qam64;
    complex derotate_qam256;

    const int tc_qam16_short[8] = { 0, 0, 0, 1, 7, 20, 20, 21 };
    const int tc_qam16_normal[8] = { 0, 0, 2, 4, 4, 5, 7, 7 };
    const int tc_qam64_short[12] = { 0, 0, 0, 2, 2, 2, 3, 3, 3, 6, 7, 7 };
    const int tc_qam64_normal[12] = { 0, 0, 2, 2, 3, 4, 4, 5, 5, 7, 8, 9 };
    const int tc_qam256_short[8] = { 0, 0, 0, 1, 7, 20, 20, 21 };
    const int tc_qam256_normal[16] = { 0, 2, 2, 2, 2, 3, 7, 15, 16, 20, 22, 22, 27, 27, 28, 32 };

    const int demux_16[8] = {7, 1, 3, 5, 2, 4, 6, 0};
    const int demux_16_fec_size_normal_code_3_5[8] = {0, 2, 3, 6, 4, 1, 7, 5};
    const int demux_64[12] = {11, 8, 5, 2, 10, 7, 4, 1, 9, 6, 3, 0};
    const int demux_64_fec_size_normal_code_3_5[12] = {4, 6, 0, 5, 8, 10, 2, 1, 7, 3, 11, 9};
    const int demux_256_fec_size_short[8] = {7, 2, 4, 1, 6, 3, 5, 0};
    const int demux_256_fec_size_normal[16] = {15, 1, 13, 3, 10, 7, 9, 11, 4, 6, 8, 5, 12, 2, 14, 0};
    const int demux_256_fec_size_normal_3_5[16] = {4, 6, 0, 2, 3, 14, 12, 10, 7, 5, 8, 1, 15, 9, 11, 13};
    const int demux_256_fec_size_normal_2_3[16] = {3, 15, 1, 7, 4, 11, 5, 0, 12, 2, 9, 14, 13, 6, 8, 10};

    int* address_qam16_fecshort;
    int* address_qam16_fecnormal;
    int* address_qam16_fecnormal_3_5;
    int* address_qam64_fecshort;
    int* address_qam64_fecnormal;
    int* address_qam64_fecnormal_3_5;
    int* address_qam256_fecshort;
    int* address_qam256_fecnormal;
    int* address_qam256_fecnormal_3_5;
    int* address_qam256_fecnormal_2_3;

    void address_generator(int _column, int _row, int *_address, const int *_tc, const int *_demux);

    const float norm_16_x1 = NORM_FACTOR_QAM16;
    const float norm_16_x2 = NORM_FACTOR_QAM16 * 2.0f;
    const float norm_16_x3 = NORM_FACTOR_QAM16 * 3.0f;
    const float norm_64_x1 = NORM_FACTOR_QAM64;
    const float norm_64_x2 = NORM_FACTOR_QAM64 * 2.0f;
    const float norm_64_x3 = NORM_FACTOR_QAM64 * 3.0f;
    const float norm_64_x4 = NORM_FACTOR_QAM64 * 4.0f;
    const float norm_64_x5 = NORM_FACTOR_QAM64 * 5.0f;
    const float norm_64_x6 = NORM_FACTOR_QAM64 * 6.0f;
    const float norm_64_x7 = NORM_FACTOR_QAM64 * 7.0f;
    const float norm_256_x1 = NORM_FACTOR_QAM256;
    const float norm_256_x2 = NORM_FACTOR_QAM256 * 2.0f;
    const float norm_256_x3 = NORM_FACTOR_QAM256 * 3.0f;
    const float norm_256_x4 = NORM_FACTOR_QAM256 * 4.0f;
    const float norm_256_x5 = NORM_FACTOR_QAM256 * 5.0f;
    const float norm_256_x6 = NORM_FACTOR_QAM256 * 6.0f;
    const float norm_256_x7 = NORM_FACTOR_QAM256 * 7.0f;
    const float norm_256_x8 = NORM_FACTOR_QAM256 * 8.0f;
    const float norm_256_x9 = NORM_FACTOR_QAM256 * 9.0f;
    const float norm_256_x10 = NORM_FACTOR_QAM256 * 10.0f;
    const float norm_256_x11 = NORM_FACTOR_QAM256 * 11.0f;
    const float norm_256_x12 = NORM_FACTOR_QAM256 * 12.0f;
    const float norm_256_x13 = NORM_FACTOR_QAM256 * 13.0f;
    const float norm_256_x14 = NORM_FACTOR_QAM256 * 14.0f;
    const float norm_256_x15 = NORM_FACTOR_QAM256 * 15.0f;

    void qpsk(int _plp_id, l1_postsignalling _l1_post, int _len_in, complex* _in);
    void qam16(int _plp_id, l1_postsignalling _l1_post, int _len_in, complex* _in);
    void qam64(int _plp_id, l1_postsignalling _l1_post, int _len_in, complex* _in);
    void qam256(int _plp_id, l1_postsignalling _l1_post, int _len_in, complex* _in);

    inline int8_t quantize(float &_precision, float _in);
};

#endif // LLR_DEMAPPER_H
