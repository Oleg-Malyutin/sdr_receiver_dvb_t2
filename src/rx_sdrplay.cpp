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
#include "rx_sdrplay.h"

#include <QThread>
#include <QWaitCondition>
#include <QMutex>

//-------------------------------------------------------------------------------------------
rx_sdrplay::rx_sdrplay(QObject *parent) : QObject(parent)
{  

}
//-------------------------------------------------------------------------------------------
rx_sdrplay::~rx_sdrplay()
{

}
//-------------------------------------------------------------------------------------------
std::string rx_sdrplay::error (int err)
{
    switch (err) {
       case mir_sdr_Success:
          return "Success";
       case mir_sdr_Fail:
          return "Fail";
       case mir_sdr_InvalidParam:
          return "Invalid parameter";
       case mir_sdr_OutOfRange:
          return "Out of range";
       case mir_sdr_GainUpdateError:
          return "Gain update error";
       case mir_sdr_RfUpdateError:
          return "Rf update error";
       case mir_sdr_FsUpdateError:
          return "Fs update error";
       case mir_sdr_HwError:
          return "Hardware error";
       case mir_sdr_AliasingError:
          return "Aliasing error";
       case mir_sdr_AlreadyInitialised:
          return "Already initialised";
       case mir_sdr_NotInitialised:
          return "Not initialised";
       case mir_sdr_NotEnabled:
          return "Not enabled";
       case mir_sdr_HwVerError:
          return "Hardware Version error";
       case mir_sdr_OutOfMemError:
          return "Out of memory error";
       case mir_sdr_HwRemoved:
          return "Hardware removed";
       default:
          return "Unknown error";
    }
}
//-------------------------------------------------------------------------------------------
mir_sdr_ErrT rx_sdrplay::get(char* &_ser_no, unsigned char &_hw_ver)
{
    mir_sdr_ErrT err;
//    mir_sdr_DebugEnable(1);

    mir_sdr_DeviceT devices[4];
    unsigned int numDevs;
    err = mir_sdr_GetDevices(&devices[0], &numDevs, 4);

    if(err != 0) return err;

    _ser_no = devices[0].SerNo;
    _hw_ver = devices[0].hwVer;
    err = mir_sdr_SetDeviceIdx(0);

    return err;
}
//-------------------------------------------------------------------------------------------
mir_sdr_ErrT rx_sdrplay::init(double _rf_frequence, int _gain_db)
{
    mir_sdr_ErrT err;

    mir_sdr_Uninit();
    err = mir_sdr_DCoffsetIQimbalanceControl(0, 0);

    if(err != 0) return err;

    ch_frequency = _rf_frequence;
    rf_frequency = ch_frequency;
    gain_db = _gain_db;
    if(gain_db < 0) {
        gain_db = 78;
        agc = true;
    }
    sample_rate = 9200000.0f; // max for 10bit (10000000.0f for 8bit)
    double sample_rate_mhz = static_cast<double>(sample_rate) / 1.0e+6;
    double rf_chanel_mhz = static_cast<double>(rf_frequency) / 1.0e+6;
    err = mir_sdr_Init(gain_db, sample_rate_mhz, rf_chanel_mhz,
                                 mir_sdr_BW_8_000, mir_sdr_IF_Zero, &len_out_device);

    if(err != 0) return err;

    max_len_out = len_out_device * max_blocks;
    i_buffer_a = new short[max_len_out];
    q_buffer_a = new short[max_len_out];
    i_buffer_b = new short[max_len_out];
    q_buffer_b = new short[max_len_out];

    demodulator = new dvbt2_demodulator(id_sdrplay, sample_rate);
    thread = new QThread;
    demodulator->moveToThread(thread);
    connect(this, &rx_sdrplay::execute, demodulator, &dvbt2_demodulator::execute);
    connect(this, &rx_sdrplay::stop_demodulator, demodulator, &dvbt2_demodulator::stop);
    connect(demodulator, &dvbt2_demodulator::finished, demodulator, &dvbt2_demodulator::deleteLater);
    connect(demodulator, &dvbt2_demodulator::finished, thread, &QThread::quit, Qt::DirectConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

    signal = new signal_estimate;

    reset();

    return err;
}
//-------------------------------------------------------------------------------------------
void rx_sdrplay::reset()
{
    signal->reset = false;
    rf_frequency = ch_frequency;
    signal->coarse_freq_offset = 0.0;
    signal->change_frequency = true;
    signal->correct_resample = 0.0;
    if(agc) {
        gain_db = 78;
    }
    signal->gain_offset = 0;
    signal->change_gain = true;
    ptr_i_bubber = i_buffer_a;
    ptr_q_buffer = q_buffer_a;
    swap_buffer = true;
    len_buffer = 0;
    blocks = norm_blocks;;
    set_rf_frequency();
    set_gain();

    qDebug() << "rx_sdrplay::reset";
}
//-------------------------------------------------------------------------------------------
void rx_sdrplay::set_rf_frequency()
{
    if(!signal->frequency_changed){
        signal->frequency_changed = frequency_changed;
    }
    if(signal->change_frequency) {
        signal->change_frequency = false;
        frequency_changed = false;
        signal->frequency_changed = false;
        signal->correct_resample = signal->coarse_freq_offset / rf_frequency;
        rf_frequency += signal->coarse_freq_offset;
        mir_sdr_ErrT err = mir_sdr_SetRf(rf_frequency, 1, 0);
        if(err != 0) {
            emit status(err);
        }
        else{
            emit radio_frequency(rf_frequency);
        }
    }
}
//-------------------------------------------------------------------------------------------
void rx_sdrplay::set_gain()
{
    if(!signal->gain_changed){
        signal->gain_changed = gain_changed;
    }
    if(agc && signal->change_gain) {
        signal->change_gain = false;
        gain_changed = false;
        signal->gain_changed = false;
        gain_db -= signal->gain_offset;
        mir_sdr_ErrT err = mir_sdr_SetGr(gain_db, 1, 0);
        if(err != 0) {
            emit status(err);
        }
        else{
            emit level_gain(gain_db);
        }
    }
}
//-------------------------------------------------------------------------------------------
void rx_sdrplay::start()
{
    ptr_i_bubber = i_buffer_a;
    ptr_q_buffer = q_buffer_a;
    unsigned int first_sample_num;
    int gr_changed = 0;
    int rf_changed = 0;
    int fs_changed = 0;   

    while(done) {

        for(int n = 0; n < blocks; ++n) {

            mir_sdr_ErrT err = mir_sdr_ReadPacket(ptr_i_bubber, ptr_q_buffer, &first_sample_num,
                                     &gr_changed, &rf_changed, &fs_changed);
            if(err != 0) {
                emit status(err);
            }
            if(rf_changed) {
                rf_changed = 0;
                frequency_changed = true;
            }
            if(gr_changed) {
                gr_changed = 0;
                gain_changed = true;
            }
            len_buffer += len_out_device;
            ptr_i_bubber += len_out_device;
            ptr_q_buffer += len_out_device;
        }

        if(demodulator->mutex->try_lock()) {

            if(signal->reset){
                reset();

                demodulator->mutex->unlock();

                continue;

            }
            // coarse frequency setting
            set_rf_frequency();
            // AGC
            set_gain();

            if(swap_buffer) {
                emit execute(len_buffer, i_buffer_a, q_buffer_a, signal);
                ptr_i_bubber = i_buffer_b;
                ptr_q_buffer = q_buffer_b;
            }
            else {
                emit execute(len_buffer, i_buffer_b, q_buffer_b, signal);
                ptr_i_bubber = i_buffer_a;
                ptr_q_buffer = q_buffer_a;
            }
            swap_buffer = !swap_buffer;
            len_buffer = 0;
            blocks = norm_blocks;

            demodulator->mutex->unlock();

        }
        else {
            ++blocks;
            int remain = max_len_out - len_buffer;
            int need = len_out_device * blocks;
            if(need > remain){
                len_buffer = 0;
                blocks = norm_blocks;;
                fprintf(stderr, "reset buffer blocks: %d\n", blocks);
                if(swap_buffer) {
                    ptr_i_bubber = i_buffer_a;
                    ptr_q_buffer = q_buffer_a;
                }
                else {
                    ptr_i_bubber = i_buffer_b;
                    ptr_q_buffer = q_buffer_b;
                }
            }
        }
    }

    mir_sdr_Uninit();
    mir_sdr_ReleaseDeviceIdx();
    emit stop_demodulator();
    if(thread->isRunning()) thread->wait(1000);
    delete [] i_buffer_a;
    delete [] q_buffer_a;
    delete [] i_buffer_b;
    delete [] q_buffer_b;
    emit finished();
}
//-------------------------------------------------------------------------------------------
void rx_sdrplay::stop()
{
    done = false;
}
//-------------------------------------------------------------------------------------------
