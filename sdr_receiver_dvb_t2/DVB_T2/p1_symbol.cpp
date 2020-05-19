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
#include "p1_symbol.h"

#include "DSP/fast_math.h"

#define P1_HERTZ_PER_RADIAN HERTZ_PER_RADIAN / (P1_LEN << 1)

//-----------------------------------------------------------------------------------------------
p1_symbol::p1_symbol(QObject *parent) : QObject(parent)
{
    const float angle_shift = M_PI_X_2 / static_cast<float>(P1_FREQUENCY_SHIFT);
    float angle = 0.0;
    for(int i = 0; i < P1_FREQUENCY_SHIFT; i++) {
        fq_shift[i].real(sin(angle));
        fq_shift[i].imag(cos(angle));
        angle += angle_shift;
    }

    fft = new fast_fourier_transform;
    in_fft = fft->init(P1_A_PART);

    init_p1_randomize();
}
//-----------------------------------------------------------------------------------------------
p1_symbol::~p1_symbol()
{
    delete fft;
}
//-----------------------------------------------------------------------------------------------
void p1_symbol::init_p1_randomize()
{
    int sr = 0x4e46;
    for (int i = 0; i < 384; ++i) {
        int b = ((sr) ^ (sr >> 1)) & 1;
        if (b == 0) p1_randomize[i] = 1;
        else p1_randomize[i] = -1;
        sr >>= 1;
        if(b > 0) sr |= 0x4000;
    }
}
//-----------------------------------------------------------------------------------------------
/*         chain block diagram
 * -->__________________________
 *     |     __________         |     __________________    ___________
 *     |    |          |       _|_   |                  |  |           |
 *     |    | delay Tc |____*_/ x \__| run average Tc   |__| delay 2Tb |_______
 *     |    |__________|      \___/  |__________________|  |___________|       |
 *     |         _|_                                                           |
 *     |________/ x \___________                                               |
 *     |        \___/           |                                              |
 *     |     _____|__________   |                                              |
 *     |    |                |  |                                              |
 *     |    |exp(-j2pi*fsh*t)|  |                                              |
 *     |    |________________|  |                                              |
 *     |     __________         |     __________________     __________        |
 *     |    |          |       _|_   |                  |   |          |      _|_      arg max    - time estimate
 *     |____| delay Tb |____*_/ x \__| run average Tb   |___| delay 2  |_____/ x \_--> angle(max) - phase estimate
 *          |__________|      \___/  |__________________|   |__________|     \___/
 */
