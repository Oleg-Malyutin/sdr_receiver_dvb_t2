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
#include "data_symbol.h"

//#include <QDebug>

#include "DSP/fast_math.h"

//-------------------------------------------------------------------------------------------
data_symbol::data_symbol(QObject* parent) : QObject(parent)
{

}
//-------------------------------------------------------------------------------------------
data_symbol::~data_symbol()
{
    if(est_show != nullptr){
        delete [] buffer_cell;
        delete [] est_show;
        delete [] show_symbol;
        delete [] show_data;

        delete [] show_est_data;
    }
}
//-------------------------------------------------------------------------------------------
void data_symbol::init(dvbt2_parameters &_dvbt2, pilot_generator* _pilot,
                       address_freq_deinterleaver* _address)
{
    table_sin_cos_instance.table_();

    fft_size = _dvbt2.fft_size;//32k
    half_fft_size = fft_size / 2;
    pilot = _pilot;
    address = _address;
    switch (_dvbt2.fft_mode)
    {
    case FFTSIZE_1K:
    case FFTSIZE_2K:
        amp_cp = 4.0f / 3.0f;
        break;
     case FFTSIZE_4K:
        amp_cp = (4.0f * sqrtf(2)) / 3.0f;

        break;
    case FFTSIZE_8K:
    case FFTSIZE_8K_T2GI:
    case FFTSIZE_16K:
    case FFTSIZE_16K_T2GI:
    case FFTSIZE_32K:
    case FFTSIZE_32K_T2GI:
        amp_cp = 8.0f / 3.0f;
        break;
    }
    dvbt2_data_parameters_init(_dvbt2);
    switch (_dvbt2.pilot_pattern)
    {
    case PP1:
    case PP2:
        amp_sp = 4.0f / 3.0f;
        break;
    case PP3:
    case PP4:
        amp_sp = 7.0f / 4.0f;
        break;
    case PP5:
    case PP6:
    case PP7:
    case PP8:
        amp_sp = 7.0f / 3.0f;
        break;
    }
    cor_amp_cp = amp_sp / amp_cp;
    n_data = _dvbt2.n_data;
    c_data = _dvbt2.c_data;
    k_total = _dvbt2.k_total;
    half_total = k_total / 2;
    n_p2 = _dvbt2.n_p2;
    left_nulls = _dvbt2.l_nulls;
    pilot->data_generator(_dvbt2);
    data_carrier_map = pilot->data_carrier_map;
    data_pilot_refer = pilot->data_pilot_refer;
    buffer_cell = new complex[c_data];
    address->data_address_freq_deinterleaver(_dvbt2);
    h_even_data = address->h_even_data;
    h_odd_data = address->h_odd_data;
    deinterleaved_buffer_a = new complex[c_data];
    deinterleaved_buffer_b = new complex[c_data];
    est_show = new complex[fft_size];
    show_symbol = new complex[fft_size];
    show_data = new complex[c_data];

    show_est_data = new complex[k_total];
}
//-------------------------------------------------------------------------------------------
complex* data_symbol::execute(int _idx_symbol, complex* _ofdm_cell,
                              float &_sample_rate_offset, float &_phase_offset)
{
    complex* ofdm_cell = &_ofdm_cell[left_nulls];
    complex est_pilot, est_dif;
    float angle = 0.0f;
    float delta_angle = 0.0f;
    float angle_est = 0.0f;
    float amp = 0.0f;
    float delta_amp = 0.0f;
    float amp_est = 0.0f;
    float dif_angle = 0.0f;
    float sum_angle_1 = 0.0f;
    float sum_angle_2 = 0.0f;
    complex sum_dif_1 = {0.0f, 0.0f};
    complex sum_dif_2 = {0.0f, 0.0f};
    complex sum_pilot_1 = {0.0f, 0.0f};
    complex sum_pilot_2 = {0.0f, 0.0f};
    int idx_symbol = _idx_symbol;
    int idx_data_symbol = idx_symbol - n_p2;
    int len_show = 0;
    float* pilot_refer_idx_data_symbol = data_pilot_refer[idx_data_symbol];
    float pilot_refer;
    int* map = data_carrier_map[idx_data_symbol];
    float amp_pilot = amp_sp;
    complex cell;
    complex old_cell = {0.0, 0.0};
    complex derotate;
    int idx_data = 0;
    int d = 0;
    int* h;
    complex* deinterleaved_cell;
    if(swap_buffer){
        swap_buffer = false;
        deinterleaved_cell = deinterleaved_buffer_b;
    }
    else{
        swap_buffer = true;
        deinterleaved_cell = deinterleaved_buffer_a;
    }
    if(idx_symbol % 2 == 0) h = h_odd_data;
    else h = h_even_data;
    //__for first pilot______
    cell = ofdm_cell[0];
    pilot_refer = pilot_refer_idx_data_symbol[0];
    est_dif = cell * conj(old_cell);
    old_cell = cell;
    est_pilot = cell * pilot_refer;
    est_dif = cell * conj(old_cell);
    old_cell = cell;
    sum_pilot_1 += est_pilot;
    sum_dif_1 += est_dif;
    angle_est = atan2_approx(est_pilot.imag(), est_pilot.real());
    amp_est = sqrt(norm(cell)) / amp_pilot;
    //__ ... ______________

    for (int i = 1; i < half_total; ++i){
        cell = ofdm_cell[i];
        pilot_refer = pilot_refer_idx_data_symbol[i];
        switch (map[i]){
        case DATA_CARRIER:
            buffer_cell[idx_data] = cell;
            ++idx_data;
            break;
        case CONTINUAL_CARRIER:
            amp_pilot = amp_cp;
           [[clang::fallthrough]];//_to suppress compiler warnings:
//            break;
        case CONTINUAL_CARRIER_INVERTED:
            amp_pilot = amp_cp;
           [[clang::fallthrough]];//_to suppress compiler warnings:
//            break;
        case SCATTERED_CARRIER_INVERTED:
        case SCATTERED_CARRIER:
            est_dif = cell * conj(old_cell);
            old_cell = cell;
            est_pilot = cell * pilot_refer;
            sum_pilot_1 += est_pilot;
            sum_dif_1 += est_dif;
            angle = atan2_approx(est_pilot.imag(), est_pilot.real());

            dif_angle = angle - angle_est;
            if(dif_angle > M_PIf32) dif_angle = M_PIf32 * 2.0f - dif_angle;
            else if(dif_angle < -M_PIf32) dif_angle = M_PIf32 * 2.0f + dif_angle;
            sum_angle_1 += angle;

            delta_angle = (dif_angle) / (idx_data + 1);
            amp = sqrt(norm(cell)) / amp_pilot;
            amp_pilot = amp_sp;
            delta_amp = (amp - amp_est) / (idx_data + 1);
            for(int j = 0; j < idx_data; ++j){
                angle_est += delta_angle;
                amp_est += delta_amp;
                derotate.real(cos_lut(angle_est) / amp_est);
                derotate.imag(sin_lut(angle_est) / amp_est);
                deinterleaved_cell[h[d]] = buffer_cell[j] * conj(derotate);
                ++d;
            }
            idx_data = 0;
            angle_est = angle;
            amp_est = amp;
            //Only for show
            est_show[len_show].real(angle);
            est_show[len_show].imag(amp);
            ++len_show;
            // ...
            break;
        case TRPAPR_CARRIER:
            //TODO
            //printf("TRPAPR_CARRIER\n");
            break;
        default:
            break;
        }
    }
    //
    cell = ofdm_cell[half_total];
    pilot_refer = pilot_refer_idx_data_symbol[half_total];
    switch (map[half_total]){
    case DATA_CARRIER:
        buffer_cell[idx_data] = cell;
        ++idx_data;
        break;
    case CONTINUAL_CARRIER:
        amp_pilot = amp_cp;
       [[clang::fallthrough]];//_to suppress compiler warnings:
        //            break;
    case CONTINUAL_CARRIER_INVERTED:
        amp_pilot = amp_cp;
       [[clang::fallthrough]];//_to suppress compiler warnings:
        //            break;
    case SCATTERED_CARRIER_INVERTED:
    case SCATTERED_CARRIER:
        est_dif = cell * conj(old_cell);
        est_pilot = cell * pilot_refer;
        amp_pilot = amp_sp;
        //Only for show
        est_show[len_show].real(atan2_approx(est_pilot.imag(), est_pilot.real()));
        est_show[len_show].imag(sqrt(norm(cell)) / amp_pilot);
        ++len_show;
        // ...
        break;
    case TRPAPR_CARRIER:
        //TODO
        //printf("TRPAPR_CARRIER\n");
        break;
    default:
        break;
    }

    int c = half_total + 1;
    for (int i = c; i < k_total; ++i){
        cell = ofdm_cell[i];
        pilot_refer = pilot_refer_idx_data_symbol[i];
        switch (map[i]){
        case DATA_CARRIER:
            buffer_cell[idx_data] = cell;
            ++idx_data;
            break;
        case CONTINUAL_CARRIER:
            amp_pilot = amp_cp;
           [[clang::fallthrough]];//_to suppress compiler warnings:
//            break;
        case CONTINUAL_CARRIER_INVERTED:
            amp_pilot = amp_cp;
           [[clang::fallthrough]];//_to suppress compiler warnings:
//            break;
        case SCATTERED_CARRIER_INVERTED:
        case SCATTERED_CARRIER:
            est_dif = cell * conj(old_cell);
            old_cell = cell;
            est_pilot = cell * pilot_refer;
            sum_pilot_2 += est_pilot;
            sum_dif_2 += est_dif;
            angle = atan2_approx(est_pilot.imag(), est_pilot.real());

            dif_angle = angle - angle_est;
            if(dif_angle > M_PIf32) dif_angle = M_PIf32 * 2.0f - dif_angle;
            else if(dif_angle < -M_PIf32) dif_angle = M_PIf32 * 2.0f + dif_angle;
            sum_angle_2 += angle;

            delta_angle = (dif_angle) / (idx_data + 1);
            amp = sqrt(norm(cell)) / amp_pilot;
            amp_pilot = amp_sp;
            delta_amp = (amp - amp_est) / (idx_data + 1);
            for(int j = 0; j < idx_data; ++j){
                angle_est += delta_angle;
                amp_est += delta_amp;
                derotate.real(cos_lut(angle_est) / amp_est);
                derotate.imag(sin_lut(angle_est) / amp_est);
                deinterleaved_cell[h[d]] = buffer_cell[j] * conj(derotate);
                ++d;
            }
            idx_data = 0;
            angle_est = angle;
            amp_est = amp;
            //Only for show
            est_show[len_show].real(angle);
            est_show[len_show].imag(amp);
            ++len_show;
            // ...
            break;
        case TRPAPR_CARRIER:
            //TODO
            //printf("TRPAPR_CARRIER\n");
            break;
        default:
            break;
        }
    }

    float ph_1 = atan2_approx(sum_pilot_1.imag(), sum_pilot_1.real());
    float ph_2 = atan2_approx(sum_pilot_2.imag(), sum_pilot_2.real());

    _phase_offset = (ph_2 + ph_1);

    _sample_rate_offset = (sum_angle_2 - sum_angle_1);

    if(idx_symbol == n_p2) {
        int len = c_data;
        memcpy(show_symbol, _ofdm_cell, sizeof(complex) * static_cast<unsigned long>(fft_size));
        memcpy(show_data, deinterleaved_cell, sizeof(complex) * static_cast<unsigned long>(len));
        emit replace_spectrograph(fft_size, &show_symbol[0]);
        emit replace_constelation(len, &show_data[0]);
    }

    return deinterleaved_cell;
}
//-------------------------------------------------------------------------------------------
