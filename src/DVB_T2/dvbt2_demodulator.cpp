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
#include "dvbt2_demodulator.h"

#include <QApplication>
#include <immintrin.h>

#include "DSP/fast_math.h"

//-------------------------------------------------------------------------------------------
dvbt2_demodulator::dvbt2_demodulator(id_device_t _id_device, float _sample_rate, QObject *parent) :
    QObject(parent),
    id_device(_id_device),
    sample_rate(_sample_rate)
{
    table_sin_cos_instance.table_();

    mutex = new QMutex;

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
        level_max = 0.04f;
        level_min = level_max - 0.02f;
        break;
    case id_plutosdr:
        convert_input = 1;
        short_to_float = 1.0f / (1 << 11);
        level_max = 0.04f;
        level_min = level_max - 0.02f;
        break;
    }
    // max parameters
    unsigned int max_len_symbol = FFT_32K + FFT_32K / 4 + P1_LEN;
    resample =  sample_rate / (SAMPLE_RATE * upsample);
    max_resample = resample + resample * 1.0e-4;// for 100ppm
    uint len_max = (max_len_symbol + P1_LEN) * max_resample * upsample;

    decimator = new filter_decimator;
    out_interpolator = static_cast<complex*>(_mm_malloc(sizeof(complex) * len_max * upsample, 32));
    out_decimator = new complex[len_max];
    out_derotate_sample = new complex[len_max];
    buffer_sym = new complex[max_len_symbol];
    for(uint i = 0; i < max_len_symbol; ++i) {
        buffer_sym[i] = {0.0f, 0.0f};
    }
    //p1 symbol
    p1_demodulator = new p1_symbol();
    //__Fast Fourier Transform__
    fft = new fast_fourier_transform;
    //pilot generator
    pilot = new pilot_generator();
    //address of frequency deinterleaved
    fq_deinterleaver = new address_freq_deinterleaver();
    //p2 symbol
    p2_demodulator = new p2_symbol();
    //data symbol
    data_demodulator = new data_symbol();
    //frame closing (fc) symbol
    fc_demod = new fc_symbol();
    //time deinterleaver and removal of cyclic Q-delay
    mutex_out = new QMutex;
    signal_out = new QWaitCondition;
    deinterleaver = new time_deinterleaver(signal_out, mutex_out);
    thread = new QThread;
    deinterleaver->moveToThread(thread);
    connect(this, &dvbt2_demodulator::data, deinterleaver, &time_deinterleaver::execute);
    connect(this, &dvbt2_demodulator::l1_dyn_execute, deinterleaver, &time_deinterleaver::l1_dyn_execute);
    connect(this, &dvbt2_demodulator::stop_deinterleaver, deinterleaver, &time_deinterleaver::stop_qam);
    connect(deinterleaver, &time_deinterleaver::finished, deinterleaver, &time_deinterleaver::deleteLater);
    connect(deinterleaver, &time_deinterleaver::finished, thread, &QThread::quit, Qt::DirectConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
//    thread->start(QThread::TimeCriticalPriority);
    thread->start();
}
//-------------------------------------------------------------------------------------------
dvbt2_demodulator::~dvbt2_demodulator()
{   
    emit stop_deinterleaver();
    if(thread->isRunning()) thread->wait(1000);
    delete p1_demodulator;
    delete p2_demodulator;
    delete data_demodulator;
    delete fc_demod;
    delete fft;
    delete [] out_derotate_sample;
    delete [] out_decimator;
    delete [] buffer_sym;
    _mm_free (out_interpolator);
}
//-------------------------------------------------------------------------------------------
void dvbt2_demodulator::reset()
{
    exp_avg_dc_real.reset();
    exp_avg_dc_imag.reset();
    loop_filter_frequency_offset.reset();
    frequency_est_filtered = 0;
    frequency_nco = 0;
    loop_filter_phase_offset.reset();
    phase_nco = 0.0f;
    old_sample_rate_est = 0.0f;
    sample_rate_est_filtered = 0;
    resample =  sample_rate / (SAMPLE_RATE * upsample);
    p2_init = false;
    demodulator_init = false;
    next_symbol_type = SYMBOL_TYPE_P1;
    qDebug() << "dvbt2_demodulator reset";
}
//-------------------------------------------------------------------------------------------
void dvbt2_demodulator::init_dvbt2()
{
    dvbt2.bandwidth = BANDWIDTH_8_0_MHZ;
    dvbt2.miso_group = MISO_TX1;//?
    dvbt2_p2_parameters_init(dvbt2);
    in_fft = fft->init(dvbt2.fft_size);
    fq_deinterleaver->init(dvbt2);
    p2_demodulator->init(dvbt2, pilot, fq_deinterleaver);
    // for start;
    dvbt2.guard_interval_size = dvbt2.fft_size / 4;
    symbol_size = dvbt2.fft_size + dvbt2.guard_interval_size;
    est_chunk = symbol_size;
    //..
    p2_init = true;
 }
