#include "rx_airspy.h"

//-----------------------------------------------------------------------------------------------
rx_airspy::rx_airspy(QObject *parent) : QObject(parent)
{

}
//-----------------------------------------------------------------------------------------------
rx_airspy::~rx_airspy()
{

}
//-----------------------------------------------------------------------------------------------
string	rx_airspy::error_airspy (int err)
{
    switch (err) {
    case AIRSPY_SUCCESS:
        return "Success";
    case AIRSPY_TRUE:
        return "True";
    case AIRSPY_ERROR_INVALID_PARAM:
        return "Invalid parameter";
    case AIRSPY_ERROR_NOT_FOUND:
        return "Not found";
    case AIRSPY_ERROR_BUSY:
        return "Busy";
    case AIRSPY_ERROR_NO_MEM:
        return "No memory";
    case AIRSPY_ERROR_LIBUSB:
        return "Error libusb";
    case AIRSPY_ERROR_THREAD:
        return "Error thread";
    case AIRSPY_ERROR_STREAMING_THREAD_ERR:
        return "Streaming thread error";
    case AIRSPY_ERROR_STREAMING_STOPPED:
        return "Streaming stopped";
    case AIRSPY_ERROR_OTHER:
        return "Unknown error";
    default:
        return std::to_string(err);
    }
}
//-----------------------------------------------------------------------------------------------
int rx_airspy::get_airspy(string &_ser_no, string &_hw_ver)
{
    int count = 1;
    airspy_error = airspy_list_devices(serials, count);

    if( airspy_error < 0 ) return airspy_error;

    _ser_no = std::to_string(serials[0]);

    airspy_error = airspy_open_sn(&device, serials[0]);

    if( airspy_error < 0 ) return airspy_error;

    const uint8_t len = 128;
    char version[len];
    airspy_error = airspy_version_string_read(device, version, len);

    if( airspy_error < 0 ) return airspy_error;

    for(int i = 6; i < len; ++i){
        if(version[i] == '\u0000') break;
        _hw_ver += version[i];
    }

    return airspy_error;
}
//-----------------------------------------------------------------------------------------------
int rx_airspy::init_airspy(uint32_t _rf_frequence_hz, int _gain)
{

    airspy_error = airspy_set_sample_type(device, AIRSPY_SAMPLE_INT16_IQ);

    if( airspy_error < 0 ) return airspy_error;

    sample_rate = 1.0e+7f;

    airspy_error = airspy_set_samplerate(device, static_cast<int>(sample_rate));

    if( airspy_error < 0 ) return airspy_error;

    uint8_t biast_val = 0;
    airspy_error = airspy_set_rf_bias(device, biast_val);

    if( airspy_error < 0 ) return airspy_error;

    gain = _gain;
    if(gain < 0) {
        gain = 0;
        agc = true;
    }
    airspy_error =  airspy_set_sensitivity_gain(device, static_cast<uint8_t>(gain));

    if( airspy_error < 0 ) return airspy_error;

    rf_frequence = _rf_frequence_hz;
    airspy_error = airspy_set_freq(device, rf_frequence);

    if( airspy_error < 0 ) return airspy_error;

    emit radio_frequency(rf_frequence);
    emit device_gain(gain);

    int len_out_device = 65536 * 2;
    int max_len_out = len_out_device * max_blocks;

    buffer_a = new int16_t[max_len_out];
    buffer_b = new int16_t[max_len_out];
    ptr_buffer = buffer_a;

    mutex_out = new QMutex;
    signal_out = new QWaitCondition;

    frame = new dvbt2_frame(signal_out, mutex_out, id_airspy, max_len_out, len_out_device, sample_rate);
    thread = new QThread;
    frame->moveToThread(thread);
    connect(this, &rx_airspy::execute, frame, &dvbt2_frame::execute);
    connect(this, &rx_airspy::stop_demodulator, frame, &dvbt2_frame::stop);
    connect(frame, &dvbt2_frame::finished, frame, &dvbt2_frame::deleteLater);
    connect(frame, &dvbt2_frame::finished, thread, &QThread::quit, Qt::DirectConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

    return airspy_error;
}
//-----------------------------------------------------------------------------------------------
void rx_airspy::start()
{
    airspy_error = airspy_start_rx(device, rx_callback, this);

    if(airspy_error < 0) emit airspy_status(airspy_error);

    qDebug() << "airspy_start_rx airspy_error "<< airspy_error;
}
//-----------------------------------------------------------------------------------------------
int rx_airspy::rx_callback(airspy_transfer_t* transfer)
{
    if(!transfer) return 0;

    int len_out_device;
    int16_t* ptr_rx_buffer;
    len_out_device = transfer->sample_count  * 2;
    ptr_rx_buffer = static_cast<int16_t*>(transfer->samples);
    rx_airspy* ctx;
    ctx = static_cast<rx_airspy*>(transfer->ctx);
    ctx->rx_execute(ptr_rx_buffer, len_out_device);
    if(transfer->dropped_samples > 0) qDebug() << "dropped_samples: " << transfer->dropped_samples;

    return 0;
}
//-----------------------------------------------------------------------------------------------
void rx_airspy::rx_execute(int16_t* _ptr_rx_buffer, int _len_out_device)
{
    int len_out_device = _len_out_device;
    for(int i = 0; i < len_out_device; ++i) ptr_buffer[i] = _ptr_rx_buffer[i];
    len_buffer += len_out_device / 2;
    ptr_buffer += len_out_device;

    if(mutex_out->try_lock()) {
        frame->get_signal_estimate(change_frequency, frequency_offset,
                                   change_gain, gain_offset);
        // coarse frequency setting
        if(!frequency_changed) {
            end_wait_frequency_changed = clock();
            float mseconds = end_wait_frequency_changed - start_wait_frequency_changed;
            if(mseconds > 100000) frequency_changed = true;
        }
        if(change_frequency) {
            float correct = -frequency_offset / static_cast<float>(rf_frequence);
            frame->correct_resample(correct);
            rf_frequence += static_cast<uint32_t>(frequency_offset);
            airspy_error = airspy_set_freq(device, rf_frequence);
            if(airspy_error != 0) emit airspy_status(airspy_error);
            frequency_changed = false;
            emit radio_frequency(rf_frequence);
            start_wait_frequency_changed = clock();
        }
        // AGC
        if(!gain_changed) {
            end_wait_gain_changed = clock();
            float mseconds = end_wait_gain_changed - start_wait_gain_changed;
            if(mseconds > 100000) gain_changed = true;
        }
        if(agc && change_gain) {
            gain_changed = false;
            gain += gain_offset;
            airspy_error =  airspy_set_sensitivity_gain(device, static_cast<uint8_t>(gain));
            if(airspy_error != 0) emit airspy_status(airspy_error);
            start_wait_gain_changed = clock();
            emit device_gain(gain);
        }
        if(swap_buffer) {
            swap_buffer = false;
            emit execute(len_buffer, &buffer_a[0], &buffer_a[1], frequency_changed, gain_changed);
            mutex_out->unlock();
            ptr_buffer = buffer_b;
        }
        else {
            swap_buffer = true;
            emit execute(len_buffer, &buffer_b[0], &buffer_b[1], frequency_changed, gain_changed);
            mutex_out->unlock();
            ptr_buffer = buffer_a;
        }
        len_buffer = 0;
        blocks = 1;
    }
    else {
        ++blocks;
        if(blocks > max_blocks){
            qDebug() << "reset buffer:  blocks" << blocks;
            blocks = 1;
            len_buffer = 0;
            if(swap_buffer) ptr_buffer = buffer_a;
            else ptr_buffer = buffer_b;
        }
    }
}
//-----------------------------------------------------------------------------------------------
void rx_airspy::stop()
{
    airspy_close(device);
    emit stop_demodulator();
    if(thread->isRunning()) thread->wait(1000);
    emit finished();
}
//-----------------------------------------------------------------------------------------------

