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
#ifndef TIME_DEINTERLEAVER_H
#define TIME_DEINTERLEAVER_H

#include <QObject>
#include <QThread>
#include <QMutex>

#include "DSP/fast_fourier_transform.h"
#include "dvbt2_definition.h"
#include "llr_demapper.h"

class time_deinterleaver : public QObject
{
    Q_OBJECT
public:
    explicit time_deinterleaver(QWaitCondition* _signal_in, QMutex* _mutex, QObject *parent = nullptr);
    ~time_deinterleaver();

    void start(dvbt2_parameters _dvbt2, l1_presignalling _l1_pre, l1_postsignalling _l1_post);
    llr_demapper* qam;
    volatile int idx_show_plp = 0;

signals:
    void ti_block(int _ti_block_size, complex* _time_deint_cell, int _plp_id, l1_postsignalling _l1_post);
    void replace_constelation(const int _len_data, complex* _data);
    void stop_qam();
    void finished();

public slots:
    void l1_dyn_init(l1_postsignalling _l1_post, int _len_in, complex* _ofdm_cell);
    void execute(int _len_in, complex* _ofdm_cell);
    void stop();

private:
    QWaitCondition* signal_in;
    QThread* thread;
    QWaitCondition* signal_out;
    QMutex* mutex_in;
    QMutex* mutex_out;

    dvbt2_parameters dvbt2;
    l1_presignalling l1_pre;
    l1_postsignalling l1_post;
    bool flag_start = false;
    int p2_start_idx_cell;
    int num_plp;                            // PLP to decode in the receiver
    const int n_split = 5;                  // number of columns occupied by each FEC block.
    int* fec_len_bits;                      // FEC block size (16200 or 64800)
    int* bits_per_cell;                     // constellation order
    int* n_ti;                              // number of TI blocks per Interleaving Frame
    int* p_i;                               // number of T2 Frame per Interleaving Frame
    int* frame_interval;                    // jump to next T2 Frame for Interleaving Frame
    int* first_frame_idx;                   // index of the first T2 Frame of the super-frame for current PLP
    int* cells_per_fec_block;               // number of cell per FEC block
    int* slice_end;                         // the address of the last cell occupied by a common or Type 1 PLP
    int* num_rows;                          // number of rows per TI block
    int** num_cols;                         // number of colums per TI block
    int** permutations;                     // address cell deinterleaving
    int* last_frame_idx;                    // last T2 frame of first interleaving frame

    int num_fec_fblock;                     // Total number of FEC blocks to decode
    int fec_blocks_per_interleaving_frame;  // FEC blocks in each Interleaving Frame
    int** fec_blocks_per_time_interleving;  // FEC block in each time interleaving

    int frame_idx;
    int sub_slice_interval;
    bool start_t2_frame = true;
    complex* buffer_a = nullptr;
    complex* buffer_b = nullptr;
    int* cell_deint = nullptr;
    complex* time_deint_cell;
    bool start_swap_buffers = true;
    bool swap_buffers = true;

    int idx_cell = 0;
    int plp_id = 0;
    int idx_time_il = 0;
    int num_rows_plp = 0;
    int idx_row_ti = 0;
    int cells_per_fec_block_plp;
    int end_cell_fec_block;
    float q_first_cell_fec_block;
    int ti_block_size = 0;
    int idx_step_ti = 0;
    int idx_time_deint_cell = 0;
    void address_cell_deinterleaving(int _num_fec_block_max, int _cell_per_fec_block,
                                     int *_permutations);
    complex* show_data;
};

#endif // TIME_DEINTERLEAVER_H