//-------------------------------------------------------------------------------------------
void dvbt2_demodulator::execute(int _len_in, int16_t* _i_in, int16_t* _q_in, signal_estimate *signal_)
{
    mutex->lock();

    int len_in = _len_in;
    int idx_in = 0;
    int remain;
    float real, imag;
    float theta1 = 0.0f, theta2 = 0.0f, theta3 = 0.0f;

    float nco_real, nco_imag;
    double arbitrary_resample;

    while(idx_in < len_in) {

        if(est_chunk == 0) {
            if(next_symbol_type == SYMBOL_TYPE_P1) est_chunk += P1_LEN;
            est_chunk += symbol_size;
        }

        arbitrary_resample = resample - sample_rate_est_filtered;
        if(arbitrary_resample > max_resample) arbitrary_resample = max_resample;

        chunk = static_cast<int>(std::nearbyint(est_chunk * arbitrary_resample * upsample));
        remain = len_in - idx_in;
        if(chunk > remain) chunk = remain;

        phase_nco += phase_est_filtered;
        while(phase_nco > M_PI_X_2) {
            phase_nco -= M_PI_X_2;
        }
        while(phase_nco < -M_PI_X_2) {
            phase_nco += M_PI_X_2;
        }

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
            while(offset_nco > M_PI_X_2) {
                offset_nco -= M_PI_X_2;
            }
            while(offset_nco < -M_PI_X_2) {
                offset_nco += M_PI_X_2;
            }
            nco_real = cos_lut(offset_nco);
            nco_imag = sin_lut(offset_nco);
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
        symbol_acquisition(len_out_decimator, out_decimator, signal_);

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
    if(signal_->gain_changed) {
        if(level_detect < level_min) {
            signal_->gain_offset = 1;
            signal_->change_gain = true;
        }
        else if(level_detect > level_max) {
            signal_->gain_offset = -1;
            signal_->change_gain = true;
        }
        else {
            signal_->gain_offset = 0;
            signal_->change_gain = false;
        }
    }


    mutex->unlock();

}
//-------------------------------------------------------------------------------------------
void  dvbt2_demodulator::est_1_bit_quantization(float _real, float _imag,
                                          float &_theta1, float &_theta2, float &_theta3)
{
    float sgn;
    sgn = _real < 0 ? -1.0f : 1.0f;
    _theta1 -= _imag * sgn;
    _theta2 += _real * sgn;
    sgn = _imag < 0 ? -1.0f : 1.0f;
    _theta3 += _imag * sgn;
}
//-------------------------------------------------------------------------------------------
void dvbt2_demodulator::symbol_acquisition(int _len_in, complex* _in, signal_estimate* signal_)
{
    int len_in = _len_in;
    complex* in = _in;

    float phase_est = 0.0f;
    float frequency_est = 0.0f;
    float sample_rate_est = 0.0f;


    int consume = 0;
    while(consume < len_in) {

        if(next_symbol_type == SYMBOL_TYPE_P1) {

            bool p1_decoded = false;
            if(p1_demodulator->execute(signal_->gain_changed, level_detect, len_in, in, consume,
                                 buffer_sym, idx_buffer_sym, dvbt2, signal_->coarse_freq_offset,
                                 p1_decoded, signal_->p1_reset)) {
                if(p2_init){
                    next_symbol_type = SYMBOL_TYPE_P2;
                }
                else if(signal_->frequency_changed){
                    resample -= signal_->correct_resample * resample;
                    if(std::abs(signal_->coarse_freq_offset) < 10.0f){
                        if(p1_decoded){
                            if(!signal_->p1_reset){
                                init_dvbt2();
                            }
                            else{
                                p2_init = true;
                                demodulator_init = true;
                                signal_->p1_reset = false;
                            }
                        }
                    }
                    else{
                        signal_->change_frequency = true;
                    }
                }
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
                float max_integral = 1.0f / dvbt2.fft_size;
                frequency_est_filtered += loop_filter_frequency_offset(frequency_est, max_integral);
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
            complex* deinterleaved_cell = data_demodulator->execute(idx_symbol, ofdm_cell,
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
            complex* deinterleaved_cell = p2_demodulator->execute(dvbt2, demodulator_init, idx_symbol, ofdm_cell,
                                                            l1_pre, l1_post, crc32_l1_pre, crc32_l1_post,
                                                            sample_rate_est, phase_est);
            if(crc32_l1_pre) {
                if(demodulator_init) {
                    if(crc32_l1_post) {
                        if(!deint_start) {
                            deinterleaver->start(dvbt2, l1_pre, l1_post);
                            deint_start = true;
                            emit amount_plp(l1_post.num_plp);
                        }
                        mutex_out->lock();
                        emit l1_dyn_execute(l1_post, dvbt2.c_p2, deinterleaved_cell);
                        signal_out->wait(mutex_out);
                        mutex_out->unlock();
                    }
                    ++idx_symbol;
                    next_symbol_type = SYMBOL_TYPE_DATA;
                }
                else {
                    set_guard_interval();
                    data_demodulator->init(dvbt2, pilot, fq_deinterleaver);
                    end_data_symbol = dvbt2.len_frame - dvbt2.l_fc;
                    if(dvbt2.l_fc) {
                        frame_closing_symbol = true;
                        fc_demod->init(dvbt2, pilot, fq_deinterleaver);
                    }
                    else {
                        frame_closing_symbol = false;
                    }
                    demodulator_init = true;
                    next_symbol_type = SYMBOL_TYPE_P1;

                    continue;

                }
            }
            else {
                if(!demodulator_init) {
                    set_guard_interval_by_brute_force();
                    next_symbol_type = SYMBOL_TYPE_P1;

                    continue;

                }
                else {
                    signal_->reset = true;
                    signal_->p1_reset = true;
                    reset();

                    return;

                }
            }
        }

        phase_est_filtered = loop_filter_phase_offset(phase_est * 0.5f, M_PIf32 * 2);
        double step = 8.0e-9;
        if(old_sample_rate_est - sample_rate_est > 0.0f) {
            sample_rate_est_filtered -= step;
            if(resample - sample_rate_est_filtered < -max_resample) sample_rate_est_filtered += step;
        }
        else if(old_sample_rate_est - sample_rate_est < 0.0f){
            sample_rate_est_filtered += step;
            if(resample - sample_rate_est_filtered > max_resample) sample_rate_est_filtered -= step;
        }
        old_sample_rate_est = sample_rate_est;

        float sample_rate_offset_hz = (sample_rate_est_filtered *(float) SAMPLE_RATE) / M_PI_X_2;
        float frequency_offset_hz = (frequency_est_filtered * (float)SAMPLE_RATE) / M_PI_X_2;

        emit replace_null_indicator(sample_rate_offset_hz, frequency_offset_hz);

    }

}
//----------------------------------------------------------------------------------------------
void dvbt2_demodulator::set_guard_interval()
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
//-------------------------------------------------------------------------------------------
void dvbt2_demodulator::set_guard_interval_by_brute_force ()
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
//-------------------------------------------------------------------------------------------
void dvbt2_demodulator::stop()
{
    emit finished();
}
//-------------------------------------------------------------------------------------------

