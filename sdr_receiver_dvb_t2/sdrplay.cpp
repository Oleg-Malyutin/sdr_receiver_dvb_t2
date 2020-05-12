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
#include "sdrplay.h"

#include <QThread>
#include <QWaitCondition>
#include <QMutex>

//-----------------------------------------------------------------------------------------------
sdrplay::sdrplay(QObject *parent) : QObject(parent)
{  

}
//-----------------------------------------------------------------------------------------------
sdrplay::~sdrplay()
{

}
//-----------------------------------------------------------------------------------------------
string	sdrplay::error_sdrplay (int err)
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
//-----------------------------------------------------------------------------------------------
mir_sdr_ErrT sdrplay::get_sdrplay(char* &_ser_no, unsigned char &_hw_ver)
{

//    mir_sdr_DebugEnable(1);

    mir_sdr_DeviceT devices[4];
    unsigned int numDevs;
    sdrplay_error = mir_sdr_GetDevices(&devices[0], &numDevs, 4);

    if(sdrplay_error != 0) return sdrplay_error;

    _ser_no = devices[0].SerNo;
    _hw_ver = devices[0].hwVer;
    sdrplay_error = mir_sdr_SetDeviceIdx(0);

    if(sdrplay_error != 0) return sdrplay_error;

    return sdrplay_error;
}
//-----------------------------------------------------------------------------------------------
mir_sdr_ErrT sdrplay::init_sdrplay(double _rf_frequence, int _gain_db)
{
    mir_sdr_Uninit();
    sdrplay_error = mir_sdr_DCoffsetIQimbalanceControl(0, 0);

    rf_frequence = _rf_frequence;
    gain_db = _gain_db;
    if(gain_db < 0) {
        gain_db = 78;
        agc = true;
    }
    sample_rate = 9200000.0f; // max for 10bit (10000000.0f for 8bit)
    double sample_rate_mhz = static_cast<double>(sample_rate) / 1.0e+6;
    double rf_chanel_mhz = static_cast<double>(rf_frequence) / 1.0e+6;
    sdrplay_error = mir_sdr_Init(gain_db, sample_rate_mhz, rf_chanel_mhz,
                                 mir_sdr_BW_8_000, mir_sdr_IF_Zero, &len_out_device);

    if(sdrplay_error != 0) return sdrplay_error;

    max_len_out = len_out_device * max_blocks;
    unsigned int len_buffer = static_cast<unsigned int>(max_len_out);
    i_buffer_a = new short[len_buffer];
    q_buffer_a = new short[len_buffer];
    i_buffer_b = new short[len_buffer];
    q_buffer_b = new short[len_buffer];

    mutex_out = new QMutex;
    signal_out = new QWaitCondition;

    frame = new dvbt2_frame(signal_out, mutex_out, max_len_out, len_out_device, sample_rate);
    thread = new QThread;
    frame->moveToThread(thread);
    connect(this, &sdrplay::execute, frame, &dvbt2_frame::execute);
    connect(this, &sdrplay::stop_demodulator, frame, &dvbt2_frame::stop);
    connect(frame, &dvbt2_frame::finished, frame, &dvbt2_frame::deleteLater);
    connect(frame, &dvbt2_frame::finished, thread, &QThread::quit, Qt::DirectConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

    return sdrplay_error;
}
//-----------------------------------------------------------------------------------------------
void sdrplay::start()
{
    short* ptr_i_bubber = i_buffer_a;
    short* ptr_q_buffer = q_buffer_a;
    unsigned int first_sample_num;
    int gr_changed = 0;
    int rf_changed = 0;
    int fs_changed = 0;

    float frequency_offset = 0.0f;
    bool change_frequency = false;
    bool frequency_changed = true;

    int gain_offset = 0;
    bool change_gain = false;
    bool gain_changed = true;    

    int  blocks = 1;
    const int norm_blocks = 500;
    const int decrease_blocks = 100;
    int len_buffer = 0;
    emit radio_frequency(rf_frequence);
    emit device_gain(gain_db);
    while(done) {
        // get samples
        for(int n = 0; n < blocks; ++n) {
            sdrplay_error = mir_sdr_ReadPacket(ptr_i_bubber, ptr_q_buffer, &first_sample_num,
                                               &gr_changed, &rf_changed, &fs_changed);
            if(sdrplay_error != 0) emit sdrplay_status(sdrplay_error);
            if(rf_changed) {
                rf_changed = 0;
                frequency_changed = true;
                emit radio_frequency(rf_frequence);
            }
            if(gr_changed) {
                gr_changed = 0;
                gain_changed = true;
                emit device_gain(gain_db);
            }
            len_buffer += len_out_device;
            ptr_i_bubber += len_out_device;
            ptr_q_buffer += len_out_device;
        }

        if(mutex_out->try_lock()) {

            frame->get_signal_estimate(change_frequency, frequency_offset,
                                       change_gain, gain_offset);

            // coarse frequency setting
            if(change_frequency) {
                float correct = -frequency_offset / static_cast<float>(rf_frequence);
                frame->correct_resample(correct);
                rf_frequence += static_cast<double>(frequency_offset);
                sdrplay_error = mir_sdr_SetRf(rf_frequence, 1, 0);
                if(sdrplay_error != 0) emit sdrplay_status(sdrplay_error);
                frequency_changed = false;
            }
            // AGC
            if(agc && change_gain) {
                gain_db -= gain_offset;
                sdrplay_error = mir_sdr_SetGr(gain_db, 1, 0);
                if(sdrplay_error != 0) emit sdrplay_status(sdrplay_error);
                gain_changed = false;
            }
            if(swap_buffer) {
                swap_buffer = false;
                emit execute(len_buffer, i_buffer_a, q_buffer_a, frequency_changed, gain_changed);
                mutex_out->unlock();
                ptr_i_bubber = i_buffer_b;
                ptr_q_buffer = q_buffer_b;
            }
            else {
                swap_buffer = true;
                emit execute(len_buffer, i_buffer_b, q_buffer_b, frequency_changed, gain_changed);
                mutex_out->unlock();
                ptr_i_bubber = i_buffer_a;
                ptr_q_buffer = q_buffer_a;
            }
            len_buffer = 0;
            if(blocks > norm_blocks) blocks -= decrease_blocks;
        }
        else {
            blocks += 1;
            int remain = max_len_out - len_buffer;
            int need = len_out_device * blocks;
            if(need > remain){
                len_buffer = 0;
                qDebug() << "reset buffer  ";
                if(swap_buffer) {
                    ptr_i_bubber = i_buffer_b;
                    ptr_q_buffer = q_buffer_b;
                }
                else {
                    ptr_i_bubber = i_buffer_a;
                    ptr_q_buffer = q_buffer_a;
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
//-----------------------------------------------------------------------------------------------
void sdrplay::stop()
{
    done = false;
}
//-----------------------------------------------------------------------------------------------
