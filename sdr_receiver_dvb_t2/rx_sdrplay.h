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
#ifndef RX_SDRPLAY_H
#define RX_SDRPLAY_H

#include <QObject>
#include <QTime>
#include <QApplication>
#include <string>
#include <mirsdrapi-rsp.h>

#include "DVB_T2/dvbt2_frame.h"

typedef std::string string;

class rx_sdrplay : public QObject
{
    Q_OBJECT
public:
    explicit rx_sdrplay(QObject* parent = nullptr);
    ~rx_sdrplay();

    string error (int err);
    mir_sdr_ErrT get(char *&_ser_no, unsigned char &_hw_ver);
    mir_sdr_ErrT init(double _rf_frequence, int _gain_db);
    dvbt2_frame* frame;

signals:
    void execute(int _len_in, short* _i_in, short* _q_in, bool _frequence_changed, bool _gain_changed);
    void status(int _err);
    void radio_frequency(double _rf);
    void level_gain(int _gain);
    void stop_demodulator();
    void finished();

public slots:
    void start();
    void stop();

private:
    QThread* thread;
    QWaitCondition* signal_out;
    QMutex* mutex_out;
    mir_sdr_ErrT err;
    int gain_db;
    double rf_frequence;
    float sample_rate;
    int len_out_device;
    const int max_blocks = 24576 * 2;//12288;//768
    int max_len_out;
    short* i_buffer_a;
    short* q_buffer_a;
    short* i_buffer_b;
    short* q_buffer_b;
    bool swap_buffer = true;
    bool agc = false;
    bool done = true;
};

#endif // RX_SDRPLAY_H