bool p1_symbol::execute(const int _len_in, complex *_in, int &_consume, complex* _buffer_sym, int &_idx_buffer_sym,
                        dvbt2_parameters &_dvbt2, float &_coarse_freq_offset, bool &_p1_decoded)
{
    static int idx_fq_shift = 0;
    complex s_sihft, a, b, c, d, in_av_c, in_av_b, out_av_c, out_av_b, cor, out;
    complex* buffer_sym = _buffer_sym;
    int idx_in = _consume;
    int len_in = _len_in;
    complex* in = _in;
    bool p1_detect = false;
    while(idx_in < len_in) {
        p1_buffer.write(in[idx_in]);
        ++len_p1;
        if(correlation_detect) {
            buffer_sym[idx_buffer++] = in[idx_in];
            if(correlation < end_threshold) {
                correlation_detect = false;
                len_p1 = 0;
                max_correlation = 0.0f;
                _idx_buffer_sym = idx_buffer;

                if(check_len_p1 < P1_LEN - 10) {
                    if(!signal_shut) {
                        signal_shut = true;
                        emit bad_signal();
                    }
                }
                else {
                    p1_detect =  true;
                    memcpy(in_fft, &p1_buffer.read()[P1_C_PART - idx_buffer],
                           sizeof(complex) * static_cast<unsigned int>(P1_A_PART));
                    p1_fft = fft->execute();
                    float coarse_freq_offset = atan2_approx(arg_max.imag(), arg_max.real()) * P1_HERTZ_PER_RADIAN;
                    if(!p1_decoded) {
                        for(int shift = 76; shift < 96; ++shift) { // +- 90kHz (one shift +-8928,5Hz)
                            complex* p1= p1_fft + shift;
                            if(demodulate(p1, _dvbt2)) {
                                p1_decoded = true;
                                if(shift != first_active_carrier) {
                                    coarse_freq_offset += (shift - first_active_carrier) * P1_CARRIER_SPASING;
                                }
                            }
                        }
                    }
                    _p1_decoded = p1_decoded;
                    _coarse_freq_offset = coarse_freq_offset;

                    cor_os = cor_buffer.read();
                    cor_os[0].imag(_coarse_freq_offset);
                    for(int i = 0; i < P1_ACTIVE_CARRIERS; ++i) {
                        p1_dbpsk[i] = (p1_fft + first_active_carrier)[p1_active_carriers[i]] * 0.1f;
                    }
                    emit replace_spectrograph(P1_A_PART, p1_fft);
                    emit replace_constelation(P1_ACTIVE_CARRIERS, p1_dbpsk);
                    emit replace_oscilloscope(P1_A_PART, cor_os);
                }
                idx_buffer = 0;
                delay_c.reset();
                delay_b.reset();
                delay_b_x2.reset();
                delay_2.reset();
                //                average_b.reset();
                //                average_c.reset();
                ++idx_in;

                break;
            }
        }

        s_sihft = in[idx_in] * fq_shift[idx_fq_shift];
        ++idx_fq_shift;
        idx_fq_shift &= 0x3FF;
        c = delay_c(s_sihft);
        in_av_c = in[idx_in] * conj(c);
        b = delay_b(in[idx_in]);
        in_av_b = s_sihft * conj(b);
        out_av_c = average_c(in_av_c);
        out_av_b = average_b(in_av_b);
        a = delay_b_x2(out_av_c);
        d = delay_2(out_av_b);
        out = a * d;
        correlation = norm(out);
        cor.real(correlation);
        cor.imag(begin_threshold);
        cor_buffer.write(cor);

        if(correlation > begin_threshold) {
            correlation_detect = true;
            if(correlation > max_correlation) {
                max_correlation = correlation;
                arg_max = out;
                idx_buffer = 0;
                check_len_p1 = len_p1;
            }
        }

        ++idx_in;
    }
    _consume = idx_in;
    return p1_detect;
}
//-----------------------------------------------------------------------------------------------
bool p1_symbol::demodulate(complex* _p1, dvbt2_parameters &_dvbt2)
{
    complex dbpsk[P1_ACTIVE_CARRIERS];
    for(int i = 0; i < P1_ACTIVE_CARRIERS; ++i) {
        dbpsk[i] = _p1[p1_active_carriers[i]] * 0.1f;
    }
    // demodulate
    float angle = 0;
    complex dif;
    int dbpsk_bit[P1_ACTIVE_CARRIERS];
    int dbpsk_old_bit = -1;
    dbpsk_bit[0] = dbpsk_old_bit;
    for(int i = 1; i < P1_ACTIVE_CARRIERS; ++i) {
        dif = dbpsk[i] * conj(dbpsk[i - 1]);
        angle = atan2_approx(dif.imag(), dif.real());
        dbpsk_bit[i] = abs(angle) > M_PI_2f32 ? -dbpsk_old_bit : dbpsk_old_bit;
        dbpsk_old_bit = dbpsk_bit[i];
        dbpsk_bit[i] *= p1_randomize[i];
    }
    dbpsk_bit[0] *= p1_randomize[0];
    dbpsk_old_bit = 1;
    // decode
    uint8_t bit[P1_ACTIVE_CARRIERS];
    uint8_t data[48] = {0};
    int idx_data = 0;
    int next_bit = 0;
    for(int i = 0; i < P1_ACTIVE_CARRIERS; ++i) {
        bit[i] = dbpsk_bit[i] == dbpsk_old_bit ? 0 : 1;
        dbpsk_old_bit = dbpsk_bit[i];
        if(next_bit == 8){
            next_bit = 0;
            ++idx_data;
            data[idx_data] = 0;
        }
        data[idx_data] = static_cast<uint8_t>((data[idx_data] << 1) + bit[i]);
        ++next_bit;
    }
    uint8_t s1 = 0;
    uint8_t check_data_s1 = data[0];
    for(int i = 0; i < 8; ++i) {

        if(data[i] != data[i + 40]) return false;

        if(check_data_s1 == s1_patterns[i][0]) s1 = static_cast<uint8_t>(i);
    }
    uint8_t s2 = 0;
    uint8_t check_data_s2_1 = data[8];
    uint8_t check_data_s2_2 = data[9];
    for(int i = 0; i < 16; ++i) {
        if(check_data_s2_1 == s2_patterns[i][0] && check_data_s2_2 == s2_patterns[i][1]) {
            s2 = static_cast<uint8_t>(i);
        }
    }
    switch (s1) {
    case 0:
        _dvbt2.preamble = T2_SISO;
        break;
    case 1:
        _dvbt2.preamble = T2_MISO;
        break;
    case 2:
        _dvbt2.preamble = NON_T2;
        break;
    case 3:
        _dvbt2.preamble = T2_LITE_SISO;
        break;
    case 4:
        _dvbt2.preamble = T2_LITE_MISO;
        break;
    default:

        return false;

    }
    uint8_t s2_field1 = s2 >> 1;
    switch (s2_field1) {
    case 0:
        _dvbt2.fft_mode = FFTSIZE_2K;
        break;
    case 1:
        _dvbt2.fft_mode = FFTSIZE_8K;
        break;
    case 2:
        _dvbt2.fft_mode = FFTSIZE_4K;
        break;
    case 3:
        _dvbt2.fft_mode = FFTSIZE_1K;
        break;
    case 4:
        _dvbt2.fft_mode = FFTSIZE_16K;
        break;
    case 5:
        _dvbt2.fft_mode = FFTSIZE_32K;
        break;
    case 6:
        _dvbt2.fft_mode = FFTSIZE_8K_T2GI;
        break;
    case 7:
        _dvbt2.fft_mode = FFTSIZE_32K_T2GI;
        break;
    default:

        return false;

    }
    uint8_t s2_field2 = s2 & 0x1;
    switch(s2_field2) {
    case 0:
        // not mixed : All preambles in the current transmission are
        // of the same type as this preamble
        break;
    case 1:
        // mixed : Preambles of different types are transmitted
        break;
    }

    return true;

}
//-----------------------------------------------------------------------------------------------


