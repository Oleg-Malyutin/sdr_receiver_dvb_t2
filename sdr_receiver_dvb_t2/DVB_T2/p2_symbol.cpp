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
#include "p2_symbol.h"

#include "DSP/fast_math.h"

//----------------------------------------------------------------------------------------------------------------------------
p2_symbol::p2_symbol(QObject *parent) : QObject(parent)
{
    table_sin_cos_instance.table_();
}
//----------------------------------------------------------------------------------------------------------------------------
p2_symbol::~p2_symbol()
{
    if(deinterleaved_cell != nullptr) delete [] deinterleaved_cell;
    if(show_symbol != nullptr) delete [] show_symbol;
    if(show_data != nullptr) delete [] show_data;
    if(est_data != nullptr) delete [] est_data;
    if(show_est_data != nullptr) delete [] show_est_data;
    if(l1_post.rf != nullptr) delete [] l1_post.rf;
    if(l1_post.plp != nullptr) delete [] l1_post.plp;
    if(l1_post.aux != nullptr) delete [] l1_post.aux;
    if(l1_post_bit != nullptr) delete [] l1_post_bit;
    if(l1_post_bit_interleaving != nullptr) delete [] l1_post_bit_interleaving;
    if(l1_post.dyn.plp != nullptr) delete [] l1_post.dyn.plp;
    if(l1_post.dyn.aux_private_dyn != nullptr) delete [] l1_post.dyn.aux_private_dyn;
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::init(dvbt2_parameters &_dvbt2, pilot_generator* _pilot,
                               address_freq_deinterleaver* _address)
{
    fft_size = _dvbt2.fft_size;//32k
    if ((_dvbt2.fft_mode == FFTSIZE_32K || _dvbt2.fft_mode == FFTSIZE_32K_T2GI) &&
            (_dvbt2.miso == FALSE)) {
        amp_p2 = sqrt(37.0f) / 5.0f;
    }
    else {
        amp_p2 = sqrt(31.0f) / 5.0f;
    }
    dvbt2_p2_parameters_init(_dvbt2);
    n_p2 = _dvbt2.n_p2;
    c_p2 = _dvbt2.c_p2;
    k_total = _dvbt2.k_total;
    half_total = k_total / 2;
    left_nulls = _dvbt2.l_nulls;
    _pilot->p2_generator(_dvbt2);
    p2_carrier_map = _pilot->p2_carrier_map;
    p2_pilot_refer = _pilot->p2_pilot_refer;
    _address->p2_address_freq_deinterleaver(_dvbt2);
    h_even_p2 = _address->h_even_p2;
    h_odd_p2 = _address->h_odd_p2;
    deinterleaved_cell = new complex[c_p2];

    show_symbol = new complex[fft_size];
    show_data = new complex[c_p2];
    est_data = new complex[k_total];
    show_est_data = new complex[k_total];

    init_l1_randomizer();
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::init_l1_randomizer()
{
    int sr = 0x4A80;
    for (int i = 0; i < KBCH_1_2; ++i) {
        int b = ((sr) ^ (sr >> 1)) & 1;
        l1_randomize[i] = static_cast<unsigned char>(b);
        sr >>= 1;
        if(b) sr |= 0x4000;
    }
}
//----------------------------------------------------------------------------------------------------------------------------
complex* p2_symbol::execute(dvbt2_parameters &_dvbt2, bool _demod_init, int &_idx_symbol, complex* _ofdm_cell,
                            l1_presignalling &_l1_pre, l1_postsignalling &_l1_post, bool &_crc32_l1_pre,
                            bool &_crc32_l1_post, float &_sample_rate_offset, float &_phase_offset)
{
    complex* ofdm_cell = &_ofdm_cell[left_nulls];
    complex old_cell = {-1.0f, 0.0f};
    complex est_pilot, est_dif;
    float angle = 0.0f;
    float delta_angle = 0.0f;
    float angle_est = 0.0f;
    float amp = 0.0f;
    float delta_amp = 0.0f;
    float amp_est = 0.0f;
    complex sum_dif_1 = {0.0f, 0.0f};
    complex sum_dif_2 = {0.0f, 0.0f};
    complex sum_pilot_1 = {0.0f, 0.0f};
    complex sum_pilot_2 = {0.0f, 0.0f};
    int idx_symbol = _idx_symbol;
    int len_est = 0;
    float* pilot_refer_idx_p2_symbol = p2_pilot_refer[idx_symbol];
    float pilot_refer;
    float amp_pilot = amp_p2;
    complex cell;
    complex derotate;
    int idx_data = 0;
    int d = 0;
    int* h;

    if(idx_symbol % 2 == 0) h = h_odd_p2;
    else h = h_even_p2;

    //__for first pilot______
    cell = ofdm_cell[0];
    pilot_refer = pilot_refer_idx_p2_symbol[0];
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
    amp_est = amp;
    //Only for show
    est_data[len_est].real(angle_est);
    est_data[len_est].imag(amp_est);
    ++len_est;
    //__ ... ______________

    for (int i = 1; i < half_total; ++i){
        cell = ofdm_cell[i];
        pilot_refer = pilot_refer_idx_p2_symbol[i];
        switch (p2_carrier_map[i]){
        case DATA_CARRIER:
            buffer_cell[idx_data] = cell;
            ++idx_data;
            break;
        case P2CARRIER_INVERTED:
        case P2CARRIER:
            if(pilot_refer < 0) {
                cell = -cell;
                pilot_refer = -pilot_refer;
            }
            est_dif = cell * conj(old_cell);
            old_cell = cell;
            est_pilot = cell * pilot_refer;
            sum_pilot_1 += est_pilot;
            sum_dif_1 += est_dif;
            angle = atan2_approx(est_pilot.imag(), est_pilot.real());
            delta_angle = (angle - angle_est) / (idx_data + 1);
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
            est_data[len_est].real(angle_est);
            est_data[len_est].imag(amp_est);
            ++len_est;
            // ...
            break;
        case  P2PAPR_CARRIER:
            //TODO
            //            printf(" P2PAPR_CARRIER\n");
            break;
        default:
            break;
        }
    }
    cell = ofdm_cell[half_total];
    pilot_refer = pilot_refer_idx_p2_symbol[half_total];
    if(pilot_refer < 0) {
        cell = -cell;
        pilot_refer = -pilot_refer;
    }
    est_pilot = cell * pilot_refer;
    angle = atan2_approx(est_pilot.imag(), est_pilot.real());
    amp = sqrt(norm(cell)) / amp_pilot;
    //Only for show
    est_data[len_est].real(angle);
    est_data[len_est].imag(amp);
    ++len_est;
    // ...
    for (int i = half_total + 1; i < k_total; ++i){
        cell = ofdm_cell[i];
        pilot_refer = pilot_refer_idx_p2_symbol[i];
        switch (p2_carrier_map[i]){
        case DATA_CARRIER:
            buffer_cell[idx_data] = cell;
            ++idx_data;
            break;
        case P2CARRIER_INVERTED:
        case P2CARRIER:
            if(pilot_refer < 0) {
                cell = -cell;
                pilot_refer = -pilot_refer;
            }
            est_dif = cell * conj(old_cell);
            old_cell = cell;
            est_pilot = cell * pilot_refer;
            sum_pilot_2 += est_pilot;
            sum_dif_2 += est_dif;
            angle = atan2_approx(est_pilot.imag(), est_pilot.real());
            delta_angle = (angle - angle_est) / (idx_data + 1);
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
            est_data[len_est].real(angle_est);
            est_data[len_est].imag(amp_est);
            ++len_est;
            // ...
            break;
        case  P2PAPR_CARRIER:
            //TODO
            //            printf(" P2PAPR_CARRIER\n");
            break;
        default:
            break;
        }
    }

    float ph_1 = atan2_approx(sum_pilot_1.imag(), sum_pilot_1.real());
    float ph_2 = atan2_approx(sum_pilot_2.imag(), sum_pilot_2.real());
    float dph_1 = atan2_approx(sum_dif_1.imag(), sum_dif_1.real());
    float dph_2 = atan2_approx(sum_dif_2.imag(), sum_dif_2.real());

    _phase_offset = (ph_2 + ph_1);
    _sample_rate_offset = (dph_2 + dph_1) / static_cast<float>(len_est);

    memcpy(show_symbol, _ofdm_cell, sizeof(complex) * static_cast<unsigned long>(fft_size));
    int len_show = L1_PRE_CELL;
    int idx_show = 0;
    if(_crc32_l1_pre){
        switch (id_show) {
        case 1:
            len_show = l1_pre.l1_post_size;
            idx_show = L1_PRE_CELL;
            break;
        case 2:
            len_show = 1024;
            if(len_show < c_p2) len_show = c_p2;
            idx_show = l1_pre.l1_post_size + L1_PRE_CELL;
            break;
        }
    }
    memcpy(show_data, deinterleaved_cell, sizeof(complex) * static_cast<unsigned long>(c_p2));
    emit replace_spectrograph(fft_size, &show_symbol[0]);
    emit replace_constelation(len_show, &show_data[idx_show]);
    memcpy(show_est_data, est_data, sizeof(complex) * static_cast<unsigned long>(len_est));
    emit replace_oscilloscope(len_est, show_est_data);

    if(_demod_init) {
        if(l1_post_info()) {
            _l1_post = l1_post;
            _crc32_l1_post = true;
            return deinterleaved_cell;
        }
    }
    else if(l1_pre_info(_dvbt2)) {
        _l1_pre = l1_pre;
        _crc32_l1_pre = true;
        if(l1_post_info()) {
            _l1_post = l1_post;
            _crc32_l1_post = true;
        }
        else{
            _crc32_l1_post = false;
        }
    }
    else{
        _crc32_l1_pre = false;
        _crc32_l1_post = false;
    }
    return deinterleaved_cell;
}
//----------------------------------------------------------------------------------------------------------------------------
bool p2_symbol::l1_pre_info(dvbt2_parameters &_dvbt2)
{
    unsigned int crc = 0xffffffff;
    int field = 0;
    unsigned int shift = 31;
    unsigned char bit;
    //calculate CRC32
    for(int i = 0; i < 168; ++i){
        //BPSK demodulate
        bit = deinterleaved_cell[i].real() > 0 ? 0 : 1;
        unsigned int b = bit ^ ((crc >> 31) & 0x01);
        crc <<= 1;
        if (b) crc ^= CRC_POLY;
    }
    //get field CRC32
    field = 0;
    for(int i = 168; i < 200; ++i){
        //BPSK demodulate
        bit = deinterleaved_cell[i].real() > 0 ? 0 : 1;
        //demap
        field |= (bit << shift);
        --shift;
    }
    l1_pre.crc_32 = field;
    //check CRC32
    if(crc != static_cast<unsigned int>(field)) {
        emit view_l1_presignalling("CRC_32 ERROR");

        return false;

    }

    field = 0;
    shift = 7;
    for(int i = 0; i < 168; ++i){
        //BPSK demodulate
        bit = deinterleaved_cell[i].real() > 0 ? 0 : 1;
        //demap bit
        field |= (bit << shift);
        --shift;
        switch (i){
        case 7:
            l1_pre.type = field;
            field = 0;
            shift = 0;
            break;
        case 8:
            l1_pre.bwt_ext = field;
            field = 0;
            shift = 2;
            break;
        case 11:
            l1_pre.s1 = field;
            field = 0;
            shift = 2;
            break;
        case 14:
            l1_pre.s2_field1 = field;
            field = 0;
            shift = 0;
            break;
        case 15:
            l1_pre.s2_field2 = field;
            idx_l1_post_fef_shift = field * 34;
            field = 0;
            shift = 0;
            break;
        case 16:
            l1_pre.l1_repetition_flag = field;
            field = 0;
            shift = 2;
            break;
        case 19:
            l1_pre.guard_interval = field;
            field = 0;
            shift = 3;
            break;
        case 23:
            l1_pre.papr = field;
            field = 0;
            shift = 3;
            break;
        case 27:
            l1_pre.l1_post_mod = field;
            field = 0;
            shift = 1;
            break;
        case 29:
            l1_pre.l1_cod = field;
            field = 0;
            shift = 1;
            break;
        case 31:
            l1_pre.l1_fec_type = field;
            field = 0;
            shift = 17;
            break;
        case 49:
            l1_pre.l1_post_size = field;
            l1_size = field + L1_PRE_CELL;
            field = 0;
            shift = 17;
            break;
        case 67:
            l1_pre.l1_post_info_size = field;
            if(l1_post_bit == nullptr) l1_post_bit =
                    new unsigned char[l1_pre.l1_post_size * l1_pre.l1_post_mod * 2];
            if(l1_post_bit_interleaving == nullptr) l1_post_bit_interleaving =
                    new unsigned char[l1_pre.l1_post_size * l1_pre.l1_post_mod * 2];
            field = 0;
            shift = 3;
            break;
        case 71:
            l1_pre.pilot_pattern = field;
            field = 0;
            shift = 7;
            break;
        case 79:
            l1_pre.tx_id_availability = field;
            field = 0;
            shift = 15;
            break;
        case 95:
            l1_pre.cell_id = field;
            field = 0;
            shift = 15;
            break;
        case 111:
            l1_pre.network_id = field;
            field = 0;
            shift = 15;
            break;
        case 127:
            l1_pre.t2_system_id = field;
            field = 0;
            shift = 7;
            break;
        case 135:
            l1_pre.num_t2_frames = field;
            field = 0;
            shift = 11;
            break;
        case 147:
            l1_pre.num_data_symbols = field;
            field = 0;
            shift = 2;
            break;
        case 150:
            l1_pre.regen_flag = field;
            field = 0;
            shift = 0;
            break;
        case 151:
            l1_pre.l1_post_extension = field;
            field = 0;
            shift = 2;
            break;
        case 154:
            l1_pre.num_rf = field;
            if(l1_post.rf == nullptr) l1_post.rf = new l1_postsignalling_rf[field];
            idx_l1_post_rf_shift = (field - 1) * 35;
            field = 0;
            shift = 2;
            break;
        case 157:
            l1_pre.current_rf_index = field;
            field = 0;
            shift = 3;
            break;
        case 161:
            l1_pre.t2_version = field;
            field = 0;
            shift = 0;
            break;
        case 162:
            l1_pre.l1_post_scrambled = field;
            field = 0;
            shift = 0;
            break;
        case 163:
            l1_pre.t2_base_lite = field;
            field = 0;
            shift = 3;
            break;
        case 167:
            l1_pre.reserved = field;
            break;
        default:
            break;
        }
    }

    if(_dvbt2.carrier_mode != l1_pre.bwt_ext){
        _dvbt2.carrier_mode = l1_pre.bwt_ext;
        dvbt2_bwt_ext_parameters_init(_dvbt2);
    }
    _dvbt2.guard_interval_mode = l1_pre.guard_interval;
    _dvbt2.papr_mode = l1_pre.papr;
    _dvbt2.pilot_pattern = l1_pre.pilot_pattern;
    _dvbt2.n_data = l1_pre.num_data_symbols;

    emit view_l1_presignalling("TYPE\t\t" + QString::number(l1_pre.type) + "\n"
                               + "BWT_EXT\t\t" + QString::number(l1_pre.bwt_ext) +  "\n"
                               + "S1\t\t" + QString::number(l1_pre.s1) +  "\n"
                               + "S2_FIELD1\t\t" + QString::number(l1_pre.s2_field1) +  "\n"
                               + "S2_FIELD2\t\t" + QString::number(l1_pre.s2_field2) +  "\n"
                               + "L1_REPETITION_FLAG " + QString::number(l1_pre.l1_repetition_flag) +  "\n"
                               + "GUARD_INTERVAL\t" + QString::number(l1_pre.guard_interval) +  "\n"
                               + "PAPR\t\t" + QString::number(l1_pre.papr) +  "\n"
                               + "L1_POST_MOD\t" + QString::number(l1_pre.l1_post_mod) +  "\n"
                               + "L1_COD\t\t" + QString::number(l1_pre.l1_cod) +  "\n"
                               + "L1_FEC_TYPE\t" + QString::number(l1_pre.l1_fec_type) +  "\n"
                               + "L1_POST_SIZE\t" + QString::number(l1_pre.l1_post_size) +  "\n"
                               + "L1_POST_INFO_SIZE  " + QString::number(l1_pre.l1_post_info_size) +  "\n"
                               + "PILOT_PATTERN\t" + QString::number(l1_pre.pilot_pattern) +  "\n"
                               + "TX_ID_AVAILABILITY  " + QString::number(l1_pre.tx_id_availability) +  "\n"
                               + "CELL_ID\t\t" + QString::number(l1_pre.cell_id) +  "\n"
                               + "NETWORK_ID\t" + QString::number(l1_pre.network_id) +  "\n"
                               + "T2_SYSTEM_ID\t" + QString::number(l1_pre.t2_system_id) +  "\n"
                               + "NUM_T2_FRAMES\t" + QString::number(l1_pre.num_t2_frames) +  "\n"
                               + "NUM_DATA_SYMBOLS " + QString::number(l1_pre.num_data_symbols) +  "\n"
                               + "REGEN_FLAG\t" + QString::number(l1_pre.regen_flag) +  "\n"
                               + "L1_POST_EXTENSION " + QString::number(l1_pre.l1_post_extension) +  "\n"
                               + "NUM_RF\t\t" + QString::number(l1_pre.num_rf) +  "\n"
                               + "CURRENT_RF_IDX " + QString::number(l1_pre.current_rf_index) +  "\n"
                               + "T2_VERSION\t" + QString::number(l1_pre.t2_version) +  "\n"
                               + "L1_POST_SCRAMBLED " + QString::number(l1_pre.l1_post_scrambled) +  "\n"
                               + "T2_BASE_LITE\t" + QString::number(l1_pre.t2_base_lite) +  "\n"
                               + "RESERVED\t\t" + QString::number(l1_pre.reserved) +  "\n");

    return true;
}
//----------------------------------------------------------------------------------------------------------------------------
bool p2_symbol::l1_post_info()
{
    unsigned int crc = 0xffffffff;
    complex* p2_l1_post = &deinterleaved_cell[L1_PRE_CELL];

    float amp2 = 0;
    float amp4 = 0;
    int n_bit = 0;
    unsigned char bit = 0;
    int n_post = 0;
    int rows = 0;
    int colums = 0;
    int size_block;
    bool randomize;
    int step = 0;
    int l = 0;
    const int* mux = nullptr;
    int m[1] = {0};
    int idx_mux = 0;
    int w = 0;
    int substreams = 1;
    switch (l1_pre.l1_post_mod) {
    case 0:
        n_bit = 0;
        n_post = l1_pre.l1_post_size;
        colums = 0;
        rows = 0;
        mux = m;
        break;
    case 1:
        n_bit = 1;
        n_post = l1_pre.l1_post_size * 2;
        colums = 0;
        rows = 0;
        mux = m;
        break;
    case 2:
        amp4 = NORM_FACTOR_QAM16 * 2;
        n_bit = 3;
        n_post = l1_pre.l1_post_size * 4;
        colums = 8;
        rows = n_post / colums;
        mux = mux16;
        substreams = 8;
        break;
    case 3:
        amp2 = NORM_FACTOR_QAM64 * 2;
        amp4 = NORM_FACTOR_QAM64 * 4;
        n_bit = 5;
        n_post = l1_pre.l1_post_size * 6;
        colums = 12;
        rows = n_post / colums;
        mux = mux64;
        substreams = 12;
        break;
    default:
        break;
    }
    int c_bit = n_bit;
    float real = (*p2_l1_post).real();
    float imag = (*p2_l1_post).imag();
    for(int i = 0; i < n_post; ++i){
        //demodulate
        switch(n_bit - c_bit){
        case 0:            
            bit = real > 0 ? 0 : 1;
            break;
        case 1:            
            bit = imag > 0 ? 0 : 1;
            break;
        case 2:            
            bit = std::abs(real) > amp4 ? 0 : 1;
            break;
        case 3:            
            bit = std::abs(imag) > amp4 ? 0 : 1;
            break;
        case 4:
            bit = std::abs(std::abs(real) - amp4) >  amp2 ? 0 : 1;
            break;
        case 5:
            bit = std::abs(std::abs(imag) - amp4) >  amp2 ? 0 : 1;
            break;
        default:
            break;
        }
        //multiplexer
        l1_post_bit_interleaving[mux[idx_mux] + w] = bit;
        ++idx_mux;
        if(idx_mux == substreams) {
            idx_mux = 0;
            w += substreams;
        }
        if(c_bit == 0){
            c_bit = n_bit + 1;
            ++p2_l1_post;
            real = (*p2_l1_post).real();
            imag = (*p2_l1_post).imag();
        }
        --c_bit;
    }
    //deinterleaving
    size_block = rows * colums;
    randomize = l1_pre.t2_version > 1 && l1_pre.l1_post_scrambled == TRUE;
    for (int i = 0; i < n_post; ++i) {
        int j = l + step;
        l1_post_bit[j] = l1_post_bit_interleaving[i];
        if (randomize) l1_post_bit[j] ^= l1_randomize[j];
        step += rows;
        if(step == size_block) {
            step = 0;
            ++l;
        }
    }
    //calculate CRC32
    for (int i = 0; i < l1_pre.l1_post_info_size; ++i) {
        unsigned int b = l1_post_bit[i] ^ ((crc >> 31) & 0x01);
        crc <<= 1;
        if (b) crc ^= CRC_POLY;
    }
    //get field CRC32
    int idx = l1_pre.l1_post_info_size;
    int field = 0;
    for(int s = 31; s >= 0 ; --s){
        field |= (l1_post_bit[idx++] << s);
    }
    //check CRC32
    if(static_cast<unsigned int>(field) != crc){
        emit view_l1_postsignalling("CRC_32 ERROR");
        emit view_l1_dynamic("CRC_32 ERROR", true);
        view_l1_post_update = true;
        chek_l1_post = false;

        return false;

    }

    chek_l1_post = true;
    idx = 15;
    l1_post.num_plp = 0;
    for(int s = 7; s >= 0 ; --s){
        l1_post.num_plp |= l1_post_bit[idx++] << s;
    }
    if(l1_post.plp == nullptr) l1_post.plp = new l1_postsignalling_plp[l1_post.num_plp];
    idx_l1_post_plp_shift = (l1_post.num_plp - 1) * 89;
    idx = 23;
    l1_post.num_aux = 0;
    for(int s = 3; s >= 0 ; --s){
        l1_post.num_aux |= l1_post_bit[idx++] << s;
    }
    if(l1_post.aux == nullptr) l1_post.aux = new l1_postsignalling_aux[l1_post.num_aux];
    idx_l1_post_aux_shift = (l1_post.num_aux - 1) * 32;
    // TODO check l1_post.num_aux != 0
//    if(l1_post.dyn.aux_private_dyn == nullptr) l1_post.dyn.aux_private_dyn = new int[l1_post.num_aux];
    idx_l1_post_dyn_aux_shift = (l1_post.num_aux - 1) * 48;
    idx_l1_post_configurable_shift = idx_l1_post_rf_shift + idx_l1_post_fef_shift +
                                        idx_l1_post_plp_shift + idx_l1_post_aux_shift + 223;
    if(l1_post.dyn.plp == nullptr) l1_post.dyn.plp = new dynamic_plp[l1_post.num_plp];
    idx_l1_post_dyn_plp_shift = (l1_post.num_plp - 1) * 48;
    idx_l1_post_dyn_shift = idx_l1_post_configurable_shift + idx_l1_post_dyn_plp_shift +
                            idx_l1_post_dyn_aux_shift + 71;

    text_l1_post = "";
    time_frequency_slicing_info(l1_post_bit, l1_post);
    plp_info(l1_post_bit, l1_post);
    rf_info(l1_post_bit, l1_post);
    fef_info(l1_post_bit, l1_post);
    aux_info(l1_post_bit, l1_post);
    if(view_l1_post_update) {
        emit view_l1_postsignalling(text_l1_post);
        view_l1_post_update = false;
    }
    text_l1_dynamic = "";
    dyn_info(l1_post_bit, l1_post);
    dyn_plp_info(l1_post_bit, l1_post);
    dyn_aux_info(l1_post_bit, l1_post);
    if(l1_pre.l1_repetition_flag){
        dyn_next_info(l1_post_bit, l1_post);
        dyn_next_plp_info(l1_post_bit, l1_post);
        dyn_next_aux_info(l1_post_bit, l1_post);
    }
    emit view_l1_dynamic(text_l1_dynamic, false);

    return true;

}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::time_frequency_slicing_info(unsigned char *bit, l1_postsignalling &l1)
{
    int idx = 0;
    l1.sub_slices_per_frame = 0;
    for(int s = 14; s >= 0 ; --s){
        l1.sub_slices_per_frame |= bit[idx++] << s;
    }
    text_l1_post += "SUB_SLISER_PER_FRAME  " +
                                QString::number(l1_post.sub_slices_per_frame) + "\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::plp_info(unsigned char *bit, l1_postsignalling &l1)
{
    text_l1_post += "NUM_PLP\t\t" + QString::number(l1_post.num_plp) + "\n";
    int idx = idx_l1_post_rf_shift + idx_l1_post_fef_shift + 70;
    for(int i = 0; i < l1.num_plp; i++){
        l1.plp[i].id = 0;
        for(int s = 7; s >= 0 ; --s){
            l1.plp[i].id |= bit[idx++] << s;
        }
        l1.plp[i].plp_type = 0;
        for(int s = 2; s >= 0 ; --s){
            l1.plp[i].plp_type |= bit[idx++] << s;
        }
        l1.plp[i].plp_payload_type = 0;
        for(int s = 4; s >= 0 ; --s){
            l1.plp[i].plp_payload_type |= bit[idx++] << s;
        }
        l1.plp[i].ff_flag = bit[idx++];
        l1.plp[i].first_rf_idx = 0;
        for(int s = 2; s >= 0 ; --s){
            l1.plp[i].first_rf_idx |= bit[idx++] << s;
        }
        l1.plp[i].first_frame_idx  = 0;
        for(int s = 7; s >= 0 ; --s){
            l1.plp[i].first_frame_idx |= bit[idx++] << s;
        }
        l1.plp[i].plp_group_id = 0;
        for(int s = 7; s >= 0 ; --s){
            l1.plp[i].plp_group_id |= bit[idx++] << s;
        }
        l1.plp[i].plp_cod  = 0;
        for(int s = 2; s >= 0 ; --s){
            l1.plp[i].plp_cod |= bit[idx++] << s;
        }
        l1.plp[i].plp_mod  = 0;
        for(int s = 2; s >= 0 ; --s){
            l1.plp[i].plp_mod |= bit[idx++] << s;
        }
        l1.plp[i].plp_rotation = bit[idx++];
        l1.plp[i].plp_fec_type = 0;
        for(int s = 1; s >= 0 ; --s){
            l1.plp[i].plp_fec_type |= bit[idx++] << s;
        }
        l1.plp[i].plp_num_blocks_max = 0;
        for(int s = 9; s >= 0 ; --s){
            l1.plp[i].plp_num_blocks_max |= bit[idx++] << s;
        }
        l1.plp[i].frame_interval = 0;
        for(int s = 7; s >= 0 ; --s){
            l1.plp[i].frame_interval |= bit[idx++] << s;
        }
        l1.plp[i].time_il_length = 0;
        for(int s = 7; s >= 0 ; --s){
            l1.plp[i].time_il_length |= bit[idx++] << s;
        }
        l1.plp[i].time_il_type = bit[idx++];
        l1.plp[i].in_band_a_flag = bit[idx++];
        l1.plp[i].in_band_b_flag = bit[idx++];
        l1.plp[i].reserved_1 = 0;
        for(int s = 10; s >= 0 ; --s){
            l1.plp[i].reserved_1 |= bit[idx++] << s;
        }
        l1.plp[i].plp_mode = 0;
        for(int s = 1; s >= 0 ; --s){
            l1.plp[i].plp_mode |= bit[idx++] << s;
        }
        l1.plp[i].static_flag = bit[idx++];
        l1.plp[i].static_padding_flag = bit[idx++];
        text_l1_post += QString::number(i) +
              "  PLP_ID\t\t" + QString::number(l1_post.plp[i].id) + "\n"
              + QString::number(i) +
              "  PLP_TYPE\t" + QString::number(l1_post.plp[i].plp_type) + "\n"
              + QString::number(i) +
              "  PLP_PAYLOAD_TYPE  " + QString::number(l1_post.plp[i].plp_payload_type) + "\n"
              + QString::number(i) +
              "  FF_FLAG\t\t" + QString::number(l1_post.plp[i].ff_flag) + "\n"
              + QString::number(i) +
              "  FIRST_RF_IDX\t" + QString::number(l1_post.plp[i].first_rf_idx) + "\n"
              + QString::number(i) +
              "  FIRST_FRAME_IDX\t" + QString::number(l1_post.plp[i].first_frame_idx) + "\n"
              + QString::number(i) +
              "  PLP_GROUP_ID\t" + QString::number(l1_post.plp[i].plp_group_id) + "\n"
              + QString::number(i) +
              "  PLP_COD\t" + QString::number(l1_post.plp[i].plp_cod) + "\n"
              + QString::number(i) +
              "  PLP_POTATION\t" + QString::number(l1_post.plp[i].plp_rotation) + "\n"
              + QString::number(i) +
              "  PLP_FEC_TYPE\t" + QString::number(l1_post.plp[i].plp_fec_type) + "\n"
              + QString::number(i) +
              "  PLP_NUM_BLOCKS_MAX " + QString::number(l1_post.plp[i].plp_num_blocks_max) + "\n"
              + QString::number(i) +
              "  FRAME_INTERVAL\t" + QString::number(l1_post.plp[i].frame_interval) + "\n"
              + QString::number(i) +
              "  TIME_IL_LENGTH\t" + QString::number(l1_post.plp[i].time_il_length) + "\n"
              + QString::number(i) +
              "  TIME_IL_TYPE\t" + QString::number(l1_post.plp[i].time_il_type) + "\n"
              + QString::number(i) +
              "  IN_BAND_A_FLAG\t" + QString::number(l1_post.plp[i].in_band_a_flag) + "\n"
              + QString::number(i) +
              "  IN_BAND_B_FLAG\t" + QString::number(l1_post.plp[i].in_band_b_flag) + "\n"
              + QString::number(i) +
              "  RESERVED_1\t" + QString::number(l1_post.plp[i].reserved_1) + "\n"
              + QString::number(i) +
              "  PLP_MODE\t" + QString::number(l1_post.plp[i].plp_mode) + "\n"
              + QString::number(i) +
              "  STATIC_FLAG\t" + QString::number(l1_post.plp[i].static_flag) + "\n"
              + QString::number(i) +
              "  STATIC_PADDING_FLAG " + QString::number(l1_post.plp[i].static_padding_flag) + "\n";
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::rf_info(unsigned char *bit, l1_postsignalling &l1)
{
    int idx = 35;
    for(int i = 0; i < l1_pre.num_rf; i++){
        l1.rf[i].rf_idx = 0;
        for(int s = 2; s >= 0 ; --s){
            l1.rf[i].rf_idx |= (bit[idx++] << s);
        }
        l1.rf[i].frequency = 0;
        for(int s = 31; s >= 0 ; --s){
            l1.rf[i].frequency |= (bit[idx++] << s);
        }
        text_l1_post += QString::number(i) +
              "  RF_IDX\t\t" + QString::number(l1_post.rf[i].rf_idx) + "\n"
              + QString::number(i) +
              "  FREQUENCY\t" + QString::number(l1_post.rf[i].frequency) + "\n";
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::fef_info(unsigned char *bit, l1_postsignalling &l1)
{
    int idx;
    if(l1_pre.s2_field2 == TRUE){
        idx = idx_l1_post_rf_shift + 70;
        l1.fef_type = 0;
        for(int s = 4; s >= 0 ; --s){
            l1.fef_type |= bit[idx++] << s;
        }
        l1.fef_length = 0;
        for(int s = 21; s >= 0 ; --s){
            l1.fef_length |= bit[idx++] << s;
        }
        l1.fef_interval = 0;
        for(int s = 7; s >= 0 ; --s){
            l1.fef_interval |= bit[idx++] << s;
        }
    }
    idx = idx_l1_post_rf_shift + idx_l1_post_fef_shift + idx_l1_post_plp_shift + 169;
    l1.fef_length_msb = 0;
    for(int s = 1; s >= 0 ; --s){
        l1.fef_length_msb |= bit[idx++] << s;
    }
    l1.reserved_2 = 0;
    for(int s = 1; s >= 0 ; --s){
        l1.reserved_2 |= bit[idx++] << s;
    }
    text_l1_post += "FEF_TYPE\t\t" + QString::number(l1_post.fef_type) + "\n"
          + "FEF_LENGHT\t" + QString::number(l1_post.fef_length) + "\n"
          + "FEF_INTERVAL\t" + QString::number(l1_post.fef_interval) + "\n"
          + "FEF_LENGHT_MSB\t" + QString::number(l1_post.fef_length_msb) + "\n"
          + "RESERVED_2\t" + QString::number(l1_post.reserved_2) + "\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::aux_info(unsigned char *bit, l1_postsignalling &l1)
{
    int idx = 27;
    l1.aux_config_rfu = 0;
    for(int s = 7; s >= 0 ; --s){
        l1.aux_config_rfu |= bit[idx++] << s;
    }
    text_l1_post += "NUM_AUX\t\t" + QString::number(l1_post.num_aux) + "\n"
          + "AUX_CONFIG_RFU\t" + QString::number(l1_post.aux_config_rfu) + "\n";

    idx = idx_l1_post_rf_shift + idx_l1_post_fef_shift + idx_l1_post_plp_shift + 191;
    for(int i = 0; i < l1.num_aux; i++){
        l1.aux[i].aux_stream_type = 0;
        for(int s = 3; s >= 0 ; --s){
            l1.aux[i].aux_stream_type |= bit[idx++] << s;
        }
        l1.aux[i].aux_private_config = 0;
        for(int s = 27; s >= 0 ; --s){
            l1.aux[i].aux_private_config |= bit[idx++] << s;
        }
        text_l1_post += QString::number(i) +
              "  AUX_STREAM_TYPE  " + QString::number(l1_post.aux[i].aux_stream_type) + "\n"
              + QString::number(i) +
              "  AUX_PRIVATE_CONFIG " + QString::number(l1_post.aux[i].aux_private_config) + "\n";
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::dyn_info(unsigned char *bit, l1_postsignalling &l1)
{
    int idx = idx_l1_post_configurable_shift;
    l1.dyn.frame_idx = 0;
    for(int s = 7; s >= 0; --s){
        l1.dyn.frame_idx |= (bit[idx++] << s);
    }
    l1.dyn.sub_slice_interval = 0;
    for(int s = 21; s >= 0; --s){
        l1.dyn.sub_slice_interval |= (bit[idx++] << s);
    }
    l1.dyn.type_2_start = 0;
    for(int s = 21; s >= 0; --s){
        l1.dyn.type_2_start |= (bit[idx++] << s);
    }
    l1.dyn.l1_change_counter = 0;
    for(int s = 7; s >= 0; --s){
        l1.dyn.l1_change_counter |= (bit[idx++] << s);
    }
    l1.dyn.start_rf_idx = 0;
    for(int s = 2; s >= 0; --s){
        l1.dyn.start_rf_idx |= (bit[idx++] << s);
    }
    l1.dyn.reserved_1 = 0;
    for(int s = 7; s >= 0; --s){
        l1.dyn.reserved_1 |= (bit[idx++] << s);
    }
    text_l1_dynamic += "DYN_FRAME_IDX\t" + QString::number(l1_post.dyn.frame_idx) + "\n"
          + "DYN_SUBSLICE_INTERVAL " + QString::number(l1_post.dyn.sub_slice_interval) + "\n"
          + "DYN_TYPE_2_START\t" + QString::number(l1_post.dyn.type_2_start) + "\n"
          + "DYN_L1_CHANGE_COUNTER " + QString::number(l1_post.dyn.l1_change_counter) + "\n"
          + "DYN_START_RF_IDX\t" + QString::number(l1_post.dyn.start_rf_idx) + "\n"
          + "DYN_RESERVED_1\t" + QString::number(l1_post.dyn.reserved_1) + "\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::dyn_plp_info(unsigned char *bit, l1_postsignalling &l1)
{
    int idx = idx_l1_post_configurable_shift + 71;
    for(int i = 0; i < l1_post.num_plp; i++){
        l1.dyn.plp[i].id = 0;
        for(int s = 7; s >= 0; --s){
            l1.dyn.plp[i].id |= (bit[idx++] << s);
        }
        l1.dyn.plp[i].start = 0;
        for(int s = 21; s >= 0; --s){
            l1.dyn.plp[i].start |= (bit[idx++] << s);
        }
        l1.dyn.plp[i].num_blocks = 0;
        for(int s = 9; s >= 0; --s){
            l1.dyn.plp[i].num_blocks |= (bit[idx++] << s);
        }
        l1.dyn.plp[i].reserved_2 = 0;
        for(int s = 7; s >= 0; --s){
            l1.dyn.plp[i].reserved_2 |= (bit[idx++] << s);
        }
        text_l1_dynamic +=QString::number(i) +
              "  DYN_PLP_ID\t" + QString::number(l1_post.dyn.plp[i].id) + "\n"
              + QString::number(i) +
              "  DYN_PLP_START\t" + QString::number(l1_post.dyn.plp[i].start) + "\n"
              + QString::number(i) +
              "  DYN_PLP_NUM_BLOCKS " + QString::number(l1_post.dyn.plp[i].num_blocks) + "\n"
              + QString::number(i) +
              "  DYN_RESERVED_2\t" + QString::number(l1_post.dyn.plp[i].reserved_2) + "\n";
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::dyn_aux_info(unsigned char *bit, l1_postsignalling &l1)
{
    int idx = idx_l1_post_configurable_shift + idx_l1_post_dyn_plp_shift + 119;
    l1.dyn_next.reserved_3 = 0;
    for(int s = 7; s >= 0; --s){
        l1.dyn.reserved_3 = (bit[idx++] << s);
    }
    for(int i = 0; i < l1.num_aux; i++){
        for(int s = 47; s >=0; --s){
            l1.dyn.aux_private_dyn[i] |= (bit[idx++] << s);
        }
        text_l1_dynamic +=QString::number(i) +
              "  DYN_AUX_PRIVATE_DYN " + QString::number(l1_post.dyn.aux_private_dyn[i]) + "\n";
    }
    text_l1_dynamic += "DYN_RESERVED_3\t" + QString::number(l1_post.dyn.reserved_3) + "\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::dyn_next_info(unsigned char *bit, l1_postsignalling &l1)
{
    int idx = idx_l1_post_dyn_shift;
    l1.dyn_next.frame_idx = 0;
    for(int s = 7; s >= 0; --s){
        l1.dyn_next.frame_idx |= (bit[idx++] << s);
    }
    l1.dyn_next.sub_slice_interval = 0;
    for(int s = 21; s >= 0; --s){
        l1.dyn_next.sub_slice_interval |= (bit[idx++] << s);
    }
    l1.dyn_next.type_2_start = 0;
    for(int s = 21; s >= 0; --s){
        l1.dyn_next.type_2_start |= (bit[idx++] << s);
    }
    l1.dyn_next.l1_change_counter = 0;
    for(int s = 7; s >= 0; --s){
        l1.dyn_next.l1_change_counter |= (bit[idx++] << s);
    }
    l1.dyn_next.start_rf_idx = 0;
    for(int s = 2; s >= 0; --s){
        l1.dyn.start_rf_idx |= (bit[idx++] << s);
    }
    l1.dyn_next.reserved_1 = 0;
    for(int s = 7; s >= 0; --s){
        l1.dyn_next.reserved_1 |= (bit[idx++] << s);
    }
    text_l1_dynamic += "DYN_NEXT_FRAME_IDX  " + QString::number(l1_post.dyn_next.frame_idx) + "\n"
          + "DYN_NEXT_SUBSLICE_INTERVAL " + QString::number(l1_post.dyn_next.sub_slice_interval) + "\n"
          + "DYN_NEXT_TYPE_2_START " + QString::number(l1_post.dyn_next.type_2_start) + "\n"
          + "DYN_NEXT_L1_CHANGE_COUNTER " + QString::number(l1_post.dyn_next.l1_change_counter) + "\n"
          + "DYN_NEXT_START_RF_IDX " + QString::number(l1_post.dyn_next.start_rf_idx) + "\n"
          + "DYN_NEXT_RESERVED_1 " + QString::number(l1_post.dyn_next.reserved_1) + "\n";
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::dyn_next_plp_info(unsigned char *bit, l1_postsignalling &l1)
{
    int idx = idx_l1_post_dyn_shift + 71;
    for(int i = 0; i < l1_post.num_plp; i++){
        l1.dyn_next.plp[i].id = 0;
        for(int s = 7; s >= 0; --s){
            l1.dyn_next.plp[i].id |= (bit[idx++] << s);
        }
        l1.dyn_next.plp[i].start = 0;
        for(int s = 21; s >= 0; --s){
            l1.dyn_next.plp[i].start |= (bit[idx++] << s);
        }
        l1.dyn_next.plp[i].num_blocks = 0;
        for(int s = 9; s >= 0; --s){
            l1.dyn.plp[i].num_blocks |= (bit[idx++] << s);
        }
        l1.dyn_next.plp[i].reserved_2 = 0;
        for(int s = 7; s >= 0; --s){
            l1.dyn_next.plp[i].reserved_2 |= (bit[idx++] << s);
        }
        text_l1_dynamic += QString::number(i) +
              "  DYN_NEXT_PLP_ID\t" + QString::number(l1_post.dyn_next.plp[i].id) + "\n"
              + QString::number(i) +
              "  DYN_NEXT_PLP_START " + QString::number(l1_post.dyn_next.plp[i].start) + "\n"
              + QString::number(i) +
              "  DYN_NEXT_PLP_NUM_BLOCKS " + QString::number(l1_post.dyn_next.plp[i].num_blocks) + "\n"
              + QString::number(i) +
              "  DYN_NEXT_RESERVED_2 " + QString::number(l1_post.dyn_next.plp[i].reserved_2) + "\n";
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void p2_symbol::dyn_next_aux_info(unsigned char *bit, l1_postsignalling &l1)
{
    int idx = idx_l1_post_dyn_shift + 119;
    l1.dyn_next.reserved_3 = 0;
    for(int s = 7; s >= 0; --s){
        l1.dyn_next.reserved_3 = (bit[idx++] << s);
    }
    for(int i = 0; i < l1.num_aux; i++){
        for(int s = 47; s >=0; --s){
            l1.dyn_next.aux_private_dyn[i] |= (bit[idx++] << s);
        }
        text_l1_dynamic += QString::number(i) +
              "  DYN_NEXT_AUX_PRIVATE_DYN " + QString::number(l1_post.dyn_next.aux_private_dyn[i]) + "\n";
    }
    text_l1_dynamic += "DYN_NEXT_RESERVED_3 " + QString::number(l1_post.dyn_next.reserved_3) + "\n";
}
//----------------------------------------------------------------------------------------------------------------------------
