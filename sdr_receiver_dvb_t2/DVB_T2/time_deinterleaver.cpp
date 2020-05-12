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
#include "time_deinterleaver.h"

#include <immintrin.h>

//-----------------------------------------------------------------------------------------------
time_deinterleaver::time_deinterleaver(QWaitCondition* _signal_in, QMutex *_mutex, QObject *parent) :
    QObject(parent),
    signal_in(_signal_in),
    mutex_in(_mutex)
{
    mutex_out = new QMutex;
    signal_out = new QWaitCondition;
    qam = new llr_demapper(signal_out, mutex_out);
    thread = new QThread;
    qam->moveToThread(thread);
    connect(this, &time_deinterleaver::ti_block, qam, &llr_demapper::execute);
    connect(this, &time_deinterleaver::stop_qam, qam, &llr_demapper::stop);
    connect(qam, &llr_demapper::finished, qam, &llr_demapper::deleteLater);
    connect(qam, &llr_demapper::finished, thread, &QThread::quit, Qt::DirectConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}
//-----------------------------------------------------------------------------------------------
void time_deinterleaver::start(dvbt2_parameters _dvbt2, l1_presignalling _l1_pre, l1_postsignalling _l1_post)

{
    dvbt2 = _dvbt2;
    l1_pre = _l1_pre;
    l1_post = _l1_post;
    p2_start_idx_cell = L1_PRE_CELL + l1_pre.l1_post_size;
    num_plp = l1_post.num_plp;
    fec_len_bits = new int[num_plp];
    bits_per_cell = new int[num_plp];
    n_ti = new int[num_plp];
    fec_blocks_per_time_interleving = new int* [num_plp];
    p_i = new int[num_plp];
    frame_interval = new int[num_plp];
    first_frame_idx = new int[num_plp];
    cells_per_fec_block = new int[num_plp];
    slice_end = new int[num_plp];
    num_rows = new int[num_plp];
    num_cols = new int* [num_plp];
    permutations = new int* [num_plp];
    last_frame_idx = new int[num_plp];
    int len_max = 0;
    for(int i = 0; i < num_plp; ++i){
        switch (static_cast<dvbt2_fectype_t>(l1_post.plp[i].plp_fec_type)) {
        case FECFRAME_SHORT:
            fec_len_bits[i] = FEC_SIZE_SHORT;
            switch (l1_post.plp[i].plp_mod) {
            case 0:
                bits_per_cell[i] = 2;
                cells_per_fec_block[i] = 8100;      // cell_per_fec_block[i] = fec_len_bits[i] / bits_per_cell[i]
                num_rows[i] = 1620;                 // num_rows[i] = cell_per_fec_block[i] / n_split (5)
                break;
            case 1:
                bits_per_cell[i] = 4;
                cells_per_fec_block[i] = 4050;
                num_rows[i] = 810;
                break;
            case 2:
                bits_per_cell[i] = 6;
                cells_per_fec_block[i] = 2700;
                num_rows[i] = 540;
                break;
            case 3:
                bits_per_cell[i] = 8;
                cells_per_fec_block[i] = 2025;
                num_rows[i] = 405;
                break;
            default:
                break;
            }
            break;
        case FEC_FRAME_NORMAL:
            fec_len_bits[i] = FEC_SIZE_NORMAL;
            switch (l1_post.plp[i].plp_mod) {
            case 0:
                bits_per_cell[i] = 2;
                cells_per_fec_block[i] = 32400;
                num_rows[i] = 6480;
                break;
            case 1:
                bits_per_cell[i] = 4;
                cells_per_fec_block[i] = 16200;
                num_rows[i] = 3240;
                break;
            case 2:
                bits_per_cell[i] = 6;
                cells_per_fec_block[i] = 10800;
                num_rows[i] = 2160;
                break;
            case 3:
                bits_per_cell[i] = 8;
                cells_per_fec_block[i] = 8100;
                num_rows[i] = 1620;
                break;
            default:
                break;
            }
            break;
        }
        switch (l1_post.plp[i].time_il_type) {
        case 0:
            n_ti[i] = l1_post.plp[i].time_il_length;
            p_i[i] = 1;
            break;
        case 1:
            n_ti[i] = 1;
            p_i[i] = l1_post.plp[i].time_il_length;
            break;
        default:
            break;
        }
        fec_blocks_per_time_interleving[i] = new int [n_ti[i]];
        num_cols[i] = new int [n_ti[i]];
        int len_buffer = l1_post.plp[i].plp_num_blocks_max * cells_per_fec_block[i];
        permutations[i] = new int [len_buffer];
        address_cell_deinterleaving(l1_post.plp[i].plp_num_blocks_max, cells_per_fec_block[i], permutations[i]);
        frame_interval[i] = l1_post.plp[i].frame_interval;
        first_frame_idx[i] = l1_post.plp[i].first_frame_idx;
        last_frame_idx[i] = first_frame_idx[i] + (p_i[i] - 1) * frame_interval[i];
        if(len_max < len_buffer) len_max = len_buffer;
    }

    buffer_a = static_cast<complex *>(_mm_malloc(sizeof(complex) * static_cast<unsigned int>(len_max), 32));
    buffer_b = static_cast<complex *>(_mm_malloc(sizeof(complex) * static_cast<unsigned int>(len_max), 32));

    show_data = new complex[len_max];
    flag_start = true;
}
//-----------------------------------------------------------------------------------------------
time_deinterleaver::~time_deinterleaver()
{
    emit stop_qam();
    if(thread->isRunning()) thread->wait(1000);
    if(flag_start){
        delete [] fec_len_bits;
        delete [] bits_per_cell;
        delete [] n_ti;
        delete [] p_i;
        for (int i = 0; i < num_plp; ++i) delete[] fec_blocks_per_time_interleving[i];
        delete [] fec_blocks_per_time_interleving;
        delete [] frame_interval;
        delete [] first_frame_idx;
        delete [] cells_per_fec_block;
        delete [] slice_end;
        delete [] num_rows;
        for (int i = 0; i < num_plp; ++i) delete[] num_cols[i];
        delete [] num_cols;
        for (int i = 0; i < num_plp; ++i) delete [] permutations[i];
        delete [] permutations;
        delete [] last_frame_idx;
        _mm_free(buffer_a);
        _mm_free(buffer_b);
        delete [] show_data;
    }
}
//-----------------------------------------------------------------------------------------------
void time_deinterleaver::address_cell_deinterleaving(int _num_fec_block_max, int _cells_per_fec_block,
                                                     int* _permutations)
{
    int block_max = _num_fec_block_max;
    int cells_size = _cells_per_fec_block;
    int pn_degree = static_cast<int>(ceil(log2(cells_size)));
    int max_states = static_cast<int>(pow(2, pn_degree));
    int lfsr = 0;
    int logic11[2] = {0, 3};
    int logic12[2] = {0, 2};
    int logic13[4] = {0, 1, 4, 6};
    int logic14[6] = {0, 1, 4, 5, 9, 11};
    int logic15[4] = {0, 1, 2, 12};
    int *logic;
    int xor_size, pn_mask, result;
    int q = 0;
    int n = 0;
    int shift, temp;
    int index = 0;
    int* first_permutation = new int[max_states];
    int address = 0;
    switch (pn_degree) {
    case 11:
        logic = &logic11[0];
        xor_size = 2;
        pn_mask = 0x3ff;
        break;
    case 12:
        logic = &logic12[0];
        xor_size = 2;
        pn_mask = 0x7ff;
        break;
    case 13:
        logic = &logic13[0];
        xor_size = 4;
        pn_mask = 0xfff;
        break;
    case 14:
        logic = &logic14[0];
        xor_size = 6;
        pn_mask = 0x1fff;
        break;
    case 15:
        logic = &logic15[0];
        xor_size = 4;
        pn_mask = 0x3fff;
        break;
    default:
        logic = &logic14[0];
        xor_size = 6;
        pn_mask = 0x1fff;
        break;
    }
    for (int i = 0; i < max_states; ++i) {
        if (i == 0 || i == 1) {
            lfsr = 0;
        }
        else if (i == 2) {
            lfsr = 1;
        }
        else {
            result = 0;
            for (int k = 0; k < xor_size; ++k) {
                result ^= (lfsr >> logic[k]) & 1;
            }
            lfsr &= pn_mask;
            lfsr >>= 1;
            lfsr |= result << (pn_degree - 2);
        }
        lfsr |= (i % 2) << (pn_degree - 1);
        if (lfsr < cells_size) {
            first_permutation[q++] = lfsr;
        }
    }
    for (int r = 0; r < block_max; r++) {
        shift = cells_size;
        while (shift >= cells_size) {
            temp = n;
            shift = 0;
            for (int p = 0; p < pn_degree; ++p) {
                shift |= temp & 1;
                shift <<= 1;
                temp >>= 1;
            }
            n++;
        }
        for (int w = 0; w < cells_size; ++w) {
            _permutations[((first_permutation[w]  + shift) % cells_size) + index] = address++;
        }
        index += cells_size;
    }
    delete [] first_permutation;
}
//-----------------------------------------------------------------------------------------------
void time_deinterleaver::l1_dyn_init(l1_postsignalling _l1_post, int _len_in, complex* _ofdm_cell)
{
    // dynamic l1 post signaling
    l1_post = _l1_post;
    for(int i = 0; i < num_plp; ++i){
        slice_end[i] = l1_post.dyn.plp[i].start + l1_post.dyn.plp[i].num_blocks *
                        cells_per_fec_block[1] / p_i[i] - 1;
        int fec_blocks_per_ti_block = static_cast<int>(floorf(static_cast<float>(l1_post.dyn.plp[i].num_blocks) /
                                             static_cast<float>(n_ti[i]) * static_cast<float>(p_i[i])));
        for(int j = 0; j < l1_post.plp[i].time_il_length; ++j){
            int f = fec_blocks_per_ti_block;
            if(j >= (n_ti[i]  - l1_post.dyn.plp[i].num_blocks % n_ti[i])) f += 1;
            fec_blocks_per_time_interleving[i][j] = f;
            num_cols[i][j] = f * n_split;
        }
    }
    start_t2_frame = true;
    execute(_len_in, _ofdm_cell);
}
//-----------------------------------------------------------------------------------------------
void time_deinterleaver::execute(int _len_in, complex* _ofdm_cell)
{
    mutex_in->lock();
    signal_in->wakeOne();
    int num_cells = _len_in;
    complex* ofdm_cell = &_ofdm_cell[0];
    if(start_t2_frame == true) {
        start_t2_frame = false;
        idx_cell = 0;
        num_cells = _len_in - p2_start_idx_cell;
        ofdm_cell = &_ofdm_cell[p2_start_idx_cell];
        for (int i = 0; i < num_plp; ++i) {
            if(l1_post.dyn.plp[i].start == 0) plp_id = i;
        }
        num_rows_plp = num_rows[plp_id];
        cells_per_fec_block_plp = cells_per_fec_block[plp_id];
        idx_time_il = 0;
        ti_block_size = num_cols[plp_id][idx_time_il] * num_rows_plp;
        cell_deint = &permutations[plp_id][0];
        idx_step_ti = 0;
        idx_row_ti = 0;
        if(swap_buffers) time_deint_cell = buffer_a;
        else time_deint_cell = buffer_b;
    }
    for (int i = 0; i < num_cells; ++i) {
        int d = idx_step_ti + idx_row_ti;
        int i_address = cell_deint[d];
        int q_address = i_address - 1;
        if(i_address % cells_per_fec_block_plp == 0) {
            if(i_address != 0) time_deint_cell[end_cell_fec_block].imag(q_first_cell_fec_block);
            q_first_cell_fec_block = ofdm_cell->imag();
            end_cell_fec_block = q_address + cells_per_fec_block_plp;
        }
        else {
            time_deint_cell[q_address].imag(ofdm_cell->imag());
        }
        time_deint_cell[i_address].real(ofdm_cell->real());
        ++ofdm_cell;
        idx_step_ti += num_rows_plp;

        if(idx_step_ti == ti_block_size) {
            time_deint_cell[end_cell_fec_block].imag(q_first_cell_fec_block);
            idx_step_ti = 0;
            if(++idx_row_ti == num_rows_plp) {
                idx_row_ti = 0;
                if(idx_show_plp == plp_id) {
                    int len = cells_per_fec_block_plp;
                    memcpy(show_data, &time_deint_cell[0], sizeof(complex) * static_cast<unsigned long>(len));
                    emit replace_constelation(len, show_data);
                }
                mutex_out->lock();
                emit ti_block(ti_block_size, time_deint_cell, plp_id, l1_post);
                signal_out->wait(mutex_out);
                mutex_out->unlock();
                if(swap_buffers) {
                    swap_buffers = false;
                    time_deint_cell = buffer_b;
                }
                else {
                    swap_buffers = true;
                    time_deint_cell = buffer_a;
                }
                if(++idx_time_il == l1_post.plp[plp_id].time_il_length) {
                    idx_time_il = 0;
                    if(idx_cell == slice_end[plp_id]) {
                        for (int i = 0; i < num_plp; ++i) {
                            if(idx_cell == l1_post.dyn.plp[i].start - 1) {
                                plp_id = l1_post.dyn.plp[i].id;
                                num_rows_plp = num_rows[plp_id];
                                ti_block_size = num_cols[plp_id][idx_time_il] * num_rows_plp;
                                cell_deint = &permutations[plp_id][0];
                                cells_per_fec_block_plp = cells_per_fec_block[plp_id];
                            }
                        }
                    }
                }
                else {
                    ti_block_size = num_cols[plp_id][idx_time_il] * num_rows_plp;
                }
            }
        }
        ++idx_cell;
    }
    mutex_in->unlock();
}
//-----------------------------------------------------------------------------------------------
void time_deinterleaver::stop()
{
    emit finished();
}
//-----------------------------------------------------------------------------------------------
