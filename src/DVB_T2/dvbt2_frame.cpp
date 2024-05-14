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
#include "dvbt2_frame.h"

#include <QApplication>
#include <immintrin.h>

// #include <iostream>
// #include <iomanip>

#include "DSP/fast_math.h"

//----------------------------------------------------------------------------------------------------------------------------
dvbt2_frame::dvbt2_frame(QWaitCondition *_signal_in, QMutex *_mutex_in, id_device_t _id_device,
                         int _len_in_max, int _len_block, float _sample_rate, QObject *parent) :
    QObject(parent),
    mutex_in(_mutex_in),
    signal_in(_signal_in),
    id_device(_id_device),
    len_in_max(_len_in_max),
    len_block(_len_block),
    sample_rate(_sample_rate)
{
    table_sin_cos_instance.table_();
    out_derotate_sample = new complex[len_in_max];
    resample =  sample_rate / (SAMPLE_RATE * upsample);
    arbitrary_resample = resample;
    out_interpolator = static_cast<complex *>(_mm_malloc(sizeof(complex)
                       * static_cast<uint>(len_in_max * upsample), 32));
    out_decimator = new complex[len_in_max];
    decimator = new filter_decimator;
    unsigned int max_len_symbol = FFT_32K + FFT_32K / 4 + P1_LEN;
    buffer_sym = new complex[max_len_symbol];
    for(uint i = 0; i < max_len_symbol; ++i) buffer_sym[i] = {0.0f, 0.0f};

    switch (id_device) {
    case id_sdrplay:
        convert_input = 1;
        short_to_float = 1.0f / (1 << 14);
        level_max = 0.04f;
        level_min = level_max - 0.02f;
        break;
    case id_airspy:
        convert_input = 2;
        short_to_float = 1.0f / (1 << 12);
        level_max = 0.03f;
        level_min = level_max - 0.015f;
        break;
    case id_plutosdr:
        convert_input = 1;
        short_to_float = 1.0f / (1 << 11);
        level_max = 0.06f;
        level_min = level_max - 0.02f;
        break;
    }

    //preamble p1 symbol
    p1_demod = new p1_symbol();
    //__Fast Fourier Transform__
    fft = new fast_fourier_transform;
    //pilot generator
    pilot = new pilot_generator();
    //address of frequancy deintrleaved
    fq_deint = new address_freq_deinterleaver();
    //preamble p2 symbol
    p2_demod = new p2_symbol();
    //data symbol
    data_demod = new data_symbol();
    //frame closing symbol
    fc_demod = new fc_symbol();
    //time deinterleaver and removal of cyclic Q-delay
    mutex_out = new QMutex;
    signal_out = new QWaitCondition;
    deinterleaver = new time_deinterleaver(signal_out, mutex_out);
    thread = new QThread;
    deinterleaver->moveToThread(thread);
    connect(this, &dvbt2_frame::data, deinterleaver, &time_deinterleaver::execute);
    connect(this, &dvbt2_frame::l1_dyn_init, deinterleaver, &time_deinterleaver::l1_dyn_init);
    connect(this, &dvbt2_frame::stop_deinterleaver, deinterleaver, &time_deinterleaver::stop_qam);
    connect(deinterleaver, &time_deinterleaver::finished, deinterleaver, &time_deinterleaver::deleteLater);
    connect(deinterleaver, &time_deinterleaver::finished, thread, &QThread::quit, Qt::DirectConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
//    thread->start(QThread::TimeCriticalPriority);
    thread->start();
}
//----------------------------------------------------------------------------------------------------------------------------
dvbt2_frame::~dvbt2_frame()
{   
    emit stop_deinterleaver();
    if(thread->isRunning()) thread->wait(1000);
    delete p1_demod;
    delete p2_demod;
    delete data_demod;
    delete fc_demod;
    delete fft;
    delete [] out_derotate_sample;
    delete [] out_decimator;
    delete [] buffer_sym;
    _mm_free (out_interpolator);
}
//----------------------------------------------------------------------------------------------------------------------------
void dvbt2_frame::init_dvbt2()
{
    dvbt2.bandwidth = BANDWIDTH_8_0_MHZ;
    dvbt2.miso_group = MISO_TX1;//?
    dvbt2_p2_parameters_init(dvbt2);
    in_fft = fft->init(dvbt2.fft_size);
    fq_deint->init(dvbt2);
    p2_demod->init(dvbt2, pilot, fq_deint);
    // for start;
    dvbt2.guard_interval_size = dvbt2.fft_size / 4;
    symbol_size = dvbt2.fft_size + dvbt2.guard_interval_size;
    est_chunk = symbol_size;
    //..
    p2_alredy_init = true;
 }
//----------------------------------------------------------------------------------------------------------------------------
void dvbt2_frame::correct_resample(float &_corect)
{
    resample += _corect * resample;
}
//----------------------------------------------------------------------------------------------------------------------------
void dvbt2_frame::execute(int _len_in, int16_t* _i_in, int16_t* _q_in,
                          bool _frequence_changed, bool _gain_changed)
{
    mutex_in->lock();

    int len_in = _len_in;
    int idx_in = 0;
    int remain;
    float real, imag;
    float theta1 = 0.0f, theta2 = 0.0f, theta3 = 0.0f;
    float nco_real, nco_imag;
    while(idx_in < len_in) {

        if(est_chunk < len_block) {
            if(next_symbol_type == SYMBOL_TYPE_P1) est_chunk += P1_LEN;
            est_chunk += symbol_size;
        }
        chunk = static_cast<int>(std::nearbyint(est_chunk * arbitrary_resample * upsample));
        remain = len_in - idx_in;
        if(chunk > remain) chunk = remain;
        int j = 0;
        for(int i = 0; i < chunk; ++i) {
            j = (i + idx_in) * convert_input;
            real = _i_in[j] * short_to_float;
            imag = _q_in[j] * short_to_float;
            //___DC offset remove____________
            real -= exp_avg_dc_real(real);
            imag -= exp_avg_dc_imag(imag);
            //___IQ imbalance remove_________
            est_1_bit_quantization(real, imag, theta1, theta2, theta3);
            real *= c2;
            imag += c1 * real;
            //___phase and frequency synchronization___
            frequency_nco -= frequency_est_filtered;
            while(frequency_nco > M_PI_X_2) {
                frequency_nco -= M_PI_X_2;
            }
            while(frequency_nco < -M_PI_X_2) {
                frequency_nco += M_PI_X_2;
            }
            float offset_nco = frequency_nco - phase_nco;
            nco_real = cos(offset_nco);
            nco_imag = sin(offset_nco);
            out_derotate_sample[i].real(real * nco_real - imag * nco_imag);
            out_derotate_sample[i].imag(imag * nco_real + real * nco_imag);
            //_____________________________
        }  
        idx_in += chunk;

        //___timing synchronization___
        int len_out_interpolator;
        interpolator(chunk, out_derotate_sample, arbitrary_resample,
                     len_out_interpolator, out_interpolator);
        int len_out_decimator;

        decimator->execute(len_out_interpolator, out_interpolator, len_out_decimator, out_decimator);
        //___demodulations and get offset synchronization__
        symbol_acquisition(len_out_decimator, out_decimator, _frequence_changed, _gain_changed);
        arbitrary_resample = resample - sample_rate_est_filtered;
        phase_nco += phase_est_filtered;
        while(phase_nco > M_PI_X_2) {
            phase_nco -= M_PI_X_2;
        }
        while(phase_nco > M_PI_X_2) {
            phase_nco += M_PI_X_2;
        }
    }
    //___IQ imbalance estimations___
    float avg_theta1 = theta1 / len_in;
    float avg_theta2 = theta2 / len_in;
    float avg_theta3 = theta3 / len_in;
    c1 = avg_theta1 / avg_theta2;
    float c_temp = avg_theta3 / avg_theta2;
    c2 = sqrtf(c_temp * c_temp - c1 * c1);
    //___level gain estimation___
    level_detect = avg_theta2 * avg_theta3;
    check_gain = _gain_changed ? true : false;
    if(check_gain) {
        check_gain = false;
        if(level_detect < level_min) {
            gain_offset = 1;
            change_gain = true;
        }
        else if(level_detect > level_max) {
            gain_offset = -1;
            change_gain = true;
        }
        else {
            change_gain = false;
        }
    }

    mutex_in->unlock();

}
//----------------------------------------------------------------------------------------------------------------------------
void  dvbt2_frame::est_1_bit_quantization(float _real, float _imag,
                                          float &_theta1, float &_theta2, float &_theta3)
{
    float sgn;
    sgn = _real < 0 ? -1.0f : 1.0f;
    _theta1 -= _imag * sgn;
    _theta2 += _real * sgn;
    sgn = _imag < 0 ? -1.0f : 1.0f;
    _theta3 += _imag * sgn;
}
//----------------------------------------------------------------------------------------------------------------------------
void dvbt2_frame::symbol_acquisition(int _len_in, complex* _in, bool& _frequency_changed,
                                     bool _gain_changed)
{
    int len_in = _len_in;
    complex* in = _in;

    float phase_est = 0.0f;
    float frequency_est = 0.0f;
    float sample_rate_est = 0.0f;
    static float old_sample_rate_est = 0.0f;

    int consume = 0;
    while(consume < len_in) {

        if(next_symbol_type == SYMBOL_TYPE_P1) {

            bool p1_decoded = false;
            if(p1_demod->execute(_gain_changed, level_detect, len_in, in, consume, buffer_sym,
                                 idx_buffer_sym, dvbt2, coarse_freq_offset, p1_decoded)) {
                if(p2_alredy_init){
                    next_symbol_type = SYMBOL_TYPE_P2;
                }
                else if(_frequency_changed){
                    if(std::abs(coarse_freq_offset) < 10.0f){
                        if(p1_decoded)init_dvbt2();
                    }
                    else{
                        coarse_frequency_setting = true;
                    }
                }
                else{
                   coarse_frequency_setting = false;
                }
            }
            else if(!_frequency_changed) {
                coarse_frequency_setting = false;
            }
            else if(len_in - consume > P1_LEN){
                loop_filter_frequency_offset.reset();
                frequency_nco = 0;
                phase_nco = 0.0f;
                old_sample_rate_est = 0.0f;
            }

            continue;

        }
        //__Fast Fourier Transform_________________________________
        uint len_in_sym = len_in - consume;
        uint len_out_sym = symbol_size - idx_buffer_sym;
        uint len_cpy_sym = len_out_sym > len_in_sym ? len_in_sym : len_out_sym;
        memcpy(buffer_sym + idx_buffer_sym, in + consume, sizeof(complex) * len_cpy_sym);
        consume += len_cpy_sym;
        idx_buffer_sym += len_cpy_sym;
        if(idx_buffer_sym == symbol_size) {
            idx_buffer_sym = 0;
            if(crc32_l1_pre) {
                complex* cp = buffer_sym + dvbt2.fft_size;
                complex sum = {0.0f, 0.0f};
                for (int i = 4; i < dvbt2.guard_interval_size - 4; ++i){
                    sum += (cp[i] * conj(buffer_sym[i]));
                }
                frequency_est = atan2_approx(sum.imag(), sum.real()) / (dvbt2.fft_size << 1);
                frequency_est_filtered += loop_filter_frequency_offset(frequency_est);
            }

            memcpy(in_fft, buffer_sym + dvbt2.guard_interval_size,
                   sizeof(complex) * static_cast<uint>(dvbt2.fft_size));
            ofdm_cell = fft->execute();

            est_chunk = 0;
        }
        else {
            est_chunk = symbol_size - idx_buffer_sym;

            continue;

        }
        //________________________________________________________
        if(next_symbol_type == SYMBOL_TYPE_DATA) {
            complex* deinterleaved_cell = data_demod->execute(idx_symbol, ofdm_cell,
                                                              sample_rate_est, phase_est);
            if(deint_start) {
                mutex_out->lock();
                emit data(dvbt2.c_data, deinterleaved_cell);
                signal_out->wait(mutex_out);
                mutex_out->unlock();
            }
            ++idx_symbol;
            if(idx_symbol == end_data_symbol) {
                if(frame_closing_symbol) next_symbol_type = SYMBOL_TYPE_FC;
                else next_symbol_type = SYMBOL_TYPE_P1;
            }
        }
        else if(next_symbol_type == SYMBOL_TYPE_FC) {
            complex* deinterleaved_cell = fc_demod->execute(ofdm_cell, sample_rate_est, phase_est);
            if(deint_start) {
                mutex_out->lock();
                emit data(dvbt2.n_fc, deinterleaved_cell);
                signal_out->wait(mutex_out);
                mutex_out->unlock();
            }
            next_symbol_type = SYMBOL_TYPE_P1;
        }
        else if(next_symbol_type == SYMBOL_TYPE_P2) {
            idx_symbol = 0;
            bool crc32_l1_post = false;
            complex* deinterleaved_cell = p2_demod->execute(dvbt2, demod_init, idx_symbol, ofdm_cell,
                                                            l1_pre, l1_post, crc32_l1_pre, crc32_l1_post,
                                                            sample_rate_est, phase_est);
            if(crc32_l1_pre) {
                if(demod_init) {
                    if(crc32_l1_post) {
                        if(deint_start) {
                            mutex_out->lock();
                            emit l1_dyn_init(l1_post, dvbt2.c_p2, deinterleaved_cell);
                            signal_out->wait(mutex_out);
                            mutex_out->unlock();
                        }
                        else {
                            deinterleaver->start(dvbt2, l1_pre, l1_post);
                            deint_start = true;
                            mutex_out->lock();
                            emit l1_dyn_init(l1_post, dvbt2.c_p2, deinterleaved_cell);
                            signal_out->wait(mutex_out);
                            mutex_out->unlock();
                            emit amount_plp(l1_post.num_plp);
                        }
                    }
                    else{
                    }
                    ++idx_symbol;
                    next_symbol_type = SYMBOL_TYPE_DATA;
                }
                else {
                    set_guard_interval();
                    data_demod->init(dvbt2, pilot, fq_deint);
                    end_data_symbol = dvbt2.len_frame - dvbt2.l_fc;
                    if(dvbt2.l_fc) {
                        frame_closing_symbol = true;
                        fc_demod->init(dvbt2, pilot, fq_deint);
                    }
                    else {
                        frame_closing_symbol = false;
                    }
                    demod_init = true;
                    next_symbol_type = SYMBOL_TYPE_P1;
                }
            }
            else {
                if(!demod_init) {
                    set_guard_interval_by_brute_force();
                    next_symbol_type = SYMBOL_TYPE_P1;
                    continue;
                }
                else {
                    ++idx_symbol;
                    next_symbol_type = SYMBOL_TYPE_DATA;
                }
            }
        }

        if(demod_init) {
            phase_est_filtered = phase_est * 0.25f;
            if(old_sample_rate_est - sample_rate_est > 0.0f) {
                sample_rate_est_filtered -= 1.0e-8;
            }
            else if(old_sample_rate_est - sample_rate_est < 0.0f){
                sample_rate_est_filtered += 1.0e-8;
            }
            old_sample_rate_est = sample_rate_est;
        }
        float sample_rate_offset_hz = (resample - arbitrary_resample) * SAMPLE_RATE / M_PI_X_2;
        if(std::abs(frequency_est_filtered / M_PI_X_2) > 0.8f / dvbt2.fft_size ||
            std::abs(sample_rate_offset_hz) > 5.0) {
            frequency_nco = 0;
            frequency_est_filtered = 0;
            sample_rate_est_filtered = 0;
            next_symbol_type = SYMBOL_TYPE_P1;
        }

        float frequency_offset_hz = (frequency_est_filtered * (float)SAMPLE_RATE) / M_PI_X_2;

        emit replace_null_indicator(sample_rate_offset_hz, frequency_offset_hz);

    }

}
//----------------------------------------------------------------------------------------------
void dvbt2_frame::set_guard_interval()
{
    switch (dvbt2.guard_interval_mode) {
    case GI_1_4:
        dvbt2.guard_interval_size = dvbt2.fft_size / 4;
        break;
    case GI_1_8:
        dvbt2.guard_interval_size = dvbt2.fft_size / 8;
        break;
    case GI_1_16:
        dvbt2.guard_interval_size = dvbt2.fft_size / 16;
        break;
    case GI_1_32:
        dvbt2.guard_interval_size = dvbt2.fft_size / 32;
        break;
    case GI_1_128:
        dvbt2.guard_interval_size = dvbt2.fft_size / 128;
        break;
    case GI_19_128:
        dvbt2.guard_interval_size = (dvbt2.fft_size / 128) * 19;
        break;
    case GI_19_256:
        dvbt2.guard_interval_size = (dvbt2.fft_size / 256) * 19;
        break;
    default:
        break;
    }
    symbol_size = dvbt2.fft_size + dvbt2.guard_interval_size;
    est_chunk = symbol_size;
}
//----------------------------------------------------------------------------------------------------------------------------
void dvbt2_frame::set_guard_interval_by_brute_force ()
{
    static int idx = 0;
    static int c = 0;
    int num = 5;
    if(c == 0) {
        frequency_est_filtered = 0.0f;
        frequency_nco = 0.0f;
    }
    switch (idx) {
    case 0:
        dvbt2.guard_interval_size = dvbt2.fft_size / 32;
        if(c++ > num){
            ++idx;
            c = 0;
        }
        break;
    case 1:
        dvbt2.guard_interval_size = dvbt2.fft_size / 16;
        if(c++ > num){
            ++idx;
            c = 0;
        }
        break;
    case 2:
        dvbt2.guard_interval_size = dvbt2.fft_size / 8;
        if(c++ > num){
            ++idx;
            c = 0;
        }
        break;
    case 3:
        dvbt2.guard_interval_size = dvbt2.fft_size / 4;
        if(c++ > num){
            ++idx;
            c = 0;
        }
        break;
    case 4:
        dvbt2.guard_interval_size = dvbt2.fft_size / 128;
        if(c++ > num){
            ++idx;
            c = 0;
        }
        break;
    case 5:
        dvbt2.guard_interval_size = (dvbt2.fft_size / 128) * 19;
        if(c++ > num){
            ++idx;
            c = 0;
        }
        break;
    case 6:
        dvbt2.guard_interval_size = (dvbt2.fft_size / 256) * 19;
        if(c++ > num){
            idx = 0;
            c = 0;
        }
        break;
    default:
        break;
    }
    symbol_size = dvbt2.fft_size + dvbt2.guard_interval_size;
    est_chunk = symbol_size;
}
//----------------------------------------------------------------------------------------------------------------------------
void dvbt2_frame::get_signal_estimate(bool &_cange_frequency, float &_coarse_freq_offset,
                                      bool &_change_gain, int &_gain_offset)
{
    _cange_frequency = coarse_frequency_setting;
    coarse_frequency_setting = false;
    _coarse_freq_offset = coarse_freq_offset;
    _change_gain = change_gain;
    change_gain = false;
    _gain_offset = gain_offset;

}
//----------------------------------------------------------------------------------------------------------------------------
void dvbt2_frame::stop()
{
    emit finished();
}
//----------------------------------------------------------------------------------------------------------------------------

