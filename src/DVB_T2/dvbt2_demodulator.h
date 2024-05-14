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
#ifndef DVBT2_DEMODULATOR_H
#define DVBT2_DEMODULATOR_H

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

struct signal_estimate{
    bool change_frequency = false;
    double coarse_freq_offset = 0.0;
    bool frequency_changed = true;
    bool change_gain = false;
    int gain_offset = 0;
    bool gain_changed = true;
    double correct_resample = 0.0;
    bool reset = false;
    bool p1_reset = false;
};

class dvbt2_demodulator : public QObject
{
    Q_OBJECT

public:
    explicit dvbt2_demodulator(id_device_t _id_device, float _sample_rate, QObject *parent = nullptr);
    ~dvbt2_demodulator();

    QMutex* mutex;
    p1_symbol* p1_demodulator;
    p2_symbol* p2_demodulator;
    data_symbol* data_demodulator;
    fc_symbol* fc_demod;
    time_deinterleaver* deinterleaver = nullptr;

signals:
    void replace_null_indicator(const float _b1, const float _b2);
    void l1_dyn_execute(l1_postsignalling _l1_post, int _len_in, complex* _in);
    void amount_plp(int _num_plp);
    void data(int _len_in, complex* _in);
    void stop_deinterleaver();
    void finished();

public slots:
    void execute(int _len_in, int16_t* _i_in, int16_t* _q_in, signal_estimate* signal_);
    void stop();

private:
    QThread* thread = nullptr;
    QMutex* mutex_out;
    QWaitCondition* signal_out;

    id_device_t id_device;
    int convert_input = 1;
    float short_to_float;

    int remain = 0;
    int chunk = 0;
    int est_chunk = 0;

    static constexpr float dc_ratio = 1.0e-6f;//1.0e-5f
    exponential_averager<float, float, dc_ratio> exp_avg_dc_real;
    exponential_averager<float, float, dc_ratio> exp_avg_dc_imag;

    float c1 = 0.0f;
    float c2 = 1.0f;
    inline void est_1_bit_quantization(float _real, float _imag,
                                       float &_theta1, float &_theta2, float &_theta3);

    constexpr static int samplerate_hz = SAMPLE_RATE;

    float phase_nco = 0.0f;
    float phase_est_filtered = 0.0f;
    constexpr static float damping_ratio_ = 0.3f;
    constexpr static int bw_hz_ = 1000000;
    proportional_integral_loop_filter<float, float, damping_ratio_, bw_hz_,
                                      samplerate_hz> loop_filter_phase_offset;
    float frequency_nco = 0.0f;
    float frequency_est_filtered = 0.0f;
    constexpr static float damping_ratio = 0.7f;//0.7f
    constexpr static int bw_hz = 4000000;
    proportional_integral_loop_filter<float, float, damping_ratio, bw_hz,
                                      samplerate_hz> loop_filter_frequency_offset;

    double sample_rate_est_filtered = 0.0;
    float old_sample_rate_est = 0.0f;

    complex* out_derotate_sample;
    constexpr static uint upsample = DECIMATION_STEP;
    float sample_rate;
    double resample;
    double max_resample;

    complex* out_interpolator;
    complex* out_decimator;
    interpolator_farrow<complex, float> interpolator;
    filter_decimator* decimator;

    dvbt2_parameters dvbt2;
    bool p2_init = false;
    void reset();
    void init_dvbt2();

    int symbol_size = P1_LEN;
    int idx_buffer_sym = 0;
    complex* buffer_sym = nullptr;
    fast_fourier_transform* fft;
    complex* in_fft = nullptr;
    complex* ofdm_cell;
    pilot_generator* pilot;
    address_freq_deinterleaver* fq_deinterleaver;
    int next_symbol_type = SYMBOL_TYPE_P1;
    bool demodulator_init = false;
    bool deint_start = false;
    int idx_symbol = 0;
    bool crc32_l1_pre = false;
    l1_presignalling l1_pre;
    l1_postsignalling l1_post;

    bool frame_closing_symbol = false;
    int end_data_symbol = 1;

    bool change_frequency = false;
    float coarse_freq_offset = 0.0f;

    bool change_gain = false;
    int gain_offset = 0;
    float level_detect = std::numeric_limits<float>::max();
    float level_max;
    float level_min;

    void symbol_acquisition(int _len_in, complex* _in, signal_estimate *signal_);
    void set_guard_interval();
    void set_guard_interval_by_brute_force ();

};

#endif // DVBT2_DEMODULATOR_H
