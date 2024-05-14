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
#include "address_freq_deinterleaver.h"

//-------------------------------------------------------------------------------------------
address_freq_deinterleaver::address_freq_deinterleaver()
{

}
//-------------------------------------------------------------------------------------------
address_freq_deinterleaver::~address_freq_deinterleaver()
{

}
//-------------------------------------------------------------------------------------------
void address_freq_deinterleaver::init(dvbt2_parameters _dvbt2)
{
    dvbt2_parameters dvbt2 = _dvbt2;
    int xor_size, pn_mask, result;
    int lfsr = 0;
    int logic1k[2] = {0, 4};
    int logic2k[2] = {0, 3};
    int logic4k[2] = {0, 2};
    int logic8k[4] = {0, 1, 4, 6};
    int logic16k[6] = {0, 1, 4, 5, 9, 11};
    int logic32k[4] = {0, 1, 2, 12};
    int *logic;
    const int *bitpermeven = &bitperm1keven[0];
    const int *bitpermodd = &bitperm1kodd[0];
    int pn_degree, even, odd;
    switch (dvbt2.fft_mode){
    case FFTSIZE_1K:
        pn_degree = 9;
        pn_mask = 0x1ff;
        max_states = 1024;
        logic = &logic1k[0];
        xor_size = 2;
        bitpermeven = &bitperm1keven[0];
        bitpermodd = &bitperm1kodd[0];
        break;
    case FFTSIZE_2K:
        pn_degree = 10;
        pn_mask = 0x3ff;
        max_states = 2048;
        logic = &logic2k[0];
        xor_size = 2;
        bitpermeven = &bitperm2keven[0];
        bitpermodd = &bitperm2kodd[0];
        break;
    case FFTSIZE_4K:
        pn_degree = 11;
        pn_mask = 0x7ff;
        max_states = 4096;
        logic = &logic4k[0];
        xor_size = 2;
        bitpermeven = &bitperm4keven[0];
        bitpermodd = &bitperm4kodd[0];
        break;
    case FFTSIZE_8K:
    case FFTSIZE_8K_T2GI:
        pn_degree = 12;
        pn_mask = 0xfff;
        max_states = 8192;
        logic = &logic8k[0];
        xor_size = 4;
        bitpermeven = &bitperm8keven[0];
        bitpermodd = &bitperm8kodd[0];
        break;
    case FFTSIZE_16K:
    case FFTSIZE_16K_T2GI:
        pn_degree = 13;
        pn_mask = 0x1fff;
        max_states = 16384;
        logic = &logic16k[0];
        xor_size = 6;
        bitpermeven = &bitperm16keven[0];
        bitpermodd = &bitperm16kodd[0];
        break;
    case FFTSIZE_32K:
    case FFTSIZE_32K_T2GI:
        pn_degree = 14;
        pn_mask = 0x3fff;
        max_states = 32768;
        logic = &logic32k[0];
        xor_size = 4;
        bitpermeven = &bitperm32k[0];
        bitpermodd = &bitperm32k[0];
        break;
    default:
        pn_degree = 0;
        pn_mask = 0;
        max_states = 0;
        logic = &logic1k[0];
        xor_size = 0;
        break;
    }
    for (int i = 0; i < max_states; i++){
        if (i == 0 || i == 1){
            lfsr = 0;
        }else if (i == 2){
            lfsr = 1;
        }else{
            result = 0;
            for (int k = 0; k < xor_size; k++){
                result ^= (lfsr >> logic[k]) & 1;
            }
            lfsr &= pn_mask;
            lfsr >>= 1;
            lfsr |= result << (pn_degree - 1);
        }
        even = 0;
        odd = 0;
        for (int n = 0; n < pn_degree; n++){
            even |= ((lfsr >> n) & 0x1) << bitpermeven[n];
        }
        for (int n = 0; n < pn_degree; n++){
            odd |= ((lfsr >> n) & 0x1) << bitpermodd[n];
        }
        max_even[i] = even + ((i % 2) * (max_states / 2));
        max_odd[i] = odd + ((i % 2) * (max_states / 2));
    }
}
//-------------------------------------------------------------------------------------------
void address_freq_deinterleaver::p2_address_freq_deinterleaver(dvbt2_parameters _dvbt2)
{
    dvbt2_parameters dvbt2 = _dvbt2;
    int q_even_p2 = 0;
    int q_odd_p2 = 0;
    for (int i = 0; i < max_states; i++){
        if (max_even[i] < dvbt2.c_p2){
            h_even_p2_tx[q_even_p2++] = max_even[i];
        }
        if (max_odd[i] < dvbt2.c_p2){
            h_odd_p2_tx[q_odd_p2++] = max_odd[i];
        }
    }
    if (dvbt2.fft_mode == FFTSIZE_32K || dvbt2.fft_mode == FFTSIZE_32K_T2GI) {
        for (int j = 0; j < q_odd_p2; ++j) {
            int a;
            a = h_odd_p2_tx[j];
            h_even_p2_tx[a] = j;
        }
    }
    for(int i = 0; i < q_even_p2; i++){
        h_even_p2[h_even_p2_tx[i]] = i;
    }
    for(int i = 0; i < q_odd_p2; i++){
        h_odd_p2[h_odd_p2_tx[i]] = i;
    }
}
//-------------------------------------------------------------------------------------------
void address_freq_deinterleaver::data_address_freq_deinterleaver(dvbt2_parameters _dvbt2)
{
    dvbt2_parameters dvbt2 = _dvbt2;
    int q_even = 0;
    int q_odd = 0;
    int q_even_fc = 0;
    int q_odd_fc = 0;
    for (int i = 0; i < max_states; i++){
        if (max_even[i] < dvbt2.c_data){
            h_even_data_tx[q_even++] = max_even[i];
        }
        if (max_odd[i] < dvbt2.c_data){
            h_odd_data_tx[q_odd++] = max_odd[i];
        }
        if (max_even[i] < dvbt2.n_fc){
            h_even_fc_tx[q_even_fc++] = max_even[i];
        }
        if (max_odd[i] < dvbt2.n_fc){
            h_odd_fc_tx[q_odd_fc++] = max_odd[i];
        }
    }
    if (dvbt2.fft_mode == FFTSIZE_32K || dvbt2.fft_mode == FFTSIZE_32K_T2GI) {
        for (int j = 0; j < q_odd; j++) {
            int a;
            a = h_odd_data_tx[j];
            h_even_data_tx[a] = j;
        }
        for (int j = 0; j < q_odd_fc; j++) {
            int a;
            a = h_odd_fc_tx[j];
            h_even_fc_tx[a] = j;
        }
    }
    for(int i = 0; i < q_even; i++){
        h_even_data[h_even_data_tx[i]] = i;
    }
    for(int i = 0; i < q_odd; i++){
        h_odd_data[h_odd_data_tx[i]] = i;
    }
    for(int i = 0; i < q_even_fc; i++){
        h_even_fc[h_even_fc_tx[i]] = i;
    }
    for(int i = 0; i < q_odd_fc; i++){
        h_odd_fc[h_odd_fc_tx[i]] = i;
    }
}
//-------------------------------------------------------------------------------------------
