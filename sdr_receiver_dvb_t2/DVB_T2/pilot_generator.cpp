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
#include "pilot_generator.h"

//----------------------------------------------------------------------------------------------------------------------------
pilot_generator::pilot_generator()
{

}
//----------------------------------------------------------------------------------------------------------------------------
pilot_generator::~pilot_generator()
{
    if(!p2_pilot_refer){
        for(int i = 0; i < dvbt2.n_p2; ++i) delete [] p2_pilot_refer[i];
        delete [] p2_pilot_refer;
    }
    if(!data_pilot_refer){
        for(int i = 0; i < n_data; ++i) delete [] data_pilot_refer[i];
        delete [] data_pilot_refer;
    }
    if(!data_carrier_map){
        for(int i = 0; i < n_data; ++i) delete [] data_carrier_map[i];
        delete [] data_carrier_map;
    }

    if(!prbs) delete [] prbs;
    if(!p2_carrier_map)delete [] p2_carrier_map;
    if(!data_carrier_map_temp)delete [] data_carrier_map_temp;
    if(!fc_carrier_map){
        delete [] fc_carrier_map;
        delete [] fc_pilot_refer;
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::init_prbs(dvbt2_parameters _dvbt2)
{
    if(prbs != nullptr) delete [] prbs;
    int len = _dvbt2.k_total + _dvbt2.k_offset;
    prbs = new int[len];
    int sr = 0x7ff;
    int j = 0;
    for (int i = 0; i < len; ++i) {
        int b = ((sr) ^ (sr >> 2)) & 1;
        prbs[i] = sr & 1;
        sr >>= 1;
        if(b) sr |= 0x400;
    }
    for (int i = 0; i < (CHIPS / 8); ++i) {
        for (int k = 7; k >= 0; --k){
            pn_sequence[j] = (pn_sequence_table[i] >> k) & 0x1;
            j = j + 1;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::p2_generator(dvbt2_parameters _dvbt2)
{
    dvbt2 = _dvbt2;
    init_prbs(dvbt2);
    if(p2_carrier_map !=nullptr) delete [] p2_carrier_map;
    p2_carrier_map = new int[dvbt2.k_total];
    if(p2_pilot_refer != nullptr) {
        for(int i = 0; i < dvbt2.n_p2; ++i) delete [] p2_pilot_refer[i];
        delete [] p2_pilot_refer;
    }
    p2_pilot_refer = new float* [dvbt2.n_p2];
    for(int i = 0; i < dvbt2.n_p2; ++i) p2_pilot_refer[i] = new float[dvbt2.k_total];
    p2_carrier_mapping();
    p2_pilot_amplitudes();
    p2_modulation();
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::data_generator(dvbt2_parameters _dvbt2)
{
    dvbt2 = _dvbt2;

    n_data = dvbt2.n_data;
    if(data_carrier_map != nullptr) {
        for(int i = 0; i < n_data; ++i) delete [] data_carrier_map[i];
        delete [] data_carrier_map;
    }
    data_carrier_map = new int *[n_data];

    if(data_carrier_map_temp != nullptr) {
        delete [] data_carrier_map_temp;
    }
    data_carrier_map_temp = new int[dvbt2.k_total];

    for(int i = 0; i < n_data; ++i) data_carrier_map[i] = new int[dvbt2.k_total];
    if(data_pilot_refer != nullptr) {
        for(int i = 0; i < n_data; ++i) delete [] data_pilot_refer[i];
        delete [] data_pilot_refer;
    }
    data_pilot_refer = new float *[n_data];
    for(int i = 0; i < n_data; ++i) data_pilot_refer[i] = new float[dvbt2.k_total];
    if(fc_carrier_map != nullptr) {
        delete [] fc_carrier_map;
        delete [] fc_pilot_refer;
    }
    fc_carrier_map = new int[dvbt2.k_total];
    fc_pilot_refer = new float[dvbt2.k_total];
    cp_amplitudes();
    sp_amplitudes();
    //TODO:
    int idx_data_symbol = 0;
    for(int idx_symbol = dvbt2.n_p2; idx_symbol < dvbt2.len_frame - dvbt2.l_fc ; ++idx_symbol){
        data_carries_mapping();
        cp_mappinng();
        sp_mappinng(idx_symbol);
        tr_papr_carriers_mapping(idx_symbol);
        for(int i = 0; i < dvbt2.k_total; ++i) {
             data_carrier_map[idx_data_symbol][i] = data_carrier_map_temp[i];
        }
        ++idx_data_symbol;
    }
    //...
    fc_carrier_mapping();
    modulation();
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::p2_carrier_mapping()
{
    int step, ki;
    for (int i = 0; i < dvbt2.k_total; ++i) {
        p2_carrier_map[i] = DATA_CARRIER;
    }
    if ((dvbt2.fft_mode == FFTSIZE_32K || dvbt2.fft_mode == FFTSIZE_32K_T2GI) &&
            (dvbt2.miso == FALSE)) {
        step = 6;
    }
    else {
        step = 3;
    }
    for (int i = 0; i < dvbt2.k_total; i += step) {
        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
            if (((i / 3) % 2) && (i % 3 == 0)) {
                p2_carrier_map[i] = P2CARRIER_INVERTED;
            }
            else {
                p2_carrier_map[i] = P2CARRIER;
            }
        }
        else {
            p2_carrier_map[i] = P2CARRIER;
        }
    }
    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
        for (int i = 0; i < dvbt2.k_ext; ++i) {
            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                if (((i / 3) % 2) && (i % 3 == 0)) {
                    p2_carrier_map[i] = P2CARRIER_INVERTED;
                }
                else {
                    p2_carrier_map[i] = P2CARRIER;
                }
                if ((((i + (dvbt2.k_total - dvbt2.k_ext)) / 3) % 2) &&
                        ((i + (dvbt2.k_total - dvbt2.k_ext)) % 3 == 0)) {
                    p2_carrier_map[i + (dvbt2.k_total - dvbt2.k_ext)] = P2CARRIER_INVERTED;
                }
                else {
                    p2_carrier_map[i + (dvbt2.k_total - dvbt2.k_ext)] = P2CARRIER;
                }
            }
            else {
                p2_carrier_map[i] = P2CARRIER;
                p2_carrier_map[i + (dvbt2.k_total - dvbt2.k_ext)] = P2CARRIER;
            }
        }
    }
    if (dvbt2.miso == TRUE) {
        p2_carrier_map[dvbt2.k_ext + 1] = P2CARRIER;
        p2_carrier_map[dvbt2.k_ext + 2] = P2CARRIER;
        p2_carrier_map[dvbt2.k_total - dvbt2.k_ext - 2] = P2CARRIER;
        p2_carrier_map[dvbt2.k_total - dvbt2.k_ext - 3] = P2CARRIER;
    }
    switch (dvbt2.fft_mode) {
    case FFTSIZE_1K:
        for (int i = 0; i < 10; ++i) {
            p2_carrier_map[p2_papr_map_1k[i]] = P2PAPR_CARRIER;
        }
        if (dvbt2.miso == TRUE) {
            for (int i = 0; i < 10; ++i) {
                ki = p2_papr_map_1k[i] + dvbt2.k_ext;
                if (i < 9) {
                    if (((ki % 3) == 1) && ((ki + 1) != (p2_papr_map_1k[i + 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 1) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                if (i > 0) {
                    if (((ki % 3) == 2) && ((ki - 1) != (p2_papr_map_1k[i - 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 2) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
            }
        }
        break;
    case FFTSIZE_2K:
        for (int i = 0; i < 18; ++i) {
            p2_carrier_map[p2_papr_map_2k[i]] = P2PAPR_CARRIER;
        }
        if (dvbt2.miso == TRUE) {
            for (int i = 0; i < 18; ++i) {
                ki = p2_papr_map_2k[i] + dvbt2.k_ext;
                if (i < 17) {
                    if (((ki % 3) == 1) && ((ki + 1) != (p2_papr_map_2k[i + 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 1) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                if (i > 0) {
                    if (((ki % 3) == 2) && ((ki - 1) != (p2_papr_map_2k[i - 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 2) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
            }
        }
        break;
    case FFTSIZE_4K:
        for (int i = 0; i < 36; ++i) {
            p2_carrier_map[p2_papr_map_4k[i]] = P2PAPR_CARRIER;
        }
        if (dvbt2.miso == TRUE) {
            for (int i = 0; i < 36; ++i) {
                ki = p2_papr_map_4k[i] + dvbt2.k_ext;
                if (i < 35) {
                    if (((ki % 3) == 1) && ((ki + 1) != (p2_papr_map_4k[i + 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 1) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                if (i > 0) {
                    if (((ki % 3) == 2) && ((ki - 1) != (p2_papr_map_4k[i - 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 2) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
            }
        }
        break;
    case FFTSIZE_8K:
    case FFTSIZE_8K_T2GI:
        for (int i = 0; i < 72; ++i) {
            p2_carrier_map[p2_papr_map_8k[i] + dvbt2.k_ext] = P2PAPR_CARRIER;
        }
        if (dvbt2.miso == TRUE) {
            for (int i = 0; i < 72; ++i) {
                ki = p2_papr_map_8k[i] + dvbt2.k_ext;
                if (i < 71) {
                    if (((ki % 3) == 1) && ((ki + 1) != (p2_papr_map_8k[i + 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 1) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                if (i > 0) {
                    if (((ki % 3) == 2) && ((ki - 1) != (p2_papr_map_8k[i - 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 2) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
            }
        }
        break;
    case FFTSIZE_16K:
    case FFTSIZE_16K_T2GI:
        for (int i = 0; i < 144; ++i) {
            p2_carrier_map[p2_papr_map_16k[i] + dvbt2.k_ext] = P2PAPR_CARRIER;
        }
        if (dvbt2.miso == TRUE) {
            for (int i = 0; i < 144; ++i) {
                ki = p2_papr_map_16k[i] + dvbt2.k_ext;
                if (i < 143) {
                    if (((ki % 3) == 1) && ((ki + 1) != (p2_papr_map_16k[i + 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 1) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                if (i > 0) {
                    if (((ki % 3) == 2) && ((ki - 1) != (p2_papr_map_16k[i - 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 2) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
            }
        }
        break;
    case FFTSIZE_32K:
    case FFTSIZE_32K_T2GI:
        for (int i = 0; i < 288; ++i) {
            p2_carrier_map[p2_papr_map_32k[i] + dvbt2.k_ext] = P2PAPR_CARRIER;
        }
        if (dvbt2.miso == TRUE) {
            for (int i = 0; i < 288; ++i) {
                ki = p2_papr_map_32k[i] + dvbt2.k_ext;
                if (i < 287) {
                    if (((ki % 3) == 1) && ((ki + 1) != (p2_papr_map_32k[i + 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 1) {
                        p2_carrier_map[ki + 1] = P2CARRIER;
                    }
                }
                if (i > 0) {
                    if (((ki % 3) == 2) && ((ki - 1) != (p2_papr_map_32k[i - 1] + dvbt2.k_ext))) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
                else {
                    if ((ki % 3) == 2) {
                        p2_carrier_map[ki - 1] = P2CARRIER;
                    }
                }
            }
        }
        break;
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::p2_pilot_amplitudes()
{
    if ((dvbt2.fft_mode == FFTSIZE_32K || dvbt2.fft_mode == FFTSIZE_32K_T2GI) &&
            (dvbt2.miso == FALSE)) {
        p2_bpsk[0] = sqrtf(37.0f) / 5.0f;
        p2_bpsk[1] = -(sqrtf(37.0f) / 5.0f);
        p2_bpsk_inverted[0] = -(sqrtf(37.0f) / 5.0f);
        p2_bpsk_inverted[1] = sqrtf(37.0f) / 5.0f;

    }
    else {
        p2_bpsk[0] = sqrtf(31.0f) / 5.0f;
        p2_bpsk[1] = -(sqrtf(31.0f) / 5.0f);
        p2_bpsk_inverted[0] = -(sqrtf(31.0f) / 5.0f);
        p2_bpsk_inverted[1] = sqrtf(31.0f) / 5.0f;
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::cp_amplitudes()
{
    switch (dvbt2.fft_mode) {
    case FFTSIZE_1K:
        cp_bpsk[0] = 4.0f / 3.0f;
        cp_bpsk[1] = -4.0f / 3.0f;
        cp_bpsk_inverted[0] = -4.0f / 3.0f;
        cp_bpsk_inverted[1] = 4.0f / 3.0f;
        break;
    case FFTSIZE_2K:
        cp_bpsk[0] = 4.0f / 3.0f;
        cp_bpsk[1] = -4.0f / 3.0f;
        cp_bpsk_inverted[0] = -4.0f / 3.0f;
        cp_bpsk_inverted[1] = 4.0f / 3.0f;
        break;
    case FFTSIZE_4K:
        cp_bpsk[0] = (4.0f * sqrtf(2.0f)) / 3.0f;
        cp_bpsk[1] = -(4.0f * sqrtf(2.0f)) / 3.0f;
        cp_bpsk_inverted[0] = -(4.0f * sqrtf(2.0f)) / 3.0f;
        cp_bpsk_inverted[1] = (4.0f * sqrtf(2.0f)) / 3.0f;
        break;
    case FFTSIZE_8K:
    case FFTSIZE_8K_T2GI:
        cp_bpsk[0] = 8.0f / 3.0f;
        cp_bpsk[1] = -8.0f / 3.0f;
        cp_bpsk_inverted[0] = -8.0f / 3.0f;
        cp_bpsk_inverted[1] = 8.0f / 3.0f;
        break;
    case FFTSIZE_16K:
    case FFTSIZE_16K_T2GI:
        cp_bpsk[0] = 8.0f / 3.0f;
        cp_bpsk[1] = -8.0f / 3.0f;
        cp_bpsk_inverted[0] = -8.0f / 3.0f;
        cp_bpsk_inverted[1] = 8.0f / 3.0f;
        break;
    case FFTSIZE_32K:
    case FFTSIZE_32K_T2GI:
        cp_bpsk[0] = 8.0f / 3.0f;
        cp_bpsk[1] = -8.0f / 3.0f;
        cp_bpsk_inverted[0] = -8.0f / 3.0f;
        cp_bpsk_inverted[1] = 8.0f / 3.0f;
        break;
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::sp_amplitudes()
{
    switch (dvbt2.pilot_pattern) {
    case PP1:
        sp_bpsk[0] = 4.0f / 3.0f;
        sp_bpsk[1] = -4.0f / 3.0f;
        sp_bpsk_inverted[0] = -4.0f / 3.0f;
        sp_bpsk_inverted[1] = 4.0f / 3.0f;
        dx = 3;
        dy = 4;
        break;
    case PP2:
        sp_bpsk[0] = 4.0f / 3.0f;
        sp_bpsk[1] = -4.0f / 3.0f;
        sp_bpsk_inverted[0] = -4.0f / 3.0f;
        sp_bpsk_inverted[1] = 4.0f / 3.0f;
        dx = 6;
        dy = 2;
        break;
    case PP3:
        sp_bpsk[0] = 7.0f / 4.0f;
        sp_bpsk[1] = -7.0f / 4.0f;
        sp_bpsk_inverted[0] = -7.0f / 4.0f;
        sp_bpsk_inverted[1] = 7.0f / 4.0f;
        dx = 6;
        dy = 4;
        break;
    case PP4:
        sp_bpsk[0] = 7.0f / 4.0f;
        sp_bpsk[1] = -7.0f / 4.0f;
        sp_bpsk_inverted[0] = -7.0f / 4.0f;
        sp_bpsk_inverted[1] = 7.0f / 4.0f;
        dx = 12;
        dy = 2;
        break;
    case PP5:
        sp_bpsk[0] = 7.0f / 3.0f;
        sp_bpsk[1] = -7.0f / 3.0f;
        sp_bpsk_inverted[0] = -7.0f / 3.0f;
        sp_bpsk_inverted[1] = 7.0f / 3.0f;
        dx = 12;
        dy = 4;
        break;
    case PP6:
        sp_bpsk[0] = 7.0f / 3.0f;
        sp_bpsk[1] = -7.0f / 3.0f;
        sp_bpsk_inverted[0] = -7.0f / 3.0f;
        sp_bpsk_inverted[1] = 7.0f / 3.0f;
        dx = 24;
        dy = 2;
        break;
    case PP7:
        sp_bpsk[0] = 7.0f / 3.0f;
        sp_bpsk[1] = -7.0f / 3.0f;
        sp_bpsk_inverted[0] = -7.0f / 3.0f;
        sp_bpsk_inverted[1] = 7.0f / 3.0f;
        dx = 24;
        dy = 4;
        break;
    case PP8:
        sp_bpsk[0] = 7.0f / 3.0f;
        sp_bpsk[1] = -7.0f / 3.0f;
        sp_bpsk_inverted[0] = -7.0f / 3.0f;
        sp_bpsk_inverted[1] = 7.0f / 3.0f;
        dx = 6;
        dy = 16;
        break;
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::data_carries_mapping()
{
    for (int i = 0; i < dvbt2.k_total; ++i) {
        data_carrier_map_temp[i] = DATA_CARRIER;
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::cp_mappinng()
{
    switch (dvbt2.fft_mode) {
        case FFTSIZE_1K:
            switch (dvbt2.pilot_pattern) {
                case PP1:
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp1_cp1[i] % 1632) / dx)) % 2 && (((pp1_cp1[i] % 1632) % dx) == 0)) {
                                data_carrier_map_temp[pp1_cp1[i] % 1632] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp1_cp1[i] % 1632] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp1_cp1[i] % 1632] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP2:
                    for (int i = 0; i < 20; ++i) {
                        data_carrier_map_temp[pp2_cp1[i] % 1632] = CONTINUAL_CARRIER;
                    }
                    break;
                case PP3:
                    for (int i = 0; i < 22; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp1[i] % 1632) / dx)) % 2 && (((pp3_cp1[i] % 1632) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp1[i] % 1632] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp1[i] % 1632] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp1[i] % 1632] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP4:
                    for (int i = 0; i < 20; ++i) {
                        data_carrier_map_temp[pp4_cp1[i] % 1632] = CONTINUAL_CARRIER;
                    }
                    break;
                case PP5:
                    for (int i = 0; i < 19; ++i) {
                        data_carrier_map_temp[pp5_cp1[i] % 1632] = CONTINUAL_CARRIER;
                    }
                    break;
                case PP6:
                    break;
                case PP7:
                    for (int i = 0; i < 15; ++i) {
                        data_carrier_map_temp[pp7_cp1[i] % 1632] = CONTINUAL_CARRIER;
                    }
                    break;
                case PP8:
                    break;
            }
            break;
        case FFTSIZE_2K:
            switch (dvbt2.pilot_pattern) {
                case PP1:
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp1_cp1[i] % 1632) / dx)) % 2 && (((pp1_cp1[i] % 1632) % dx) == 0)) {
                                data_carrier_map_temp[pp1_cp1[i] % 1632] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp1_cp1[i] % 1632] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp1_cp1[i] % 1632] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 25; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp1_cp2[i] % 1632) / dx)) % 2 && (((pp1_cp2[i] % 1632) % dx) == 0)) {
                                data_carrier_map_temp[pp1_cp2[i] % 1632] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp1_cp2[i] % 1632] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp1_cp2[i] % 1632] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP2:
                    for (int i = 0; i < 20; ++i) {
                        data_carrier_map_temp[pp2_cp1[i] % 1632] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 22; ++i) {
                        data_carrier_map_temp[pp2_cp2[i] % 1632] = CONTINUAL_CARRIER;
                    }
                    break;
                case PP3:
                    for (int i = 0; i < 22; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp1[i] % 1632) / dx)) % 2 && (((pp3_cp1[i] % 1632) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp1[i] % 1632] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp1[i] % 1632] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp1[i] % 1632] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp2[i] % 1632) / dx)) % 2 && (((pp3_cp2[i] % 1632) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp2[i] % 1632] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp2[i] % 1632] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp2[i] % 1632] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP4:
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp1[i] % 1632) / dx)) % 2 && (((pp4_cp1[i] % 1632) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp1[i] % 1632] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp1[i] % 1632] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp1[i] % 1632] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 23; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp2[i] % 1632) / dx)) % 2 && (((pp4_cp2[i] % 1632) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp2[i] % 1632] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp2[i] % 1632] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp2[i] % 1632] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP5:
                    for (int i = 0; i < 19; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp1[i] % 1632) / dx)) % 2 && (((pp5_cp1[i] % 1632) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp1[i] % 1632] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp1[i] % 1632] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp1[i] % 1632] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 23; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp2[i] % 1632) / dx)) % 2 && (((pp5_cp2[i] % 1632) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp2[i] % 1632] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp2[i] % 1632] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp2[i] % 1632] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP6:
                    break;
                case PP7:
                    for (int i = 0; i < 15; ++i) {
                        data_carrier_map_temp[pp7_cp1[i] % 1632] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 30; ++i) {
                        data_carrier_map_temp[pp7_cp2[i] % 1632] = CONTINUAL_CARRIER;
                    }
                    break;
                case PP8:
                    break;
            }
            break;
        case FFTSIZE_4K:
            switch (dvbt2.pilot_pattern) {
                case PP1:
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp1_cp1[i] % 3264) / dx)) % 2 && (((pp1_cp1[i] % 3264) % dx) == 0)) {
                                data_carrier_map_temp[pp1_cp1[i] % 3264] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp1_cp1[i] % 3264] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp1_cp1[i] % 3264] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 25; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp1_cp2[i] % 3264) / dx)) % 2 && (((pp1_cp2[i] % 3264) % dx) == 0)) {
                                data_carrier_map_temp[pp1_cp2[i] % 3264] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp1_cp2[i] % 3264] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp1_cp2[i] % 3264] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP2:
                    for (int i = 0; i < 20; ++i) {
                        data_carrier_map_temp[pp2_cp1[i] % 3264] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 22; ++i) {
                        data_carrier_map_temp[pp2_cp2[i] % 3264] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 2; ++i) {
                        data_carrier_map_temp[pp2_cp3[i] % 3264] = CONTINUAL_CARRIER;
                    }
                    break;
                case PP3:
                    for (int i = 0; i < 22; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp1[i] % 3264) / dx)) % 2 && (((pp3_cp1[i] % 3264) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp1[i] % 3264] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp1[i] % 3264] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp1[i] % 3264] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp2[i] % 3264) / dx)) % 2 && (((pp3_cp2[i] % 3264) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp2[i] % 3264] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp2[i] % 3264] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp2[i] % 3264] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 1; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp3[i] % 3264) / dx)) % 2 && (((pp3_cp3[i] % 3264) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp3[i] % 3264] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp3[i] % 3264] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp3[i] % 3264] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP4:
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp1[i] % 3264) / dx)) % 2 && (((pp4_cp1[i] % 3264) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp1[i] % 3264] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp1[i] % 3264] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp1[i] % 3264] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 23; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp2[i] % 3264) / dx)) % 2 && (((pp4_cp2[i] % 3264) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp2[i] % 3264] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp2[i] % 3264] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp2[i] % 3264] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 1; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp3[i] % 3264) / dx)) % 2 && (((pp4_cp3[i] % 3264) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp3[i] % 3264] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp3[i] % 3264] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp3[i] % 3264] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP5:
                    for (int i = 0; i < 19; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp1[i] % 3264) / dx)) % 2 && (((pp5_cp1[i] % 3264) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp1[i] % 3264] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp1[i] % 3264] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp1[i] % 3264] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 23; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp2[i] % 3264) / dx)) % 2 && (((pp5_cp2[i] % 3264) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp2[i] % 3264] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp2[i] % 3264] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp2[i] % 3264] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 3; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp3[i] % 3264) / dx)) % 2 && (((pp5_cp3[i] % 3264) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp3[i] % 3264] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp3[i] % 3264] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp3[i] % 3264] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP6:
                    break;
                case PP7:
                    for (int i = 0; i < 15; ++i) {
                        data_carrier_map_temp[pp7_cp1[i] % 3264] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 30; ++i) {
                        data_carrier_map_temp[pp7_cp2[i] % 3264] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 5; ++i) {
                        data_carrier_map_temp[pp7_cp3[i] % 3264] = CONTINUAL_CARRIER;
                    }
                    break;
                case PP8:
                    break;
            }
            break;
        case FFTSIZE_8K:
        case FFTSIZE_8K_T2GI:
            switch (dvbt2.pilot_pattern) {
                case PP1:
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp1_cp1[i] % 6528) / dx)) % 2 && (((pp1_cp1[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp1_cp1[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp1_cp1[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp1_cp1[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 25; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp1_cp2[i] % 6528) / dx)) % 2 && (((pp1_cp2[i] % 6528) % dx) == 0)){
                                data_carrier_map_temp[pp1_cp2[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp1_cp2[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp1_cp2[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP2:
                    for (int i = 0; i < 20; ++i) {
                        data_carrier_map_temp[pp2_cp1[i] % 6528] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 22; ++i) {
                        data_carrier_map_temp[pp2_cp2[i] % 6528] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 2; ++i) {
                        data_carrier_map_temp[pp2_cp3[i] % 6528] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 2; ++i) {
                        data_carrier_map_temp[pp2_cp4[i] % 6528] = CONTINUAL_CARRIER;
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 4; ++i) {
                            data_carrier_map_temp[pp2_8k[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP3:
                    for (int i = 0; i < 22; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp1[i] % 6528) / dx)) % 2 && (((pp3_cp1[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp1[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp1[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp1[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp2[i] % 6528) / dx)) % 2 && (((pp3_cp2[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp2[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp2[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp2[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 1; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp3[i] % 6528) / dx)) % 2 && (((pp3_cp3[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp3[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp3[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp3[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 2; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp3_8k[i] / dx)) % 2 && ((pp3_8k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp3_8k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp3_8k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp3_8k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
                case PP4:
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp1[i] % 6528) / dx)) % 2 && (((pp4_cp1[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp1[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp1[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp1[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 23; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp2[i] % 6528) / dx)) % 2 && (((pp4_cp2[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp2[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp2[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp2[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 1; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp3[i] % 6528) / dx)) % 2 && (((pp4_cp3[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp3[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp3[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp3[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 2; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp4[i] % 6528) / dx)) % 2 && (((pp4_cp4[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp4[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp4[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp4[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 2; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp4_8k[i] / dx)) % 2 && ((pp4_8k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp4_8k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp4_8k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp4_8k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
                case PP5:
                    for (int i = 0; i < 19; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp1[i] % 6528) / dx)) % 2 && (((pp5_cp1[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp1[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp1[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp1[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 23; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp2[i] % 6528) / dx)) % 2 && (((pp5_cp2[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp2[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp2[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp2[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 3; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp3[i] % 6528) / dx)) % 2 && (((pp5_cp3[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp3[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp3[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp3[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 1; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp4[i] % 6528) / dx)) % 2 && (((pp5_cp4[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp4[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp4[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp4[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP6:
                    break;
                case PP7:
                    for (int i = 0; i < 15; ++i) {
                        data_carrier_map_temp[pp7_cp1[i] % 6528] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 30; ++i) {
                        data_carrier_map_temp[pp7_cp2[i] % 6528] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 5; ++i) {
                        data_carrier_map_temp[pp7_cp3[i] % 6528] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 3; ++i) {
                        data_carrier_map_temp[pp7_cp4[i] % 6528] = CONTINUAL_CARRIER;
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 5; ++i) {
                            data_carrier_map_temp[pp7_8k[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP8:
                    for (int i = 0; i < 47; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp8_cp4[i] % 6528) / dx)) % 2 && (((pp8_cp4[i] % 6528) % dx) == 0)) {
                                data_carrier_map_temp[pp8_cp4[i] % 6528] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp8_cp4[i] % 6528] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp8_cp4[i] % 6528] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 5; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp8_8k[i] / dx)) % 2 && ((pp8_8k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp8_8k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp8_8k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp8_8k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
            }
            break;
        case FFTSIZE_16K:
        case FFTSIZE_16K_T2GI:
            switch (dvbt2.pilot_pattern) {
                case PP1:
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp1_cp1[i] % 13056) / dx)) % 2 && (((pp1_cp1[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp1_cp1[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp1_cp1[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp1_cp1[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 25; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp1_cp2[i] % 13056) / dx)) % 2 && (((pp1_cp2[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp1_cp2[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp1_cp2[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp1_cp2[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 44; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp1_cp5[i] % 13056) / dx)) % 2 && (((pp1_cp5[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp1_cp5[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp1_cp5[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp1_cp5[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 4; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp1_16k[i] / dx)) % 2 && ((pp1_16k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp1_16k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp1_16k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp1_16k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
                case PP2:
                    for (int i = 0; i < 20; ++i) {
                        data_carrier_map_temp[pp2_cp1[i] % 13056] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 22; ++i) {
                        data_carrier_map_temp[pp2_cp2[i] % 13056] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 2; ++i) {
                        data_carrier_map_temp[pp2_cp3[i] % 13056] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 2; ++i) {
                        data_carrier_map_temp[pp2_cp4[i] % 13056] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 41; ++i) {
                        data_carrier_map_temp[pp2_cp5[i] % 13056] = CONTINUAL_CARRIER;
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 2; ++i) {
                            data_carrier_map_temp[pp2_16k[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP3:
                    for (int i = 0; i < 22; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp1[i] % 13056) / dx)) % 2 && (((pp3_cp1[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp1[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp1[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp1[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp2[i] % 13056) / dx)) % 2 && (((pp3_cp2[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp2[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp2[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp2[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 1; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp3[i] % 13056) / dx)) % 2 && (((pp3_cp3[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp3[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp3[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp3[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 44; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp3_cp5[i] % 13056) / dx)) % 2 && (((pp3_cp5[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp3_cp5[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp3_cp5[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp3_cp5[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 2; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp3_16k[i] / dx)) % 2 && ((pp3_16k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp3_16k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp3_16k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp3_16k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
                case PP4:
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp1[i] % 13056) / dx)) % 2 && (((pp4_cp1[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp1[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp1[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp1[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 23; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp2[i] % 13056) / dx)) % 2 && (((pp4_cp2[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp2[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp2[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp2[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 1; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp3[i] % 13056) / dx)) % 2 && (((pp4_cp3[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp3[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp3[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp3[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 2; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp4[i] % 13056) / dx)) % 2 && (((pp4_cp4[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp4[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp4[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp4[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 44; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp4_cp5[i] % 13056) / dx)) % 2 && (((pp4_cp5[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp5[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp5[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp5[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 2; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp4_16k[i] / dx)) % 2 && ((pp4_16k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp4_16k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp4_16k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp4_16k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
                case PP5:
                    for (int i = 0; i < 19; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp1[i] % 13056) / dx)) % 2 && (((pp5_cp1[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp1[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp1[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp1[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 23; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp2[i] % 13056) / dx)) % 2 && (((pp5_cp2[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp2[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp2[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp2[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 3; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp3[i] % 13056) / dx)) % 2 && (((pp5_cp3[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp3[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp3[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp3[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 1; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp4[i] % 13056) / dx)) % 2 && (((pp5_cp4[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp4[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp4[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp4[i] % 13056] = CONTINUAL_CARRIER;
                       }
                    }
                    for (int i = 0; i < 44; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp5_cp5[i] % 13056) / dx)) % 2 && (((pp5_cp5[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp5_cp5[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp5_cp5[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp5_cp5[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 2; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp5_16k[i] / dx)) % 2 && ((pp5_16k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp5_16k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp5_16k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp5_16k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
                case PP6:
                    for (int i = 0; i < 88; ++i) {
                        data_carrier_map_temp[pp6_cp5[i] % 13056] = CONTINUAL_CARRIER;
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 2; ++i) {
                            data_carrier_map_temp[pp6_16k[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP7:
                    for (int i = 0; i < 15; ++i) {
                        data_carrier_map_temp[pp7_cp1[i] % 13056] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 30; ++i) {
                        data_carrier_map_temp[pp7_cp2[i] % 13056] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 5; ++i) {
                        data_carrier_map_temp[pp7_cp3[i] % 13056] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 3; ++i) {
                        data_carrier_map_temp[pp7_cp4[i] % 13056] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 35; ++i) {
                        data_carrier_map_temp[pp7_cp5[i] % 13056] = CONTINUAL_CARRIER;
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 3; ++i) {
                            data_carrier_map_temp[pp7_16k[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP8:
                    for (int i = 0; i < 47; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp8_cp4[i] % 13056) / dx)) % 2 && (((pp8_cp4[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp8_cp4[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp8_cp4[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp8_cp4[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 39; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if ((((pp8_cp5[i] % 13056) / dx)) % 2 && (((pp8_cp5[i] % 13056) % dx) == 0)) {
                                data_carrier_map_temp[pp8_cp5[i] % 13056] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp8_cp5[i] % 13056] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp8_cp5[i] % 13056] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 3; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp8_16k[i] / dx)) % 2 && ((pp8_16k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp8_16k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp8_16k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp8_16k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
            }
            break;
        case FFTSIZE_32K:
        case FFTSIZE_32K_T2GI:
            switch (dvbt2.pilot_pattern) {
                case PP1:
                    for (int i = 0; i < 20; ++i) {
                        data_carrier_map_temp[pp1_cp1[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 25; ++i) {
                        data_carrier_map_temp[pp1_cp2[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 44; ++i) {
                        data_carrier_map_temp[pp1_cp5[i]] = CONTINUAL_CARRIER;
                    }
                    break;
                case PP2:
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp2_cp1[i] / dx)) % 2 && ((pp2_cp1[i] % dx) == 0)) {
                                data_carrier_map_temp[pp2_cp1[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp2_cp1[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp2_cp1[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 22; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp2_cp2[i] / dx)) % 2 && ((pp2_cp2[i] % dx) == 0)) {
                                data_carrier_map_temp[pp2_cp2[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp2_cp2[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp2_cp2[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 2; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp2_cp3[i] / dx)) % 2 && ((pp2_cp3[i] % dx) == 0)) {
                                data_carrier_map_temp[pp2_cp3[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp2_cp3[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp2_cp3[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 2; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp2_cp4[i] / dx)) % 2 && ((pp2_cp4[i] % dx) == 0)) {
                                data_carrier_map_temp[pp2_cp4[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp2_cp4[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp2_cp4[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 41; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp2_cp5[i] / dx)) % 2 && ((pp2_cp5[i] % dx) == 0)) {
                                data_carrier_map_temp[pp2_cp5[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp2_cp5[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp2_cp5[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 88; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp2_cp6[i] / dx)) % 2 && ((pp2_cp6[i] % dx) == 0)) {
                                data_carrier_map_temp[pp2_cp6[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp2_cp6[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp2_cp6[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 2; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp2_32k[i] / dx)) % 2 && ((pp2_32k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp2_32k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp2_32k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp2_32k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
                case PP3:
                    for (int i = 0; i < 22; ++i) {
                        data_carrier_map_temp[pp3_cp1[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 20; ++i) {
                        data_carrier_map_temp[pp3_cp2[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 1; ++i) {
                        data_carrier_map_temp[pp3_cp3[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 44; ++i) {
                        data_carrier_map_temp[pp3_cp5[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 49; ++i) {
                        data_carrier_map_temp[pp3_cp6[i]] = CONTINUAL_CARRIER;
                    }
                    break;
                case PP4:
                    for (int i = 0; i < 20; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp4_cp1[i] / dx)) % 2 && ((pp4_cp1[i] % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp1[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp1[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp1[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 23; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp4_cp2[i] / dx)) % 2 && ((pp4_cp2[i] % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp2[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp2[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp2[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 1; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp4_cp3[i] / dx)) % 2 && ((pp4_cp3[i] % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp3[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp3[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp3[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 2; ++i)  {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp4_cp4[i] / dx)) % 2 && ((pp4_cp4[i] % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp4[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp4[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp4[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 44; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp4_cp5[i] / dx)) % 2 && ((pp4_cp5[i] % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp5[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp5[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp5[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 86; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp4_cp6[i] / dx)) % 2 && ((pp4_cp6[i] % dx) == 0)) {
                                data_carrier_map_temp[pp4_cp6[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp4_cp6[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp4_cp6[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 2; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp4_32k[i] / dx)) % 2 && ((pp4_32k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp4_32k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp4_32k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp4_32k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
                case PP5:
                    for (int i = 0; i < 19; ++i) {
                        data_carrier_map_temp[pp5_cp1[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 23; ++i) {
                        data_carrier_map_temp[pp5_cp2[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 3; ++i) {
                        data_carrier_map_temp[pp5_cp3[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 1; ++i) {
                        data_carrier_map_temp[pp5_cp4[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 44; ++i) {
                        data_carrier_map_temp[pp5_cp5[i]] = CONTINUAL_CARRIER;
                    }
                    break;
                case PP6:
                    for (int i = 0; i < 88; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp6_cp5[i] / dx)) % 2 && ((pp6_cp5[i] % dx) == 0)) {
                                data_carrier_map_temp[pp6_cp5[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp6_cp5[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp6_cp5[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 88; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp6_cp6[i] / dx)) % 2 && ((pp6_cp6[i] % dx) == 0)) {
                                data_carrier_map_temp[pp6_cp6[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp6_cp6[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp6_cp6[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 4; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp6_32k[i] / dx)) % 2 && ((pp6_32k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp6_32k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp6_32k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp6_32k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
                case PP7:
                    for (int i = 0; i < 15; ++i) {
                        data_carrier_map_temp[pp7_cp1[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 30; ++i) {
                        data_carrier_map_temp[pp7_cp2[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 5; ++i) {
                        data_carrier_map_temp[pp7_cp3[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 3; ++i) {
                        data_carrier_map_temp[pp7_cp4[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 35; ++i) {
                        data_carrier_map_temp[pp7_cp5[i]] = CONTINUAL_CARRIER;
                    }
                    for (int i = 0; i < 92; ++i) {
                        data_carrier_map_temp[pp7_cp6[i]] = CONTINUAL_CARRIER;
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 2; ++i) {
                            data_carrier_map_temp[pp7_32k[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    break;
                case PP8:
                    for (int i = 0; i < 47; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp8_cp4[i] / dx)) % 2 && ((pp8_cp4[i] % dx) == 0)) {
                                data_carrier_map_temp[pp8_cp4[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp8_cp4[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp8_cp4[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 39; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp8_cp5[i] / dx)) % 2 && ((pp8_cp5[i] % dx) == 0)) {
                                data_carrier_map_temp[pp8_cp5[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp8_cp5[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp8_cp5[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    for (int i = 0; i < 89; ++i) {
                        if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                            if (((pp8_cp6[i] / dx)) % 2 && ((pp8_cp6[i] % dx) == 0)) {
                                data_carrier_map_temp[pp8_cp6[i]] = CONTINUAL_CARRIER_INVERTED;
                            }
                            else {
                                data_carrier_map_temp[pp8_cp6[i]] = CONTINUAL_CARRIER;
                            }
                        }
                        else {
                            data_carrier_map_temp[pp8_cp6[i]] = CONTINUAL_CARRIER;
                        }
                    }
                    if (dvbt2.carrier_mode == CARRIERS_EXTENDED) {
                        for (int i = 0; i < 6; ++i) {
                            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                                if (((pp8_32k[i] / dx)) % 2 && ((pp8_32k[i] % dx) == 0)) {
                                    data_carrier_map_temp[pp8_32k[i]] = CONTINUAL_CARRIER_INVERTED;
                                }
                                else {
                                    data_carrier_map_temp[pp8_32k[i]] = CONTINUAL_CARRIER;
                                }
                            }
                            else {
                                data_carrier_map_temp[pp8_32k[i]] = CONTINUAL_CARRIER;
                            }
                        }
                    }
                    break;
            }
            break;
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::sp_mappinng(const int &_idx_symbol)
{
    int remainder;
    int idx_symbol = _idx_symbol;
    for (int i = 0; i < dvbt2.k_total; ++i){
        remainder = (i - dvbt2.k_ext) % (dx * dy);
        if (remainder < 0){
            remainder += (dx * dy);
        }
        if (remainder == (dx * (idx_symbol % dy))){
            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2 && (i / dx) % 2){
                data_carrier_map_temp[i] = SCATTERED_CARRIER_INVERTED;
            }
            else{
                data_carrier_map_temp[i] = SCATTERED_CARRIER;
            }
        }
    }
    if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2 && idx_symbol % 2){
        data_carrier_map_temp[0] = SCATTERED_CARRIER_INVERTED;
        data_carrier_map_temp[dvbt2.k_total - 1] = SCATTERED_CARRIER_INVERTED;
    }
    else{
        data_carrier_map_temp[0] = SCATTERED_CARRIER;
        data_carrier_map_temp[dvbt2.k_total - 1] = SCATTERED_CARRIER;
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::tr_papr_carriers_mapping(const int &_idx_symbol)
{
    int idx_symbol = _idx_symbol;
    int shift;
    if (dvbt2.papr_mode == PAPR_TR || dvbt2.papr_mode == PAPR_BOTH) {
        if (dvbt2.carrier_mode == CARRIERS_NORMAL) {
            shift = dx * (idx_symbol % dy);
        }
        else {
            shift = dx * ((idx_symbol + (dvbt2.k_ext / dx)) % dy);
        }
        switch (dvbt2.fft_mode) {
            case FFTSIZE_1K:
                for (int i = 0; i < 10; ++i) {
                    data_carrier_map_temp[tr_papr_map_1k[i] + shift] = TRPAPR_CARRIER;
                }
                break;
            case FFTSIZE_2K:
                for (int i = 0; i < 18; ++i) {
                    data_carrier_map_temp[tr_papr_map_2k[i] + shift] = TRPAPR_CARRIER;
                }
                break;
            case FFTSIZE_4K:
                for (int i = 0; i < 36; ++i) {
                    data_carrier_map_temp[tr_papr_map_4k[i] + shift] = TRPAPR_CARRIER;
                }
                break;
            case FFTSIZE_8K:
            case FFTSIZE_8K_T2GI:
                for (int i = 0; i < 72; ++i) {
                    data_carrier_map_temp[tr_papr_map_8k[i] + shift] = TRPAPR_CARRIER;
                }
                break;
            case FFTSIZE_16K:
            case FFTSIZE_16K_T2GI:
                for (int i = 0; i < 144; ++i) {
                    data_carrier_map_temp[tr_papr_map_16k[i] + shift] = TRPAPR_CARRIER;
                }
                break;
            case FFTSIZE_32K:
            case FFTSIZE_32K_T2GI:
                for (int i = 0; i < 288; ++i) {
                    data_carrier_map_temp[tr_papr_map_32k[i] + shift] = TRPAPR_CARRIER;
                }
                break;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::fc_carrier_mapping()
{
    for (int i = 0; i < dvbt2.k_total; ++i) {
        fc_carrier_map[i] = DATA_CARRIER;
    }
    for (int i = 0; i < dvbt2.k_total; ++i) {
        if (i % dx == 0) {
            if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
                if ((i / dx) % 2) {
                    fc_carrier_map[i] = SCATTERED_CARRIER_INVERTED;
                }
                else {
                    fc_carrier_map[i] = SCATTERED_CARRIER;
                }
            }
            else {
                fc_carrier_map[i] = SCATTERED_CARRIER;
            }
        }
    }
    if (dvbt2.fft_mode == FFTSIZE_1K && dvbt2.pilot_pattern == PP4) {
        fc_carrier_map[dvbt2.k_total - 2] = SCATTERED_CARRIER;
    }
    else if (dvbt2.fft_mode == FFTSIZE_1K && dvbt2.pilot_pattern == PP5) {
        fc_carrier_map[dvbt2.k_total - 2] = SCATTERED_CARRIER;
    }
    else if (dvbt2.fft_mode == FFTSIZE_2K && dvbt2.pilot_pattern == PP7) {
        fc_carrier_map[dvbt2.k_total - 2] = SCATTERED_CARRIER;
    }
    if (dvbt2.miso == TRUE && dvbt2.miso_group == MISO_TX2) {
        if ((dvbt2.n_data + dvbt2.n_p2 - 1) % 2) {
            fc_carrier_map[0] = SCATTERED_CARRIER_INVERTED;
            fc_carrier_map[dvbt2.k_total - 1] = SCATTERED_CARRIER_INVERTED;
        }
        else {
            fc_carrier_map[0] = SCATTERED_CARRIER;
            fc_carrier_map[dvbt2.k_total - 1] = SCATTERED_CARRIER;
        }
    }
    else {
        fc_carrier_map[0] = SCATTERED_CARRIER;
        fc_carrier_map[dvbt2.k_total - 1] = SCATTERED_CARRIER;
    }
    if (dvbt2.papr_mode == PAPR_TR || dvbt2.papr_mode == PAPR_BOTH) {
        switch (dvbt2.fft_mode) {
        case FFTSIZE_1K:
            for (int i = 0; i < 10; ++i) {
                fc_carrier_map[p2_papr_map_1k[i]] = TRPAPR_CARRIER;
            }
            break;
        case FFTSIZE_2K:
            for (int i = 0; i < 18; ++i) {
                fc_carrier_map[p2_papr_map_2k[i]] = TRPAPR_CARRIER;
            }
            break;
        case FFTSIZE_4K:
            for (int i = 0; i < 36; ++i) {
                fc_carrier_map[p2_papr_map_4k[i]] = TRPAPR_CARRIER;
            }
            break;
        case FFTSIZE_8K:
        case FFTSIZE_8K_T2GI:
            for (int i = 0; i < 72; ++i) {
                fc_carrier_map[p2_papr_map_8k[i] + dvbt2.k_ext] = TRPAPR_CARRIER;
            }
            break;
        case FFTSIZE_16K:
        case FFTSIZE_16K_T2GI:
            for (int i = 0; i < 144; ++i) {
                fc_carrier_map[p2_papr_map_16k[i] + dvbt2.k_ext] = TRPAPR_CARRIER;
            }
            break;
        case FFTSIZE_32K:
        case FFTSIZE_32K_T2GI:
            for (int i = 0; i < 288; ++i) {
                fc_carrier_map[p2_papr_map_32k[i] + dvbt2.k_ext] = TRPAPR_CARRIER;
            }
            break;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::p2_modulation()
{
    for (int i = 0; i < dvbt2.n_p2; ++i) {
        for (int n = 0; n < dvbt2.k_total; n++) {
            switch (p2_carrier_map[n]) {
            case DATA_CARRIER:
                p2_pilot_refer[i][n] = 0.0;
                break;
            case P2CARRIER:
                p2_pilot_refer[i][n] = p2_bpsk[prbs[n + dvbt2.k_offset] ^ pn_sequence[i]];
                break;
            case P2PAPR_CARRIER:
                p2_pilot_refer[i][n] = 0.0;
                break;
            default:
                break;
            }
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void pilot_generator::modulation()
{
    int idx_data = 0;
    for (int j = dvbt2.n_p2; j < dvbt2.len_frame; j++) {
        if (j == (dvbt2.len_frame - dvbt2.l_fc)){
            for (int n = 0; n < dvbt2.k_total; n++) {
                switch (fc_carrier_map[n]) {
                case DATA_CARRIER:
                    fc_pilot_refer[n] = 0.0;
                    break;
                case SCATTERED_CARRIER:
                    fc_pilot_refer[n] = sp_bpsk[prbs[n + dvbt2.k_offset] ^ pn_sequence[j]];
                    break;
                case SCATTERED_CARRIER_INVERTED:
                    fc_pilot_refer[n] = sp_bpsk_inverted[prbs[n + dvbt2.k_offset] ^ pn_sequence[j]];
                    break;
                case TRPAPR_CARRIER:
                    fc_pilot_refer[n] = 0.0;
                    break;
                default:
                    break;
                }
            }
        }
        else{
            for (int n = 0; n < dvbt2.k_total; n++) {
                switch (data_carrier_map[idx_data][n]) {
                case DATA_CARRIER:
                    data_pilot_refer[idx_data][n] = 0.0;
                    break;
                case SCATTERED_CARRIER:
                    data_pilot_refer[idx_data][n] = sp_bpsk[prbs[n + dvbt2.k_offset] ^ pn_sequence[j]];
                    break;
                case SCATTERED_CARRIER_INVERTED:
                    data_pilot_refer[idx_data][n] = sp_bpsk_inverted[prbs[n + dvbt2.k_offset] ^ pn_sequence[j]];
                    break;
                case CONTINUAL_CARRIER:
                    data_pilot_refer[idx_data][n] = cp_bpsk[prbs[n + dvbt2.k_offset] ^ pn_sequence[j]];
                    break;
                case CONTINUAL_CARRIER_INVERTED:
                    data_pilot_refer[idx_data][n] = cp_bpsk_inverted[prbs[n + dvbt2.k_offset] ^ pn_sequence[j]];
                    break;
                case TRPAPR_CARRIER:
                    data_pilot_refer[idx_data][n] = 0.0;
                    break;
                default:
                    break;
                }
            }
            idx_data++;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------------
