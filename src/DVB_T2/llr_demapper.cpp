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
#include "llr_demapper.h"

//#include <QDebug>
#include <immintrin.h>

#if defined(_MSC_VER)
#define ALIGNED_(x) __declspec(align(x))
#else
#if defined(__GNUC__)
#define ALIGNED_(x) __attribute__ ((aligned(x)))
#endif
#endif

//------------------------------------------------------------------------------------------
llr_demapper::llr_demapper(QWaitCondition *_signal_in, QMutex* _mutex, QObject* parent) :
    QObject(parent),
    signal_in(_signal_in),
    mutex_in(_mutex)
{
    derotate_qpsk.real(cos(-ROT_QPSK));
    derotate_qpsk.imag(sin(-ROT_QPSK));
    derotate_qam16.real(cos(-ROT_QAM16));
    derotate_qam16.imag(sin(-ROT_QAM16));
    derotate_qam64.real(cos(-ROT_QAM64));
    derotate_qam64.imag(sin(-ROT_QAM64));
    derotate_qam256.real(cos(-ROT_QAM256));
    derotate_qam256.imag(sin(-ROT_QAM256));

    //address column twist deinterleaved and demultiplexer
    int column, row;
    column = 2025;
    row = 8;
    address_qam16_fecshort = new int[FEC_SIZE_SHORT];
    address_generator(column, row, address_qam16_fecshort, tc_qam16_short, demux_16);
    column = 8100;
    address_qam16_fecnormal = new int[FEC_SIZE_NORMAL];
    address_generator(column, row, address_qam16_fecnormal, tc_qam16_normal, demux_16);
    address_qam16_fecnormal_3_5 = new int[FEC_SIZE_NORMAL];
    address_generator(column, row, address_qam16_fecnormal_3_5, tc_qam16_normal, demux_16_fec_size_normal_code_3_5);
    column = 1350;
    row = 12;
    address_qam64_fecshort = new int[FEC_SIZE_SHORT];
    address_generator(column, row, address_qam64_fecshort, tc_qam64_short, demux_64);
    column = 5400;
    address_qam64_fecnormal = new int[FEC_SIZE_NORMAL];
    address_generator(column, row, address_qam64_fecnormal, tc_qam64_normal, demux_64);
    address_qam64_fecnormal_3_5 = new int[FEC_SIZE_NORMAL];
    address_generator(column, row, address_qam64_fecnormal_3_5, tc_qam64_normal, demux_64_fec_size_normal_code_3_5);
    column = 2025;
    row = 8;
    address_qam256_fecshort = new int[FEC_SIZE_SHORT];
    address_generator(column, row, address_qam256_fecshort, tc_qam256_short, demux_256_fec_size_short);
    column = 4050;
    row = 16;
    address_qam256_fecnormal = new int[FEC_SIZE_NORMAL];
    address_generator(column, row, address_qam256_fecnormal, tc_qam256_normal, demux_256_fec_size_normal);
    address_qam256_fecnormal_3_5 = new int[FEC_SIZE_NORMAL];
    address_generator(column, row, address_qam256_fecnormal_3_5, tc_qam256_normal, demux_256_fec_size_normal_3_5);
    address_qam256_fecnormal_2_3 = new int[FEC_SIZE_NORMAL];
    address_generator(column, row, address_qam256_fecnormal_2_3, tc_qam256_normal, demux_256_fec_size_normal_2_3);

    buffer_a = new int8_t[FEC_SIZE_NORMAL * SIZEOF_SIMD];
    buffer_b = new int8_t[FEC_SIZE_NORMAL * SIZEOF_SIMD];

    mutex_out = new QMutex;
    signal_out = new QWaitCondition;
    decoder = new ldpc_decoder(signal_out, mutex_out);
    thread = new QThread;
    decoder->moveToThread(thread);
    connect(this, &llr_demapper::soft_multiplexer_de_twist, decoder, &ldpc_decoder::execute);
    connect(this, &llr_demapper::stop_decoder, decoder, &ldpc_decoder::stop);
    connect(decoder, &ldpc_decoder::finished, decoder, &ldpc_decoder::deleteLater);
    connect(decoder, &ldpc_decoder::finished, thread, &QThread::quit, Qt::DirectConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}
//------------------------------------------------------------------------------------------
llr_demapper::~llr_demapper()
{
    emit stop_decoder();
    if(thread->isRunning()) thread->wait(1000);
    delete [] buffer_a;
    delete [] buffer_b;
    delete [] address_qam16_fecshort;
    delete [] address_qam16_fecnormal;
    delete [] address_qam16_fecnormal_3_5;
    delete [] address_qam64_fecshort;
    delete [] address_qam64_fecnormal;
    delete [] address_qam64_fecnormal_3_5;
    delete [] address_qam256_fecshort;
    delete [] address_qam256_fecnormal;
    delete [] address_qam256_fecnormal_2_3;
    delete [] address_qam256_fecnormal_3_5;
}
//------------------------------------------------------------------------------------------
void llr_demapper::address_generator(int _column, int _row, int* _address, const int* _tc,
                                     const int* _demux)
{
    int address[FEC_SIZE_NORMAL];
    for(int c = 0; c < _column; ++c){
        for(int r = 0; r < _row; ++r){
            address[c * _row + r] = _column * r + (c + _column - _tc[r]) % _column;
        }
    }
    int k = 0;
    int n = 0;
    int frame = _column * _row;
    for(int i = 0; i < frame; ++i){
        _address[i] =  address[_demux[n] + k];
        ++n;
        if(n == _row) {
            n = 0;
            k += _row;
        }
    }
}
//------------------------------------------------------------------------------------------
void llr_demapper::execute(int _ti_block_size, complex* _time_deint_cell,
                               int _plp_id, l1_postsignalling _l1_post)
{
    mutex_in->lock();
    signal_in->wakeOne();
    int plp_id = _plp_id;
    l1_postsignalling l1_post = _l1_post;
    int len_in = _ti_block_size;
    complex* in = _time_deint_cell;
    switch(l1_post.plp[plp_id].plp_mod){
    case MOD_64QAM:
        qam64(plp_id, l1_post, len_in, in);
        break;
    case MOD_256QAM:
        qam256(plp_id, l1_post, len_in, in);
        break;
    case MOD_16QAM:
        qam16(plp_id, l1_post, len_in, in);
        break;
    case MOD_QPSK:
        qpsk(plp_id, l1_post, len_in, in);
        break;
    default:
        break;
    }
    mutex_in->unlock();
}
//------------------------------------------------------------------------------------------
void llr_demapper::qpsk(int _plp_id, l1_postsignalling _l1_post, int _len_in, complex* _in)
{
    int plp_id = _plp_id;
    int idx_plp_simd[SIZEOF_SIMD];
    l1_postsignalling l1_post = _l1_post;
    int len_in = _len_in;
    bool derotate = false;
    if(l1_post.plp[plp_id].plp_rotation != 0) derotate = true;
    dvbt2_fectype_t fec_type = static_cast<dvbt2_fectype_t>(l1_post.plp[plp_id].plp_fec_type);
    int fec_size = FEC_SIZE_NORMAL;
    if(fec_type == FECFRAME_SHORT) fec_size = FEC_SIZE_SHORT;
    int idx_out = 0;
    static int blocks = 0;
    static int8_t* out;
    if(swap_buffer) out = buffer_a;
    else out = buffer_b;
    float sum_s = 0;
    float sum_e = 0;
    float snr, precision;
    if(derotate) {
        for(int i = 0; i < len_in; ++i) _in[i] *=  derotate_qpsk;
    }
    complex in;
    //hard demap
    for(int i = 0; i < 2048; ++i) {
        complex s, e;
        in = _in[i];
        if(in.real() > 0)s.real(NORM_FACTOR_QPSK);
        else s.real(-NORM_FACTOR_QPSK);
        if(in.imag() > 0)s.imag(NORM_FACTOR_QPSK);
        else s.imag(-NORM_FACTOR_QPSK);
        e = in - s;
        sum_s += std::norm(s);
        sum_e += std::norm(e);
    }
    snr = 10.0f * std::log10(sum_s / sum_e);
    emit signal_noise_ratio(snr);
    precision = 8.0f * NORM_FACTOR_QPSK * sum_s / sum_e;
    //soft demap
    for(int i = 0; i < len_in; ++i) {
        complex in;
        in = _in[i];
        out[idx_out++] = quantize(precision, in.real());
        out[idx_out++] = quantize(precision, in.imag());
        if(idx_out == fec_size){
            idx_out = 0;
            out += fec_size;
            idx_plp_simd[blocks] = plp_id;
            ++blocks;
            if(blocks == SIZEOF_SIMD) {
                blocks = 0;
                int len_out = fec_size * SIZEOF_SIMD;
                mutex_out->lock();
                if(swap_buffer) {
                    swap_buffer = false;
                    emit soft_multiplexer_de_twist(idx_plp_simd, l1_post, len_out, buffer_a);
                    out = buffer_b;
                }
                else {
                    swap_buffer = true;
                    emit soft_multiplexer_de_twist(idx_plp_simd, l1_post, len_out, buffer_b);
                    out = buffer_a;
                }
                signal_out->wait(mutex_out);
                mutex_out->unlock();
            }
        }
    }
}
//------------------------------------------------------------------------------------------
void llr_demapper::qam16(int _plp_id, l1_postsignalling _l1_post, int _len_in, complex* _in)
{
    int plp_id = _plp_id;
    int idx_plp_simd[SIZEOF_SIMD];
    l1_postsignalling l1_post = _l1_post;
    int len_in = _len_in;
    bool derotate = false;
    if(l1_post.plp[plp_id].plp_rotation != 0) derotate = true;
    dvbt2_fectype_t fec_type = static_cast<dvbt2_fectype_t>(l1_post.plp[plp_id].plp_fec_type);
    dvbt2_code_rate_t code_rate = static_cast<dvbt2_code_rate_t>(l1_post.plp[plp_id].plp_cod);
    int fec_size;
    int idx_out = 0;
    static int blocks = 0;
    static int8_t* out;
    if(blocks == 0){
        if(swap_buffer) out = buffer_a;
        else out = buffer_b;
    }
    if(derotate){
        for(int i = 0; i < len_in; ++i) _in[i] *=  derotate_qam16;
    }
    //hard demap
    float sum_s = 0;
    float sum_e = 0;
    float snr;
    complex in;
    //hard demap
    for(int i = 0; i < len_in; ++i) {
        complex s, e;
        in = _in[i];
        if(in.real() > 0){
            if(in.real() > norm_16_x2) s.real(norm_16_x3);
            else s.real(norm_16_x1);
        }
        else{
            if(in.real() < -norm_16_x2) s.real(-norm_16_x3);
            else s.real(-norm_16_x1);
        }
        if(in.imag() > 0){
            if(in.imag() > norm_16_x2) s.imag(norm_16_x3);
            else s.imag(norm_16_x1);
        }
        else{
            if(in.imag() < -norm_16_x2) s.imag(-norm_16_x3);
            else s.imag(-norm_16_x1);
        }
        e = in - s;
        sum_s += std::norm(s);
        sum_e += std::norm(e);
    }
    snr = 20.0f * std::log10(sum_s / sum_e);
    emit signal_noise_ratio(snr);
    //soft demap
    int* address;
    int* address_begin = nullptr;
    float precision;
    float ALIGNED_(32) llr_bits_0_1[8] ;
    float ALIGNED_(32) llr_bits_2_3[8] ;
    __m256  v_precision, v_norm_x2, signbits;
    __m256 v_in, v_mul, v_sub, v_abs_in, v_round;
    precision = 8.0f * NORM_FACTOR_QAM16 * sum_s / sum_e;
    v_precision = _mm256_set1_ps(precision);
    v_norm_x2 = _mm256_set1_ps(norm_16_x2);
    signbits = _mm256_set1_ps(-0.0f);
    if(fec_type == FEC_FRAME_NORMAL) {
        fec_size = FEC_SIZE_NORMAL;
        if(code_rate == C3_5) address = address_qam16_fecnormal_3_5;
        else address = address_qam16_fecnormal;
    }
    else{
        fec_size = FEC_SIZE_SHORT;
        address = address_qam16_fecshort;
    }
    address_begin = address;

    for(int i = 0; i < len_in; i += 4) {

        float* in = reinterpret_cast<float*>(_in + i);
        v_in = _mm256_load_ps(in);
        v_mul = _mm256_mul_ps(v_in, v_precision);
        v_round = _mm256_round_ps(v_mul ,_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
        _mm256_store_ps(llr_bits_0_1, v_round);

        v_abs_in = _mm256_andnot_ps(signbits, v_in);
        v_sub = _mm256_sub_ps(v_abs_in, v_norm_x2);
        v_mul = _mm256_mul_ps(v_sub, v_precision);
        v_round = _mm256_round_ps(v_mul ,_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
        _mm256_store_ps(llr_bits_2_3, v_round);

        int n = 0;
        for(int i = 0; i < 2; ++i){
            int even_1 = n;
            int odd_1  = n + 1;
            int even_2 = n + 2;
            int odd_2  = n + 3;

            out[address[0]] = static_cast<int8_t>(llr_bits_0_1[even_1]);
            out[address[1]] = static_cast<int8_t>(llr_bits_0_1[odd_1]);
            out[address[2]] = static_cast<int8_t>(llr_bits_2_3[even_1]);
            out[address[3]] = static_cast<int8_t>(llr_bits_2_3[odd_1]);
            out[address[4]] = static_cast<int8_t>(llr_bits_0_1[even_2]);
            out[address[5]] = static_cast<int8_t>(llr_bits_0_1[odd_2]);
            out[address[6]] = static_cast<int8_t>(llr_bits_2_3[even_2]);
            out[address[7]] = static_cast<int8_t>(llr_bits_2_3[odd_2]);

            n += 4;
            address += 8;
        }
        idx_out += 16;
        if(idx_out == fec_size) {
            idx_out = 0;
            address = address_begin;
            out += fec_size;
            idx_plp_simd[blocks] = plp_id;
            ++blocks;
            if(blocks == SIZEOF_SIMD){
                blocks = 0;
                int len_out = fec_size * SIZEOF_SIMD;
                mutex_out->lock();
                if(swap_buffer) {
                    swap_buffer = false;
                    emit soft_multiplexer_de_twist(idx_plp_simd, l1_post, len_out, buffer_a);
                    out = buffer_b;
                }
                else {
                    swap_buffer = true;
                    emit soft_multiplexer_de_twist(idx_plp_simd, l1_post, len_out, buffer_b);
                    out = buffer_a;
                }
                signal_out->wait(mutex_out);
                mutex_out->unlock();
            }
        }
    }
}
//------------------------------------------------------------------------------------------
void llr_demapper::qam64(int _plp_id, l1_postsignalling _l1_post, int _len_in, complex* _in)
{
    int plp_id = _plp_id;
    int idx_plp_simd[SIZEOF_SIMD];
    l1_postsignalling l1_post = _l1_post;
    int len_in = _len_in;
    bool derotate = false;
    if(l1_post.plp[plp_id].plp_rotation != 0) derotate = true;
    dvbt2_fectype_t fec_type = static_cast<dvbt2_fectype_t>(l1_post.plp[plp_id].plp_fec_type);
    dvbt2_code_rate_t code_rate = static_cast<dvbt2_code_rate_t>(l1_post.plp[plp_id].plp_cod);
    int fec_size;
    int idx_out = 0;
    static int blocks = 0;
    static int8_t* out;
    if(blocks == 0) {
        if(swap_buffer) out = buffer_a;
        else out = buffer_b;
    }
    if(derotate) {
        for(int i = 0; i < len_in; ++i) _in[i] *=  derotate_qam64;
    }
    //hard demap
    float sum_s = 0;
    float sum_e = 0;
    float snr;
    complex in;
    for(int i = 0; i < len_in; ++i) {
        complex s, e;
        in = _in[i];
        if(in.real() > 0){
            if(in.real() > norm_64_x4){
                if(in.real() > norm_64_x6)s.real(norm_64_x7);
                else s.real(norm_64_x5);
            }
            else{
                if(in.real() > norm_64_x2)s.real(norm_64_x3);
                else s.real(norm_64_x1);
            }
        }
        else{
            if(in.real() < -norm_64_x4){
                if(in.real() > norm_64_x6)s.real(-norm_64_x7);
                else s.real(-norm_64_x5);
            }
            else{
                if(in.real() < -norm_64_x2)s.real(-norm_64_x3);
                else s.real(-norm_64_x1);
            }
        }
        if(in.imag() > 0){
            if(in.imag() > norm_64_x4){
                if(in.imag() > norm_64_x6)s.imag(norm_64_x7);
                else s.imag(norm_64_x5);
            }
            else{
                if(in.imag() > norm_64_x2)s.imag(norm_64_x3);
                else s.imag(norm_64_x1);
            }
        }
        else{
            if(in.imag() < -norm_64_x4){
                if(in.imag() > norm_64_x6)s.imag(-norm_64_x7);
                else s.imag(-norm_64_x5);
            }
            else{
                if(in.imag() < -norm_64_x2)s.imag(-norm_64_x3);
                else s.imag(-norm_64_x1);
            }
        }
        e = in - s;
        sum_s += std::norm(s);
        sum_e += std::norm(e);
    }
    snr = 20.0f * std::log10(sum_s / sum_e);
    emit signal_noise_ratio(snr);
    //soft demap
    int* address;
    int* address_begin = nullptr;
    float precision;
    float ALIGNED_(32) llr_bits_0_1[8];
    float ALIGNED_(32) llr_bits_2_3[8];
    float ALIGNED_(32) llr_bits_4_5[8];
    __m256  v_precision, v_norm_x4, v_norm_x2, signbits;
    __m256 v_in, v_mul, v_sub, v_abs_in, v_abs_in_sub_x, v_round;
    precision = 8.0f * NORM_FACTOR_QAM64 * sum_s / sum_e;
    v_precision = _mm256_set1_ps(precision);
    v_norm_x4 = _mm256_set1_ps(norm_64_x4);
    v_norm_x2 = _mm256_set1_ps(norm_64_x2);
    signbits = _mm256_set1_ps(-0.0f);
    if(fec_type == FEC_FRAME_NORMAL) {
        fec_size = FEC_SIZE_NORMAL;
        if(code_rate == C3_5) address = address_qam64_fecnormal_3_5;
        else address = address_qam64_fecnormal;
    }
    else{
        fec_size = FEC_SIZE_SHORT;
        address = address_qam64_fecshort;
    }
    address_begin = address;

    for(int i = 0; i < len_in; i += 4) {

        float* in = reinterpret_cast<float*>(_in + i);
        v_in = _mm256_load_ps(in);
        v_mul = _mm256_mul_ps(v_in, v_precision);
        v_round = _mm256_round_ps(v_mul ,_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
        _mm256_store_ps(llr_bits_0_1, v_round);

        v_abs_in = _mm256_andnot_ps(signbits, v_in);
        v_sub = _mm256_sub_ps(v_abs_in, v_norm_x4);
        v_mul = _mm256_mul_ps(v_sub, v_precision);
        v_round = _mm256_round_ps(v_mul ,_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
        _mm256_store_ps(llr_bits_2_3, v_round);

        v_abs_in_sub_x = _mm256_andnot_ps(signbits, v_sub);
        v_sub = _mm256_sub_ps(v_abs_in_sub_x, v_norm_x2);
        v_mul = _mm256_mul_ps(v_sub, v_precision);
        v_round = _mm256_round_ps(v_mul ,_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
        _mm256_store_ps(llr_bits_4_5, v_round);

        int n = 0;
        for(int i = 0; i < 2; ++i) {
            int even_1 = n;
            int odd_1  = n + 1;
            int even_2 = n + 2;
            int odd_2  = n + 3;

            out[address[0]] = static_cast<int8_t>(llr_bits_0_1[even_1]);
            out[address[1]] = static_cast<int8_t>(llr_bits_0_1[odd_1]);
            out[address[2]] = static_cast<int8_t>(llr_bits_2_3[even_1]);
            out[address[3]] = static_cast<int8_t>(llr_bits_2_3[odd_1]);
            out[address[4]] = static_cast<int8_t>(llr_bits_4_5[even_1]);
            out[address[5]] = static_cast<int8_t>(llr_bits_4_5[odd_1]);
            out[address[6]] = static_cast<int8_t>(llr_bits_0_1[even_2]);
            out[address[7]] = static_cast<int8_t>(llr_bits_0_1[odd_2]);
            out[address[8]] = static_cast<int8_t>(llr_bits_2_3[even_2]);
            out[address[9]] = static_cast<int8_t>(llr_bits_2_3[odd_2]);
            out[address[10]] = static_cast<int8_t>(llr_bits_4_5[even_2]);
            out[address[11]] = static_cast<int8_t>(llr_bits_4_5[odd_2]);

            n += 4;
            address += 12;
        }
        idx_out += 24;
        if(idx_out == fec_size) {
            idx_out = 0;
            address = address_begin;
            out += fec_size;
            idx_plp_simd[blocks] = plp_id;
            ++blocks;
            if(blocks == SIZEOF_SIMD) {
                blocks = 0;
                int len_out = fec_size * SIZEOF_SIMD;
                mutex_out->lock();
                if(swap_buffer) {
                    swap_buffer = false;
                    emit soft_multiplexer_de_twist(idx_plp_simd, l1_post, len_out, buffer_a);
                    out = buffer_b;
                }
                else {
                    swap_buffer = true;
                    emit soft_multiplexer_de_twist(idx_plp_simd, l1_post, len_out, buffer_b);
                    out = buffer_a;
                }
                signal_out->wait(mutex_out);
                mutex_out->unlock();
            }
        }
    }
}
//------------------------------------------------------------------------------------------
void llr_demapper::qam256(int _plp_id, l1_postsignalling _l1_post, int _len_in, complex* _in)
{
    int plp_id = _plp_id;
    int idx_plp_simd[SIZEOF_SIMD];
    l1_postsignalling l1_post = _l1_post;
    int len_in = _len_in;
    bool derotate = false;
    if(l1_post.plp[plp_id].plp_rotation != 0) derotate = true;
    dvbt2_fectype_t fec_type = static_cast<dvbt2_fectype_t>(l1_post.plp[plp_id].plp_fec_type);
    dvbt2_code_rate_t code_rate = static_cast<dvbt2_code_rate_t>(l1_post.plp[plp_id].plp_cod);
    int fec_size;
    int idx_out = 0;
    static int blocks = 0;
    static int8_t* out;
    if(blocks == 0) {
        if(swap_buffer) out = buffer_a;
        else out = buffer_b;
    }
    if(derotate) {
        for(int i = 0; i < len_in; ++i) _in[i] *=  derotate_qam256;
    }
    //hard demap
    float sum_s = 0;
    float sum_e = 0;
    float snr;
    //hard demap
    complex in;
    for(int i = 0; i < len_in; ++i) {
        complex s, e;
        in = _in[i];
        if(in.real() > 0){
            if(in.real() > norm_256_x8){
                if(in.real() > norm_256_x12){
                    if(in.real() > norm_256_x14) s.real(norm_256_x15);
                    else s.real(norm_256_x13);
                }
                else{
                    if(in.real() > norm_256_x10) s.real(norm_256_x11);
                    else s.real(norm_256_x9);
                }
            }
            else{
                if(in.real() > norm_256_x4){
                    if(in.real() > norm_256_x6) s.real(norm_256_x7);
                    else s.real(norm_256_x5);
                }
                else{
                    if(in.real() > norm_256_x2) s.real(norm_256_x3);
                    else s.real(norm_256_x1);
                }
            }
        }
        else {
            if(in.real() < -norm_256_x8){
                if(in.real() < -norm_256_x12){
                    if(in.real() < -norm_256_x14) s.real(-norm_256_x15);
                    else s.real(-norm_256_x13);
                }
                else{
                    if(in.real() < -norm_256_x10) s.real(-norm_256_x11);
                    else s.real(-norm_256_x9);
                }
            }
            else{
                if(in.real() < -norm_256_x4){
                    if(in.real() < -norm_256_x6) s.real(-norm_256_x7);
                    else s.real(-norm_256_x5);
                }
                else{
                    if(in.real() < -norm_256_x2) s.real(-norm_256_x3);
                    else s.real(-norm_256_x1);
                }
            }
        }
        if(in.imag() > 0) {
            if(in.imag() > norm_256_x8){
                if(in.imag() > norm_256_x12){
                    if(in.imag() > norm_256_x14) s.imag(norm_256_x15);
                    else s.imag(norm_256_x13);
                }
                else{
                    if(in.imag() > norm_256_x10) s.imag(norm_256_x11);
                    else s.imag(norm_256_x9);
                }
            }
            else{
                if(in.imag() > norm_256_x4){
                    if(in.imag() > norm_256_x6) s.imag(norm_256_x7);
                    else s.imag(norm_256_x5);
                }
                else{
                    if(in.imag() > norm_256_x2) s.imag(norm_256_x3);
                    else s.imag(norm_256_x1);
                }
            }
        }
        else {
            if(in.imag() < -norm_256_x8){
                if(in.imag() < -norm_256_x12){
                    if(in.imag() < -norm_256_x14) s.imag(-norm_256_x15);
                    else s.imag(-norm_256_x13);
                }
                else{
                    if(in.imag() < -norm_256_x10) s.imag(-norm_256_x11);
                    else s.imag(-norm_256_x9);
                }
            }
            else{
                if(in.imag() < -norm_256_x4){
                    if(in.imag() < -norm_256_x6) s.imag(-norm_256_x7);
                    else s.imag(-norm_256_x5);
                }
                else{
                    if(in.imag() < -norm_256_x2) s.imag(-norm_256_x3);
                    else s.imag(-norm_256_x1);
                }
            }
        }
        e = in - s;
        sum_s += std::norm(s);
        sum_e += std::norm(e);
    }
    snr = 20.0f * std::log10(sum_s / sum_e);
    emit signal_noise_ratio(snr);
    //soft demap
    int* address;
    int* address_begin = nullptr;
    float precision;
    float ALIGNED_(32) llr_bits_0_1[8];
    float ALIGNED_(32) llr_bits_2_3[8];
    float ALIGNED_(32) llr_bits_4_5[8];
    float ALIGNED_(32) llr_bits_6_7[8];
    __m256 v_precision, v_norm_x8, v_norm_x4, v_norm_x2, signbits;
    __m256 v_in, v_mul, v_sub, v_abs_in, v_abs_in_sub_x, v_round;
    precision = 8.0f * NORM_FACTOR_QAM256 * sum_s / sum_e;
    v_precision = _mm256_set1_ps(precision);
    v_norm_x8 = _mm256_set1_ps(norm_256_x8);
    v_norm_x4 = _mm256_set1_ps(norm_256_x4);
    v_norm_x2 = _mm256_set1_ps(norm_256_x2);
    signbits = _mm256_set1_ps(-0.0f);
    if(fec_type == FEC_FRAME_NORMAL) {
        fec_size = FEC_SIZE_NORMAL;
        if(code_rate == C3_5) address = address_qam256_fecnormal_3_5;
        else if (code_rate == C2_3) address = address_qam256_fecnormal_2_3;
        else address = address_qam256_fecnormal;
    }
    else{
        fec_size = FEC_SIZE_SHORT;
        address = address_qam256_fecshort;
    }
    address_begin = address;

    for(int i = 0; i < len_in; i += 4) {

        float* in = reinterpret_cast<float*>(_in + i);
        v_in = _mm256_load_ps(in);
        v_mul = _mm256_mul_ps(v_in, v_precision);
        v_round = _mm256_round_ps(v_mul ,_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
        _mm256_store_ps(llr_bits_0_1, v_round);

        v_abs_in = _mm256_andnot_ps(signbits, v_in);
        v_sub = _mm256_sub_ps(v_abs_in, v_norm_x8);
        v_mul = _mm256_mul_ps(v_sub, v_precision);
        v_round = _mm256_round_ps(v_mul ,_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
        _mm256_store_ps(llr_bits_2_3, v_round);

        v_abs_in_sub_x = _mm256_andnot_ps(signbits, v_sub);
        v_sub = _mm256_sub_ps(v_abs_in_sub_x, v_norm_x4);
        v_mul = _mm256_mul_ps(v_sub, v_precision);
        v_round = _mm256_round_ps(v_mul ,_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
        _mm256_store_ps(llr_bits_4_5, v_round);

        v_abs_in_sub_x = _mm256_andnot_ps(signbits, v_sub);
        v_sub = _mm256_sub_ps(v_abs_in_sub_x, v_norm_x2);
        v_mul = _mm256_mul_ps(v_sub, v_precision);
        v_round = _mm256_round_ps(v_mul ,_MM_FROUND_TO_NEAREST_INT |_MM_FROUND_NO_EXC);
        _mm256_store_ps(llr_bits_6_7, v_round);

        int n = 0;
        for(int i = 0; i < 2; ++i){
            int even_1 = n;
            int odd_1  = n + 1;
            int even_2 = n + 2;
            int odd_2  = n + 3;

            out[address[0]] = static_cast<int8_t>(llr_bits_0_1[even_1]);
            out[address[1]] = static_cast<int8_t>(llr_bits_0_1[odd_1]);
            out[address[2]] = static_cast<int8_t>(llr_bits_2_3[even_1]);
            out[address[3]] = static_cast<int8_t>(llr_bits_2_3[odd_1]);
            out[address[4]] = static_cast<int8_t>(llr_bits_4_5[even_1]);
            out[address[5]] = static_cast<int8_t>(llr_bits_4_5[odd_1]);
            out[address[6]] = static_cast<int8_t>(llr_bits_6_7[even_1]);
            out[address[7]] = static_cast<int8_t>(llr_bits_6_7[odd_1]);
            out[address[8]] = static_cast<int8_t>(llr_bits_0_1[even_2]);
            out[address[9]] = static_cast<int8_t>(llr_bits_0_1[odd_2]);
            out[address[10]] = static_cast<int8_t>(llr_bits_2_3[even_2]);
            out[address[11]] = static_cast<int8_t>(llr_bits_2_3[odd_2]);
            out[address[12]] = static_cast<int8_t>(llr_bits_4_5[even_2]);
            out[address[13]] = static_cast<int8_t>(llr_bits_4_5[odd_2]);
            out[address[14]] = static_cast<int8_t>(llr_bits_6_7[even_2]);
            out[address[15]] = static_cast<int8_t>(llr_bits_6_7[odd_2]);

            n += 4;
            address += 16;
        }
        idx_out += 32;
        if(idx_out == fec_size){
            idx_out = 0;
            address = address_begin;
            out += fec_size;
            idx_plp_simd[blocks] = plp_id;
            ++blocks;
            if(blocks == SIZEOF_SIMD){
                blocks = 0;
                int len_out = fec_size * SIZEOF_SIMD;
                mutex_out->lock();
                if(swap_buffer) {
                    swap_buffer = false;
                    emit soft_multiplexer_de_twist(idx_plp_simd, l1_post, len_out, buffer_a);
                    out = buffer_b;
                }
                else {
                    swap_buffer = true;
                    emit soft_multiplexer_de_twist(idx_plp_simd, l1_post, len_out, buffer_b);
                    out = buffer_a;
                }
                signal_out->wait(mutex_out);
                mutex_out->unlock();
            }
        }
    }
}
//------------------------------------------------------------------------------------------
inline int8_t llr_demapper::quantize(float &_precision, float _in)
{
    int8_t out;
    float b_int = std::nearbyint(_in * _precision);
    out = static_cast<int8_t>(std::min<float>(std::max<float>(b_int, -128), 127));
    return out;
}
//------------------------------------------------------------------------------------------
void llr_demapper::stop()
{
    emit finished();
}
//------------------------------------------------------------------------------------------
