#include "rx_airspy.h"

//----------------------------------------------------------------------------------------------------------------------------
rx_airspy::rx_airspy(QObject *parent) : QObject(parent)
{

}
//----------------------------------------------------------------------------------------------------------------------------
rx_airspy::~rx_airspy()
{

}
//----------------------------------------------------------------------------------------------------------------------------
string rx_airspy::error (int err)
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
        return "error libusb";
    case AIRSPY_ERROR_THREAD:
        return "error thread";
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
//----------------------------------------------------------------------------------------------------------------------------
int rx_airspy::get(string &_ser_no, string &_hw_ver)
{
    int count = 1;
    err = airspy_list_devices(serials, count);

    if( err < 0 ) return err;

    _ser_no = std::to_string(serials[0]);

    err = airspy_open_sn(&device, serials[0]);

    if( err < 0 ) return err;

    const uint8_t len = 128;
    char version[len];
    err = airspy_version_string_read(device, version, len);

    if( err < 0 ) return err;

    for(int i = 6; i < len; ++i){
        if(version[i] == '\u0000') break;
        _hw_ver += version[i];
    }

    return err;
}
//----------------------------------------------------------------------------------------------------------------------------
int rx_airspy::init(uint32_t _rf_frequence_hz, int _gain)
{

    err = airspy_set_sample_type(device, AIRSPY_SAMPLE_INT16_IQ);

    if( err < 0 ) return err;

    sample_rate = 1.0e+7f;

    err = airspy_set_samplerate(device, static_cast<int>(sample_rate));

    if( err < 0 ) return err;

    uint8_t biast_val = 0;
    err = airspy_set_rf_bias(device, biast_val);

    if( err < 0 ) return err;

    gain = _gain;
    if(gain < 0) {
        gain = 0;
        agc = true;
    }
    err =  airspy_set_sensitivity_gain(device, static_cast<uint8_t>(gain));

    if( err < 0 ) return err;

    rf_frequence = _rf_frequence_hz;
    err = airspy_set_freq(device, rf_frequence);

    if( err < 0 ) return err;

    emit radio_frequency(rf_frequence);
    emit level_gain(gain);

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

    return err;
}
//----------------------------------------------------------------------------------------------------------------------------
void rx_airspy::start()
{
    err = airspy_start_rx(device, rx_callback, this);

    if(err < 0) emit status(err);
}
//----------------------------------------------------------------------------------------------------------------------------
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
    if(transfer->dropped_samples > 0) fprintf(stderr, "dropped_samples: %ld\n", transfer->dropped_samples);

    return 0;
}
//----------------------------------------------------------------------------------------------------------------------------
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
            err = airspy_set_freq(device, rf_frequence);
            if(err != 0) emit status(err);
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
            err =  airspy_set_sensitivity_gain(device, static_cast<uint8_t>(gain));
            if(err != 0) emit status(err);
            start_wait_gain_changed = clock();
            emit level_gain(gain);
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
            fprintf(stderr, "reset buffer blocks: %d\n", blocks);
            blocks = 1;
            len_buffer = 0;
            if(swap_buffer) ptr_buffer = buffer_a;
            else ptr_buffer = buffer_b;
        }
    }
}
//----------------------------------------------------------------------------------------------------------------------------
void rx_airspy::stop()
{
    airspy_close(device);
    emit stop_demodulator();
    if(thread->isRunning()) thread->wait(1000);
    emit finished();
}
//----------------------------------------------------------------------------------------------------------------------------

