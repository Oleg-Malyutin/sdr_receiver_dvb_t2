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
#ifndef DATA_SYMBOL_H
#define DATA_SYMBOL_H

#include <QObject>

#include "dvbt2_definition.h"
#include "pilot_generator.h"
#include "address_freq_deinterleaver.h"

class data_symbol : public QObject
{
    Q_OBJECT
public:
    explicit data_symbol(QObject* parent = nullptr);
    ~data_symbol();

    complex *execute(int _idx_symbol, complex* _ofdm_cell,
                     float &_sample_rate_offset, float &_phase_offset);
    void init(dvbt2_parameters &_dvbt2, pilot_generator* _pilot,
              address_freq_deinterleaver* _address);

signals:
    void replace_spectrograph(const int _len_data, complex* _data);
    void replace_constelation(const int _len_data, complex* _data);
    void replace_oscilloscope(const int _len_data, complex* _data);

private:
    pilot_generator* pilot;
    address_freq_deinterleaver* address;
    int n_data;
    int fft_size;
    int half_fft_size;
    int c_data;
    int k_total;
    int half_total;
    int n_p2;
    int left_nulls;
    int** data_carrier_map = nullptr;
    float** data_pilot_refer = nullptr;
    float amp_cp;
    float amp_sp;
    float cor_amp_cp;
    int* h_even_data;
    int* h_odd_data;
    complex* buffer_cell;        // max(dx) x max(dy) x 2 for scaterred pilot pattern
    complex* deinterleaved_buffer_a;
    complex* deinterleaved_buffer_b;
    bool swap_buffer = false;
    complex* est_show = nullptr;
    complex* show_symbol = nullptr;
    complex* show_data = nullptr;
    complex* show_est_data = nullptr;
};

#endif // DATA_SYMBOL_H
