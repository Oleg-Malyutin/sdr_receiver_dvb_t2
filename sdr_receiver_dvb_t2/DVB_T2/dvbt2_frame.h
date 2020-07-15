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
#ifndef DVBT2_FRAME_H
#define DVBT2_FRAME_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QMetaType>

#include "DSP/interpolator_farrow.hh"
#include "DSP/filter_decimator.h"
#include "DSP/fast_fourier_transform.h"
#include "DSP/loop_filters.hh"
#include "dvbt2_definition.h"
#include "p1_symbol.h"
#include "pilot_generator.h"
#include "address_freq_deinterleaver.h"
#include "p2_symbol.h"
#include "data_symbol.h"
#include "fc_symbol.h"
#include "time_deinterleaver.h"

enum id_device_t{
    id_sdrplay = 0,
    id_airspy,
    id_plutosdr,
};

class dvbt2_frame : public QObject
{
    Q_OBJECT

public:
    explicit dvbt2_frame(QWaitCondition* _signal_in, QMutex* _mutex_in, id_device_t _id_device,
                         int _len_in_max, int _len_block, float _sample_rate, QObject *parent = nullptr);
    ~dvbt2_frame();

    p1_symbol* p1_demod;
    p2_symbol* p2_demod;
    data_symbol* data_demod;
    fc_symbol* fc_demod;
    time_deinterleaver* deinterleaver = nullptr;

    void correct_resample(float &_resample);
    void get_signal_estimate(bool &_adjust_frequency, float &_frequency_offset,
                               bool &_adjust_gain, int &_gain_offset);
    float value = 1.0e-2f;

signals:
    void l1_dyn_init(l1_postsignalling _l1_post, int _len_in, complex* _in);
    void amount_plp(int _num_plp);
    void data(int _len_in, complex* _in);
    void stop_deinterleaver();
    void set_os(int _n, complex* _s);
    void set_plot(int _n, complex* _fft);
    void set_const(int _len, complex* _data);
    void check(complex* _in);
    void finished();

public slots:
    void execute(int _len_in, int16_t* _i_in, int16_t* _q_in, bool _frequence_changed, bool _gain_changed);
    void stop();

private:
    QMutex* mutex_in;
    QWaitCondition* signal_in;
    QThread* thread = nullptr;
    QMutex* mutex_out;
    QWaitCondition* signal_out;

    id_device_t id_device;
    int convert_input = 1;
    float short_to_float;

    int len_in_max;
    int len_block;
    int remain = 0;
    int chunk = 0;
    int est_chunk = 0;

    static constexpr float dc_ratio = 1.0e-7f;
//    static constexpr float dc_ratio = 1.0f / SAMPLE_RATE / M_PI_X_2;//3.5e-8f;
    exponential_averager<float, float, dc_ratio> exp_avg_dc_real;
    exponential_averager<float, float, dc_ratio> exp_avg_dc_imag;

    float c1 = 0.0f;
    float c2 = 1.0f;
    inline void est_1_bit_quantization(float _real, float _imag, float &_theta1, float &_theta2, float &_theta3);
    static constexpr float theta_ratio = 5.0e-3f;
    exponential_averager<float, float, theta_ratio> exp_avg_theta1;
    exponential_averager<float, float, theta_ratio> exp_avg_theta2;
    exponential_averager<float, float, theta_ratio> exp_avg_theta3;

    float phase_nco = 0.0f;

    float frequancy_offset = 0.0f;
    static constexpr float k_proportional_fq = 0.3f;
    static constexpr float max_integral_fq = 5.0e-6f;
    pid_controller<float, k_proportional_fq, max_integral_fq> loop_filter_frequancy_offset;

    float sample_rate_offset = 0.0f;
    static constexpr float k_proportional_fs = 0.1f;
    static constexpr float max_integral_fs = 5.0e-6f;
    pid_controller<float, k_proportional_fs, max_integral_fs> loop_filter_sample_rate_offset;

    float phase_offset = 0.0f;
    complex* out_derotate_sample;
    const int upsample = DECIMATION_STEP;
    float sample_rate;
    float resample;
    float arbitrary_resample;
    complex* out_interpolator;
    complex* out_decimator;
    interpolator_farrow<complex> interpolator;   
    filter_decimator* decimator;

    dvbt2_parameters dvbt2;
    bool p2_alredy_init = false;
    void init_dvbt2();

    int symbol_size = 0;
    int idx_buffer_sym = 0;
    complex* buffer_sym = nullptr;
    fast_fourier_transform* fft;
    complex* in_fft = nullptr;
    complex* ofdm_cell;
    pilot_generator* pilot;
    address_freq_deinterleaver* fq_deint;
    int next_symbol_type = SYMBOL_TYPE_P1;
    bool demod_init = false;
    bool deint_start = false;
    int idx_symbol = 0;
    l1_presignalling l1_pre;
    l1_postsignalling l1_post;

    bool frame_closing_symbol = false;
    int end_data_symbol = 1;

    bool change_frequency = false;
    float frequency_offset = 0.0f;

    bool change_gain = false;
    int gain_offset = 0;
    bool check_gain = false;
    float level_max;
    float level_min;

    void symbol_acquisition(int _len_in, complex* _in, bool& _frequency_changed,
                            float &_frequancy_offset, float &_phase_offset,
                            float &_sample_rate_offset, int& _est_len_in);
    void set_guard_interval();
    void set_guard_interval_by_brute_force ();

};

#endif // DVBT2_FRAME_H
