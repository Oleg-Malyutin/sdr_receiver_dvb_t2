#include "rx_airspy.h"

//-------------------------------------------------------------------------------------------
rx_airspy::rx_airspy(QObject *parent) : QObject(parent)
{

}
//-------------------------------------------------------------------------------------------
rx_airspy::~rx_airspy()
{

}
//-------------------------------------------------------------------------------------------
std::string rx_airspy::error (int err)
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
//-------------------------------------------------------------------------------------------
int rx_airspy::get(std::string &_ser_no, std::string &_hw_ver)
{
    int err;

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
//-------------------------------------------------------------------------------------------
int rx_airspy::init(uint32_t _rf_frequence_hz, int _gain)
{
    int err;

    err = airspy_set_sample_type(device, AIRSPY_SAMPLE_INT16_IQ);

    if( err < 0 ) return err;

    sample_rate = 1.0e+7f;

    err = airspy_set_samplerate(device, static_cast<uint32_t>(sample_rate));

    if( err < 0 ) return err;

    uint8_t biast_val = 0;
    err = airspy_set_rf_bias(device, biast_val);

    if( err < 0 ) return err;

    gain = _gain;
    if(gain < 0) {
        agc = true;
    }
    else{
        err =  airspy_set_sensitivity_gain(device, static_cast<uint8_t>(gain));

        if( err < 0 ) return err;
    }

    ch_frequency = _rf_frequence_hz;
    rf_frequency = ch_frequency;

    uint len_out_device = 65536 * 2;
    uint max_len_out = len_out_device * max_blocks;

    buffer_a = new int16_t[max_len_out];
    buffer_b = new int16_t[max_len_out];

    signal = new signal_estimate;

    demodulator = new dvbt2_demodulator(id_airspy, sample_rate);
    thread = new QThread;
    demodulator->moveToThread(thread);
    connect(this, &rx_airspy::execute, demodulator, &dvbt2_demodulator::execute);
    connect(this, &rx_airspy::stop_demodulator, demodulator, &dvbt2_demodulator::stop);
    connect(demodulator, &dvbt2_demodulator::finished, demodulator, &dvbt2_demodulator::deleteLater);
    connect(demodulator, &dvbt2_demodulator::finished, thread, &QThread::quit, Qt::DirectConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start(QThread::TimeCriticalPriority);

    return err;
}
//-------------------------------------------------------------------------------------------
void rx_airspy::start()
{
    reset();
    int err;
    err = airspy_start_rx(device, rx_callback, this);
    if(err < 0) emit status(err);
}
//-------------------------------------------------------------------------------------------
void rx_airspy::reset()
{
    signal->reset = false;
    rf_frequency = ch_frequency;
    signal->coarse_freq_offset = 0.0;
    signal->change_frequency = true;
    signal->correct_resample = 0.0;
    if(agc) {
        gain = 0;
    }
    signal->gain_offset = 0;
    signal->change_gain = true;
    ptr_buffer = buffer_a;
    swap_buffer = true;
    len_buffer = 0;
    blocks = 1;
    set_rf_frequency();
    set_gain();
}
//-------------------------------------------------------------------------------------------
void rx_airspy::set_rf_frequency()
{
    if(!signal->frequency_changed) {
        end_wait_frequency_changed = clock();
        float mseconds = (end_wait_frequency_changed - start_wait_frequency_changed) /
                         (CLOCKS_PER_SEC / 1000);
        if(mseconds > 100) {
            signal->frequency_changed = true;
            emit radio_frequency(rf_frequency);
        }
    }
    if(signal->change_frequency) {
        signal->change_frequency = false;
        signal->correct_resample = signal->coarse_freq_offset / static_cast<float>(rf_frequency);
        rf_frequency += static_cast<uint32_t>(signal->coarse_freq_offset);
        int err = airspy_set_freq(device, rf_frequency);
        if(err != 0) {
            emit status(err);
        }
        else{
            signal->frequency_changed = false;
            start_wait_frequency_changed = clock();
        }
    }
}
//-------------------------------------------------------------------------------------------
void rx_airspy::set_gain()
{
    if(!signal->gain_changed) {
        end_wait_gain_changed = clock();
        float mseconds = (end_wait_gain_changed - start_wait_gain_changed) /
                         (CLOCKS_PER_SEC / 1000);
        if(mseconds > 10) {
            signal->gain_changed = true;
            emit level_gain(gain);
        }
    }
    if(agc && signal->change_gain) {
        signal->change_gain = false;

        gain += signal->gain_offset;
        if(gain > 21) {
            gain = 0;
        }
        int err =  airspy_set_sensitivity_gain(device, static_cast<uint8_t>(gain));
        if(err != 0) {
            emit status(err);
        }
        else{
            signal->gain_changed = false;
            start_wait_gain_changed = clock();
        }
    }
}
//-------------------------------------------------------------------------------------------
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
    if(transfer->dropped_samples > 0) {
        fprintf(stderr, "dropped_samples: %ld\n", transfer->dropped_samples);
    }

    return 0;
}
//-------------------------------------------------------------------------------------------
void rx_airspy::rx_execute(int16_t* _ptr_rx_buffer, int _len_out_device)
{
    int len_out_device = _len_out_device;
    for(int i = 0; i < len_out_device; ++i) ptr_buffer[i] = _ptr_rx_buffer[i];
    len_buffer += len_out_device / 2;
    ptr_buffer += len_out_device;

    if(demodulator->mutex->try_lock()) {

        if(signal->reset){
            reset();

            demodulator->mutex->unlock();

            return;

        }
        // coarse frequency setting
        set_rf_frequency();
        // AGC
        set_gain();

        if(swap_buffer) {
            emit execute(len_buffer, &buffer_a[0], &buffer_a[1], signal);
            ptr_buffer = buffer_b;
        }
        else {
            emit execute(len_buffer, &buffer_b[0], &buffer_b[1], signal);
            ptr_buffer = buffer_a;
        }
        swap_buffer = !swap_buffer;
        len_buffer = 0;
        blocks = 1;

        demodulator->mutex->unlock();
    }
    else {
        ++blocks;
        if(blocks > max_blocks){
            fprintf(stderr, "reset buffer blocks: %d\n", blocks);
            blocks = 1;
            len_buffer = 0;
            if(swap_buffer) {
                ptr_buffer = buffer_a;
            }
            else {
                ptr_buffer = buffer_b;
            }
        }
    }
}
//-------------------------------------------------------------------------------------------
void rx_airspy::stop()
{
    airspy_close(device);
    emit stop_demodulator();
    if(thread->isRunning()) {
        thread->wait(1000);
    }
    emit finished();
}
//-------------------------------------------------------------------------------------------

