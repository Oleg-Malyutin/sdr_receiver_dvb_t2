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
#ifndef ADDRESS_FREQ_DEINTERLEAVER_H
#define ADDRESS_FREQ_DEINTERLEAVER_H

#include "dvbt2_definition.h"

class address_freq_deinterleaver
{
public:
    explicit address_freq_deinterleaver();
    ~address_freq_deinterleaver();
    void init(dvbt2_parameters _dvbt2);
    void p2_address_freq_deinterleaver(dvbt2_parameters _dvbt2);
    void data_address_freq_deinterleaver(dvbt2_parameters _dvbt2);
    int h_even_p2[32768];
    int h_odd_p2[32768];
    int h_even_data[32768];
    int h_odd_data[32768];
    int h_even_fc[32768];
    int h_odd_fc[32768];

private:
    int max_states;
    int max_even[32768];
    int max_odd[32768];
    int h_even_data_tx[32768];
    int h_odd_data_tx[32768];
    int h_even_p2_tx[32768];
    int h_odd_p2_tx[32768];
    int h_even_fc_tx[32768];
    int h_odd_fc_tx[32768];

    const int bitperm1keven[9] =
    {
        8, 7, 6, 5, 0, 1, 2, 3, 4
    };
    //-------------------------------------------------------------------------------------------
    const int bitperm1kodd[9] =
    {
        6, 8, 7, 4, 1, 0, 5, 2, 3
    };
    //-------------------------------------------------------------------------------------------
    const int bitperm2keven[10] =
    {
        4, 3, 9, 6, 2, 8, 1, 5, 7, 0
    };
    //-------------------------------------------------------------------------------------------
    const int bitperm2kodd[10] =
    {
        6, 9, 4, 8, 5, 1, 0, 7, 2, 3
    };
    //-------------------------------------------------------------------------------------------
    const int bitperm4keven[11] =
    {
        6, 3, 0, 9, 4, 2, 1, 8, 5, 10, 7
    };
    //-------------------------------------------------------------------------------------------
    const int bitperm4kodd[11] =
    {
        5, 9, 1, 4, 3, 0, 8, 10, 7, 2, 6
    };
    //-------------------------------------------------------------------------------------------
    const int bitperm8keven[12] =
    {
        7, 1, 4, 2, 9, 6, 8, 10, 0, 3, 11, 5
    };
    //-------------------------------------------------------------------------------------------
    const int bitperm8kodd[12] =
    {
        11, 4, 9, 3, 1, 2, 5, 0, 6, 7, 10, 8
    };
    //-------------------------------------------------------------------------------------------
    const int bitperm16keven[13] =
    {
        9, 7, 6, 10, 12, 5, 1, 11, 0, 2, 3, 4, 8
    };
    //-------------------------------------------------------------------------------------------
    const int bitperm16kodd[13] =
    {
        6, 8, 10, 12, 2, 0, 4, 1, 11, 3, 5, 9, 7
    };
    //-------------------------------------------------------------------------------------------
    const int bitperm32k[14] =
    {
        7, 13, 3, 4, 9, 2, 12, 11, 1, 8, 10, 0, 5, 6
    };
    //-------------------------------------------------------------------------------------------
};

#endif /* ADDRESS_FREQ_DEINTERLEAVER_H */

