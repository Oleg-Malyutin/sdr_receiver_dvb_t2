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
#ifndef LDPC_DECODER_H
#define LDPC_DECODER_H

#include <QObject>
#include <QThread>
#include <QWaitCondition>
#include <QMutex>

#include "dvbt2_definition.h"
#include "bch_decoder.h"
#include "LDPC/dvb_t2_tables.hh"
#include "LDPC/algorithms.hh"

#ifdef __AVX2__
const int SIZEOF_SIMD = 32;
#else
const int SIZEOF_SIMD = 16;
#endif

typedef float value_type;
//typedef std::complex<value_type> complex_type;

#if 1
typedef int8_t code_type;
const int FACTOR = 2;
#else
typedef float code_type;
const int FACTOR = 1;
#endif

#if 0
const int SIMD_WIDTH = 1;
typedef code_type simd_type;
#else
const int SIMD_WIDTH = SIZEOF_SIMD / sizeof(code_type);
typedef SIMD<code_type, SIMD_WIDTH> simd_type;
#endif

#if 0
#include "LDPC/flooding_decoder.hh"
typedef SelfCorrectedUpdate<simd_type> update_type;
typedef MinSumAlgorithm<simd_type, update_type> algorithm_type;
const int TRIALS = 1;//
#else
#include "LDPC/layered_decoder.hh"
typedef NormalUpdate<simd_type> update_type;
typedef OffsetMinSumAlgorithm<simd_type, update_type, FACTOR> algorithm_type;
const int TRIALS = 25;//25
#endif

//typedef NormalUpdate<simd_type> update_type;
//typedef SelfCorrectedUpdate<simd_type> update_type;

//typedef MinSumAlgorithm<simd_type, update_type> algorithm_type;
//typedef OffsetMinSumAlgorithm<simd_type, update_type, FACTOR> algorithm_type;
//typedef MinSumCAlgorithm<simd_type, update_type, FACTOR> algorithm_type;
//typedef LogDomainSPA<simd_type, update_type> algorithm_type;
//typedef LambdaMinAlgorithm<simd_type, update_type, 3> algorithm_type;
//typedef SumProductAlgorithm<simd_type, update_type> algorithm_type;

class ldpc_decoder : public QObject
{
    Q_OBJECT
public:
    explicit ldpc_decoder(QWaitCondition* _signal_in, QMutex* _mutex_in, QObject *parent = nullptr);
    ~ldpc_decoder();
    bch_decoder* decoder;

signals:
    void bit_bch(int* _idx_plp_simd, l1_postsignalling _l1_post, int _lenout, uint8_t* out);
    void check(int _lenout, uint8_t* out);
    void stop_decoder();
    void finished();

public slots:
    void execute(int* _idx_plp_simd, l1_postsignalling _l1_post, int _len_in, int8_t *_in);
    void stop();

private:
    QWaitCondition* signal_in;
    QThread* thread;
    QWaitCondition* signal_out;
    QMutex* mutex_in;
    QMutex* mutex_out;
    uint8_t* ldpc_fec;
    uint8_t* bch_fec;
    uint8_t* buffer_a = nullptr;
    uint8_t* buffer_b = nullptr;
    bool swap_buffer = true;

    LDPCInterface* ldpc_fec_normal_cod_1_2;
    LDPCInterface* ldpc_fec_normal_cod_3_4;
    LDPCInterface* ldpc_fec_normal_cod_2_3;
    LDPCInterface* ldpc_fec_normal_cod_3_5;
    LDPCInterface* ldpc_fec_normal_cod_4_5;
    LDPCInterface* ldpc_fec_normal_cod_5_6;

    LDPCInterface* ldpc_fec_short_cod_1_2;
    LDPCInterface* ldpc_fec_short_cod_3_4;
    LDPCInterface* ldpc_fec_short_cod_2_3;
    LDPCInterface* ldpc_fec_short_cod_3_5;
    LDPCInterface* ldpc_fec_short_cod_4_5;
    LDPCInterface* ldpc_fec_short_cod_5_6;

    LDPCDecoder<simd_type, algorithm_type> decode_normal_cod_1_2;
    LDPCDecoder<simd_type, algorithm_type> decode_normal_cod_3_4;
    LDPCDecoder<simd_type, algorithm_type> decode_normal_cod_2_3;
    LDPCDecoder<simd_type, algorithm_type> decode_normal_cod_3_5;
    LDPCDecoder<simd_type, algorithm_type> decode_normal_cod_4_5;
    LDPCDecoder<simd_type, algorithm_type> decode_normal_cod_5_6;

    LDPCDecoder<simd_type, algorithm_type> decode_short_cod_1_2;
    LDPCDecoder<simd_type, algorithm_type> decode_short_cod_3_4;
    LDPCDecoder<simd_type, algorithm_type> decode_short_cod_2_3;
    LDPCDecoder<simd_type, algorithm_type> decode_short_cod_3_5;
    LDPCDecoder<simd_type, algorithm_type> decode_short_cod_4_5;
    LDPCDecoder<simd_type, algorithm_type> decode_short_cod_5_6;

    LDPCDecoder<simd_type, algorithm_type>* p_decode;

    void* aligned_buffer;
    simd_type *simd;

};

#endif // LDPC_DECODER_H
