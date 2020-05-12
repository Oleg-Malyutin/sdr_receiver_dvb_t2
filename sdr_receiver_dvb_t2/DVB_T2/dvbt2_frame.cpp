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

#include "DSP/fast_math.h"

//-----------------------------------------------------------------------------------------------
dvbt2_frame::dvbt2_frame(QWaitCondition *_signal_in, QMutex *_mutex_in,
                         int _len_in_max, int _len_block, float _sample_rate, QObject *parent) :
    QObject(parent),
    mutex_in(_mutex_in),
    signal_in(_signal_in),
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
    unsigned int max_len_symbol = FFT_32K + FFT_32K / 4;
    buffer_sym = new complex[max_len_symbol];
    memset(buffer_sym, 0, sizeof(complex) * static_cast<uint>(max_len_symbol));

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
    thread->start();
}
//-----------------------------------------------------------------------------------------------
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
}
//-----------------------------------------------------------------------------------------------
void dvbt2_frame::init_dvbt2()
{
    dvbt2.bandwidth = BANDWIDTH_8_0_MHZ;
    dvbt2.miso_group = MISO_TX1;//??????
    dvbt2_p2_parameters_init(dvbt2);
    in_fft = fft->init(dvbt2.fft_size);
    fq_deint->init(dvbt2);
    p2_demod->init(dvbt2, pilot, fq_deint);
    // for start;
    dvbt2.guard_interval_size = dvbt2.fft_size / 8;
    symbol_size = dvbt2.fft_size + dvbt2.guard_interval_size;
    est_chunk = symbol_size;
    //..
    p2_alredy_init = true;
}
//-----------------------------------------------------------------------------------------------
void dvbt2_frame::correct_resample(float &_corect)
{
    resample += _corect * resample;
}
//-----------------------------------------------------------------------------------------------
void dvbt2_frame::execute(int _len_in, short* _i_in, short* _q_in, bool _frequence_changed, bool _gain_changed)
{
    mutex_in->lock();

    int len_in = _len_in;
    int idx_in = 0;
    int remain;
    float real, imag;
    float theta1 = 0, theta2 = 0, theta3 = 0;
    float phase_derotate_real, phase_derotate_imag, nco_derotate_real, nco_derotate_imag;
    float real_rotate, imag_rotate;

    while(idx_in < len_in) {

        if(est_chunk < len_block) {
            if(next_symbol_type == SYMBOL_TYPE_P1) est_chunk += P1_LEN;
            est_chunk += symbol_size;
        }
        chunk = static_cast<int>(std::nearbyint(est_chunk * arbitrary_resample * upsample));
        remain = len_in - idx_in;
        if(chunk > remain) chunk = remain;

        phase_derotate_real = cos_lut(-phase_offset);
        phase_derotate_imag = sin_lut(-phase_offset);
        for(int i = 0; i < chunk; ++i) {
            real = _i_in[i + idx_in] * short_to_float;
            imag = _q_in[i + idx_in] * short_to_float;
            //___DC offset remove____________
            real -= exp_avg_dc_real(real);
            imag -= exp_avg_dc_imag(imag);
            //___IQ imbalance remove_________
            est_1_bit_quantization(real, imag, theta1, theta2, theta3);
            real *= c2;
            imag += c1 * real;
            //___phase synchronization_______
            real_rotate = real * phase_derotate_real - imag * phase_derotate_imag;
            imag_rotate = imag * phase_derotate_real + real * phase_derotate_imag;
            //___frequency synchronization___
            phase_nco -= frequancy_offset;
            while(phase_nco > M_PI_X_2) phase_nco -= M_PI_X_2;
            while(phase_nco < -M_PI_X_2) phase_nco += M_PI_X_2;
            nco_derotate_real = cos_lut(phase_nco);
            nco_derotate_imag = sin_lut(phase_nco);
            real = real_rotate * nco_derotate_real - imag_rotate * nco_derotate_imag;
            imag = imag_rotate * nco_derotate_real + real_rotate * nco_derotate_imag;
            out_derotate_sample[i].real(real);
            out_derotate_sample[i].imag(imag);
            //_____________________________
        }
        idx_in += chunk;

        //___timing synchronization___
        arbitrary_resample = resample - sample_rate_offset;
        int len_out_interpolator;
        interpolator(chunk, out_derotate_sample, arbitrary_resample, len_out_interpolator, out_interpolator);
        int len_out_decimator;
        decimator->execute(len_out_interpolator, out_interpolator, len_out_decimator, out_decimator);
        //___demodulations and get offset sinchronization___
        symbol_acquisition(len_out_decimator, out_decimator, _frequence_changed,
                           frequancy_offset, phase_offset, sample_rate_offset, est_chunk);
    }
    //___IQ imbalance estimations___
    float avg_theta1 = theta1 / len_in;
    float avg_theta2 = theta2 / len_in;
    float avg_theta3 = theta3 / len_in;
    c1 = avg_theta1 / avg_theta2;
    float c_temp = avg_theta3 / avg_theta2;
    c2 = sqrtf(c_temp * c_temp - c1 * c1);
    //___level gain estimation___
    float level_detect = avg_theta2 * avg_theta3;
//    qDebug() << "level_detect "<< level_detect;
    float contr = 0.006f;
    if(check_gain) {
        check_gain = false;
        if(level_detect < contr) {
            gain_offset = 1;
            change_gain = true;
        }
        else if(level_detect > contr + contr / 3.0f) {
            gain_offset = -1;
            change_gain = true;
        }
        else {
            change_gain = false;
        }
    }
    if(_gain_changed) check_gain = true;

    mutex_in->unlock();

}
//-----------------------------------------------------------------------------------------------
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
//-----------------------------------------------------------------------------------------------
void dvbt2_frame::symbol_acquisition(int _len_in, complex* _in, bool& _frequency_changed,
                                     float &_frequancy_offset, float &_phase_offset,
                                     float &_sample_rate_offset, int& _est_len_in)
{   
    int len_in = _len_in;
    complex* in = _in;
    int est_len_in = 0;
    float phase_offset = 0.0;
    static float sample_rate_offset = 0.0;
    static float frequancy_offset = 0.0;
    static bool crc32_l1_pre = false;

    int consume = 0;
    while(consume < len_in) {

        if(next_symbol_type == SYMBOL_TYPE_P1) {
            float coarse_freq_offset = 0.0;
            bool coarse_freq_tuned = false;
            bool p1_decoded = false;
//            qDebug() << "P1_";
            if(p1_demod->execute(len_in, in, consume, buffer_sym, idx_buffer_sym,
                                 dvbt2, coarse_freq_offset, p1_decoded)) {

                coarse_freq_tuned = abs(coarse_freq_offset) < 120 ? true : false;
//                qDebug() << "P1";

                if(p2_alredy_init && coarse_freq_tuned) {
                    next_symbol_type = SYMBOL_TYPE_P2;
                }
                else {
                    if(!coarse_freq_tuned) {
                        if(_frequency_changed){
                            change_frequency = true;
                            frequency_offset = coarse_freq_offset;
                        }
                    }
                    if(!p2_alredy_init && p1_decoded) init_dvbt2();
                    next_symbol_type = SYMBOL_TYPE_P1;
                }
                idx_symbol = 0;
            }

            continue;

        }

        //__Fast Fourier Transform_________________________________
        int len_in_sym = len_in - consume;
        int len_out_sym = symbol_size - idx_buffer_sym;
        int len_cpy_sym = len_out_sym > len_in_sym ? len_in_sym : len_out_sym;
        memcpy(buffer_sym + idx_buffer_sym, in + consume, sizeof(complex) *
               static_cast<uint>(len_cpy_sym));
        consume += len_cpy_sym;
        idx_buffer_sym += len_cpy_sym;
        if(idx_buffer_sym == symbol_size) {
            idx_buffer_sym = 0;
            if(crc32_l1_pre) {
                complex* cp = buffer_sym + dvbt2.fft_size;
                complex sum = {0.0, 0.0};
                for (int i = 0; i < dvbt2.guard_interval_size; ++i) sum += cp[i] * conj(buffer_sym[i]);
                frequancy_offset = atan2_approx(sum.imag(), sum.real()) / (dvbt2.fft_size << 1);
            }
            memcpy(in_fft, &buffer_sym[dvbt2.guard_interval_size], sizeof(complex) *
                    static_cast<uint>(dvbt2.fft_size));
            ofdm_cell = fft->execute();
        }
        else {
            est_len_in = symbol_size - idx_buffer_sym;

            continue;

        }
        //__end Fast Fourier Transform_________________________________

        if(next_symbol_type == SYMBOL_TYPE_DATA) {
            complex* deinterleaved_cell = data_demod->execute(idx_symbol, ofdm_cell,
                                                              sample_rate_offset, phase_offset);
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
            complex* deinterleaved_cell = fc_demod->execute(ofdm_cell, sample_rate_offset, phase_offset);
            if(deint_start) {
                mutex_out->lock();
                emit data(dvbt2.n_fc, deinterleaved_cell);
                signal_out->wait(mutex_out);
                mutex_out->unlock();
            }
            next_symbol_type = SYMBOL_TYPE_P1;
        }
        else if(next_symbol_type == SYMBOL_TYPE_P2) {
            bool crc32_l1_post = false;
            complex* deinterleaved_cell = p2_demod->execute(dvbt2, demod_init, idx_symbol, ofdm_cell,
                                                            l1_pre, l1_post, crc32_l1_pre, crc32_l1_post,
                                                            sample_rate_offset, phase_offset);
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
                set_guard_interval_by_brute_force ();
                next_symbol_type = SYMBOL_TYPE_P1;
                continue;
            }
        }

        _sample_rate_offset = sample_rate_offset;
//        _sample_rate_offset = loop_filter_sample_rate_offset(sample_rate_offset, false);
        _frequancy_offset += frequancy_offset;
//        _frequancy_offset += loop_filter_frequancy_offset(frequancy_offset, false);
        _phase_offset += phase_offset * 0.1f;
        while(_phase_offset > M_PI_X_2) _phase_offset -= M_PI_X_2;
        while(_phase_offset < -M_PI_X_2) _phase_offset += M_PI_X_2;
    }
    _est_len_in = est_len_in;
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
//-----------------------------------------------------------------------------------------------
void dvbt2_frame::set_guard_interval_by_brute_force ()
{
    static int idx = 0;
    switch (idx) {
    case 0:
        dvbt2.guard_interval_size = dvbt2.fft_size / 4;
        ++idx;
        break;
    case 1:
        dvbt2.guard_interval_size = dvbt2.fft_size / 8;
        ++idx;
        break;
    case 2:
        dvbt2.guard_interval_size = dvbt2.fft_size / 16;
        ++idx;
        break;
    case 3:
        dvbt2.guard_interval_size = dvbt2.fft_size / 32;
        ++idx;
        break;
    case 4:
        dvbt2.guard_interval_size = dvbt2.fft_size / 128;
        ++idx;
        break;
    case 5:
        dvbt2.guard_interval_size = (dvbt2.fft_size / 128) * 19;
        ++idx;
        break;
    case 6:
        dvbt2.guard_interval_size = (dvbt2.fft_size / 256) * 19;
        idx = 0;
        break;
    default:
        break;
    }
    symbol_size = dvbt2.fft_size + dvbt2.guard_interval_size;
    est_chunk = symbol_size;
}
//-----------------------------------------------------------------------------------------------
void dvbt2_frame::get_signal_estimate(bool &_cange_frequency, float &_frequency_offset,
                                      bool &_change_gain, int &_gain_offset)
{
    _cange_frequency = change_frequency;
    change_frequency = false;
    _frequency_offset = frequency_offset;
    _change_gain = change_gain;
    change_gain = false;
    _gain_offset = gain_offset;
}
//-----------------------------------------------------------------------------------------------
void dvbt2_frame::stop(){
    emit finished();
}
//-----------------------------------------------------------------------------------------------
