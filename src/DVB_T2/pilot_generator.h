/*
 *  Copyright 2014 Ron Economos.
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
#ifndef PILOT_GENERATOR_H
#define PILOT_GENERATOR_H

#include <QObject>

#include "dvbt2_definition.h"

class pilot_generator
{
public:
    pilot_generator();
    ~pilot_generator();

    int* p2_carrier_map = nullptr;
    int** data_carrier_map = nullptr;
    int* fc_carrier_map = nullptr;
    float** p2_pilot_refer = nullptr;
    float** data_pilot_refer = nullptr;
    float* fc_pilot_refer = nullptr;
    void p2_generator(dvbt2_parameters _dvbt2);
    void data_generator(dvbt2_parameters _dvbt2);

private:
    dvbt2_parameters dvbt2;
    int* data_carrier_map_temp = nullptr;
    int n_data;
    float p2_bpsk[2];
    float sp_bpsk[2];
    float cp_bpsk[2];
    float p2_bpsk_inverted[2];
    float sp_bpsk_inverted[2];
    float cp_bpsk_inverted[2];
    int dx;
    int dy;
    int* prbs = nullptr;
    int pn_sequence[CHIPS];
    void init_prbs(dvbt2_parameters _dvbt2);
    void p2_carrier_mapping();
    void p2_pilot_amplitudes();
    void cp_amplitudes();
    void sp_amplitudes();
    void p_amplitudes();
    void data_carries_mapping();
    void cp_mappinng();
    void sp_mappinng(const int &_idx_symbol);
    void tr_papr_carriers_mapping(const int &_idx_symbol);
    void fc_carrier_mapping();
    void p2_modulation();
    void modulation();
};

#endif // PILOT_GENERATOR_H
