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
#ifndef BCH_DECODER_H
#define BCH_DECODER_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "dvbt2_definition.h"
#include "bb_de_header.h"

class bch_decoder : public QObject
{
    Q_OBJECT
public:
    explicit bch_decoder(QWaitCondition* _signal_in, QMutex* _mutex_in, QObject *parent = nullptr);
    ~bch_decoder();
    bb_de_header* deheader;

signals:
    void bit_descramble(int _plp_id, l1_postsignalling _l1_post,int _lenout, uint8_t* out);
    void check(int _len, uint8_t* out);
    void stop_deheader();
    void finished();

public slots:
    void execute(int* _idx_plp_simd, l1_postsignalling _l1_post, int _len_in, uint8_t* _in);
    void stop();

private:
    QWaitCondition* signal_in;
    QThread* thread;
    QWaitCondition* signal_out;
    QMutex* mutex_in;
    QMutex* mutex_out;
    uint8_t* out = nullptr;
    uint8_t* buffer_a = nullptr;
    uint8_t* buffer_b = nullptr;
    bool swap_buffer = true;
    uint8_t descrambler[FEC_SIZE_NORMAL];
    void init_descrambler();

    unsigned int kbch;
    unsigned int nbch;
//    unsigned int bch_code;
};

#endif // BCH_DECODER_H
