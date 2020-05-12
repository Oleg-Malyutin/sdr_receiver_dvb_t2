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
#ifndef P2_SYMBOL_H
#define P2_SYMBOL_H

#include <QObject>

#include "dvbt2_definition.h"
#include "pilot_generator.h"
#include "address_freq_deinterleaver.h"

#define KBCH_1_4 3072
#define NBCH_1_4 3240
#define KBCH_1_2 7032
#define NBCH_1_2 7200
#define KSIG_PRE 200
#define KSIG_POST 350
#define NBCH_PARITY 168
#define CRC_POLY 0x04C11DB7

class p2_symbol : public QObject
{
    Q_OBJECT

public:
    p2_symbol(QObject* parent = nullptr);
    ~p2_symbol();
    void init(dvbt2_parameters &_dvbt2, pilot_generator* _pilot,
                                   address_freq_deinterleaver* _address);
    complex *execute(dvbt2_parameters &_dvbt2, bool _demod_init, int &_idx_symbol, complex* _ofdm_cell,
                     l1_presignalling &_l1_pre, l1_postsignalling &_l1_post, bool &_crc32_l1_pre,
                     bool &_crc32_l1_post, float &_sample_rate_offset, float &_phase_offset);

    enum id_show{
        p2_l1_pre = 0,
        p2_l1_post,
        p2_data,
    };
    volatile int id_show = p2_l1_pre;

signals:
    void replace_spectrograph(const int _len_data, complex* _data);
    void replace_constelation(const int _len_data, complex* _data);
    void replace_oscilloscope(const int _len_data, complex* _data);
    void view_l1_presignalling(QString _text);
    void view_l1_postsignalling(QString _text);
    void view_l1_dynamic(QString _text, bool _force_update);

private:
    int fft_size;
    float amp_p2;
    int n_p2;
    int c_p2;
    int* p2_carrier_map;
    int k_total;
    int half_total;
    int left_nulls;
    float** p2_pilot_refer;
    complex buffer_cell[14];
    complex* deinterleaved_cell = nullptr;
    int* h_even_p2;
    int* h_odd_p2;

    int l1_size;
    l1_presignalling l1_pre;
    l1_postsignalling l1_post;

    int idx_l1_post_fef_shift = 0;
    int idx_l1_post_rf_shift = 0;
    int idx_l1_post_plp_shift = 0;
    int idx_l1_post_aux_shift = 0;
    int idx_l1_post_configurable_shift = 0;
    int idx_l1_post_dyn_plp_shift = 0;
    int idx_l1_post_dyn_aux_shift = 0;
    int idx_l1_post_dyn_shift = 0;
    unsigned char l1_randomize[KBCH_1_2];
    void init_l1_randomizer();
    bool l1_pre_info(dvbt2_parameters &_dvbt2);

    QString text_l1_post;
    bool l1_post_info();
    unsigned char* l1_post_bit = nullptr;
    unsigned char* l1_post_bit_interleaving = nullptr;
    bool chek_l1_post = false;
    bool view_l1_post_update = true;
    void time_frequency_slicing_info(unsigned char *bit, l1_postsignalling &l1);
    void rf_info(unsigned char *bit, l1_postsignalling &l1);
    void plp_info(unsigned char *bit, l1_postsignalling &l1);
    void fef_info(unsigned char *bit, l1_postsignalling &l1);
    void aux_info(unsigned char *bit, l1_postsignalling &l1);

    QString text_l1_dynamic;
    void dyn_info(unsigned char *bit, l1_postsignalling &l1);
    void dyn_plp_info(unsigned char *bit, l1_postsignalling &l1);
    void dyn_aux_info(unsigned char *bit, l1_postsignalling &l1);
    void dyn_next_info(unsigned char *bit, l1_postsignalling &l1);
    void dyn_next_plp_info(unsigned char *bit, l1_postsignalling &l1);
    void dyn_next_aux_info(unsigned char *bit, l1_postsignalling &l1);

    complex* show_symbol = nullptr;
    complex* show_data = nullptr;
    complex* est_data = nullptr;
    complex* show_est_data = nullptr;
};
#endif // P2_SYMBOL_H
