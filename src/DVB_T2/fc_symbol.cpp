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
#include "fc_symbol.h"

//#include <QDebug>

#include "DSP/fast_math.h"

//-------------------------------------------------------------------------------------------
fc_symbol::fc_symbol(QObject* parent) : QObject(parent)
{

}
//-------------------------------------------------------------------------------------------
fc_symbol::~fc_symbol()
{   
    if(est_show != nullptr){
        delete [] est_show;
        delete [] show_symbol;
        delete [] show_data;

        delete [] show_est_data;
    }
}
//-------------------------------------------------------------------------------------------
void fc_symbol::init(dvbt2_parameters _dvbt2, pilot_generator* _pilot,
                     address_freq_deinterleaver* _address)
{
    table_sin_cos_instance.table_();

    fft_size = _dvbt2.fft_size;
    half_fft_size = fft_size / 2;
    pilot = _pilot;
    address = _address;
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
    idx_symbol = _dvbt2.len_frame - 1;
    k_total = _dvbt2.k_total;
    half_total = k_total / 2;
    n_fc = _dvbt2.n_fc;
    c_fc = _dvbt2.c_fc;
    left_nulls = _dvbt2.l_nulls;
    fc_carrier_map = pilot->fc_carrier_map;
    fc_pilot_refer = pilot->fc_pilot_refer;
    h_even_fc = address->h_even_fc;
    h_odd_fc = address->h_odd_fc;
    deinterleaved_cell = new complex[n_fc];
    est_show = new complex[fft_size];
    show_symbol = new complex[fft_size];
    show_data = new complex[n_fc];

    show_est_data = new complex[k_total];
}
//-------------------------------------------------------------------------------------------
complex* fc_symbol::execute(complex* _ofdm_cell, float &_sample_rate_offset, float &_phase_offset)
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
    int len_show = 0;
    float pilot_refer;
    float amp_pilot = amp_sp;
    complex cell;
    complex old_cell = {0.0, 0.0};
    complex derotate;
    int idx_data = 0;
    int d = 0;
    int* h;
    if(idx_symbol % 2 == 0) h = h_odd_fc;
    else h = h_even_fc;

    //__for first pilot______
    cell = ofdm_cell[0];
    pilot_refer = fc_pilot_refer[0];
    if(pilot_refer < 0) {
        cell = -cell;
        pilot_refer = -pilot_refer;
    }
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
        pilot_refer = fc_pilot_refer[i];
        switch (fc_carrier_map[i]){
        case DATA_CARRIER:
            buffer_cell[idx_data] = cell;
            ++idx_data;
            break;
        case SCATTERED_CARRIER_INVERTED:
        case SCATTERED_CARRIER:
            est_dif = cell * conj(old_cell);
            old_cell = cell;
            est_pilot = cell * pilot_refer;
            sum_pilot_1 += est_pilot;
            sum_dif_1 += est_dif;
            angle = atan2_approx(est_pilot.imag(), est_pilot.real());

            dif_angle = angle - angle_est;
            if(dif_angle > M_PIf32 ) dif_angle = M_PIf32 * 2.0f - dif_angle;
            else if(dif_angle < -M_PIf32) dif_angle = M_PIf32 * 2.0f + dif_angle;
            sum_angle_1 += angle;

            delta_angle = (dif_angle) / (idx_data + 1);
            amp = sqrt(norm(cell)) / amp_pilot;
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

    cell = ofdm_cell[half_total];
    pilot_refer = fc_pilot_refer[half_total];
    switch (fc_carrier_map[half_total]){
    case DATA_CARRIER:
        buffer_cell[idx_data] = cell;
        ++idx_data;
        break;
    case SCATTERED_CARRIER_INVERTED:
    case SCATTERED_CARRIER:
        est_dif = cell * conj(old_cell);
        est_pilot = cell * pilot_refer;
        angle = atan2_approx(est_pilot.imag(), est_pilot.real());
        amp = sqrt(norm(cell)) / amp_pilot;
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

    for (int i = half_total + 1; i < k_total; ++i){
        cell = ofdm_cell[i];
        pilot_refer = fc_pilot_refer[i];
        switch (fc_carrier_map[i]){
        case DATA_CARRIER:
            buffer_cell[idx_data] = cell;
            ++idx_data;
            break;
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

    _sample_rate_offset = (sum_angle_2 - sum_angle_1)/* / k_total*/;

    int len = n_fc;
    memcpy(show_symbol, _ofdm_cell, sizeof(complex) * static_cast<unsigned int>(fft_size));
    memcpy(show_data, deinterleaved_cell, sizeof(complex) * static_cast<unsigned int>(len));
    emit replace_spectrograph(fft_size, &show_symbol[0]);
    emit replace_constelation(len, &show_data[0]);

    return deinterleaved_cell;
}
//-------------------------------------------------------------------------------------------
