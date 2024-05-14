#include "rx_plutosdr.h"

#include <QFile>
#include <QThread>
#include <errno.h>
#include <chrono>
#include <sys/stat.h>

#include "libssh/libssh.h"

#define DEFAULT_SAMPLE_RATE 9200000
//enum plutosdr_error
//{
//    PLUTOSDR_SUCCESS                = 0,
//    PLUTOSDR_TRUE                   = 1,
//    PLUTOSDR_ERROR_INVALID_PARAM    = -2,
//    PLUTOSDR_ERROR_ACCESS           = -3,
//    PLUTOSDR_ERROR_NO_DEVICE        = -4,
//    PLUTOSDR_ERROR_NOT_FOUND        = -5,
//    PLUTOSDR_ERROR_BUSY             = -6,
//    PLUTOSDR_ERROR_NO_MEM           = -11,
//    PLUTOSDR_ERROR_LIBUSB           = -99,
//    PLUTOSDR_ERROR_THREAD           = -1001,
//    PLUTOSDR_ERROR_STREAMING_THREAD_ERR = -1002,
//    PLUTOSDR_ERROR_STREAMING_STOPPED    = -1003,
//    PLUTOSDR_ERROR_OTHER            = -99,
//};
//#define RETING ;
#if defined(_WIN32)
#define S_IRWXU 0x777
#endif

//-------------------------------------------------------------------------------------------
rx_plutosdr::rx_plutosdr(QObject *parent) : QObject(parent)
{

}
//-------------------------------------------------------------------------------------------
std::string rx_plutosdr::error (int _err)
{
    switch (_err) {
    case PLUTOSDR_SUCCESS:
        return "Success";
    case PLUTOSDR_TRUE:
        return "True";
    case PLUTOSDR_ERROR_INVALID_PARAM:
        return "Invalid parameter";
    case PLUTOSDR_ERROR_ACCESS:
        return "Error access";
    case PLUTOSDR_ERROR_NOT_FOUND:
        return "Not found";
    case PLUTOSDR_ERROR_BUSY:
        return "Busy";
    case PLUTOSDR_ERROR_NO_MEM:
        return "No memory";
    case PLUTOSDR_ERROR_LIBUSB:
        return "Error libusb";
    case PLUTOSDR_ERROR_THREAD:
        return "Error thread";
    case PLUTOSDR_ERROR_STREAMING_THREAD_ERR:
        return "Streaming thread error";
    case PLUTOSDR_ERROR_STREAMING_STOPPED:
        return "Streaming stopped";
    case PLUTOSDR_ERROR_OTHER:
        return "Unknown error";
    default:
        return std::to_string(_err);
    }
}
//-------------------------------------------------------------------------------------------
int rx_plutosdr::get(std::string &_ser_no, std::string &_hw_ver)
{
    pluto_kernel_patch();
    int err = 0;
    plutosdr_info_t info;
    info.samples_type = IQ;
    err = plutosdr_open(&device, 0, &info);
    if(err == PLUTOSDR_SUCCESS){
        len_out_device = info.len_out;
        max_len_out = len_out_device * max_blocks;
        for(int i = 0; i < info.serial_number_len; ++i){
            _ser_no += reinterpret_cast<char&>(info.serial_number[i]);
        }
    }

    QThread::sleep(2);

    return err;
}
//-------------------------------------------------------------------------------------------
int rx_plutosdr::init(uint64_t _rf_frequence_hz, int _gain)
{
    int err;
    rf_frequency = _rf_frequence_hz;
    ch_frequency = rf_frequency;
    sample_rate_hz = DEFAULT_SAMPLE_RATE;
    if(_gain < 0) {
        gain_db = 0;
        agc = true;
    }
    else {
        gain_db = static_cast<uint32_t>(_gain);
    }
    // set this first!
    err = plutosdr_set_rfbw(device, 8000000);
    if(err < 0) return err;
    err = plutosdr_set_sample_rate(device, sample_rate_hz);
    if(err < 0) return err;
    err = plutosdr_set_rxlo(device, rf_frequency);
    if(err < 0) return err;
    err = plutosdr_set_gainctl_manual(device);
    if(err < 0) return err;
    err = plutosdr_set_gain_mdb(device, gain_db * 1000);
    if(err < 0) return err;
    //
    err = plutosdr_buffer_channel_enable(device, 0, 1);
    if(err < 0) return err;
    err = plutosdr_buffer_channel_enable(device, 1, 1);
    if(err < 0) return err;
    err = plutosdr_bufstream_enable(device, 1);
    if(err < 0) return err;

    i_buffer_a = new int16_t[max_len_out];
    q_buffer_a = new int16_t[max_len_out];
    i_buffer_b = new int16_t[max_len_out];
    q_buffer_b = new int16_t[max_len_out];

    signal = new signal_estimate;

    demodulator = new dvbt2_demodulator(id_plutosdr, static_cast<float>(sample_rate_hz));
    thread = new QThread;
    demodulator->moveToThread(thread);
    connect(this, &rx_plutosdr::execute, demodulator, &dvbt2_demodulator::execute);
    connect(this, &rx_plutosdr::stop_demodulator, demodulator, &dvbt2_demodulator::stop);
    connect(demodulator, &dvbt2_demodulator::finished, demodulator, &dvbt2_demodulator::deleteLater);
    connect(demodulator, &dvbt2_demodulator::finished, thread, &QThread::quit, Qt::DirectConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();

    return PLUTOSDR_SUCCESS;
}
//-------------------------------------------------------------------------------------------
void rx_plutosdr::start()
{
    reset();
    int err = plutosdr_start_rx(device, plutosdr_callback, this);
    fprintf(stderr, "plutosdr start rx %d\n", err);
}
//-------------------------------------------------------------------------------------------
void rx_plutosdr::reset()
{
    signal->reset = false;
    rf_frequency = ch_frequency;
    signal->coarse_freq_offset = 0.0;
    signal->change_frequency = true;
    signal->correct_resample = 0.0;
    if(agc) {
        gain_db = 0;
    }
    signal->gain_offset = 0;
    signal->change_gain = true;
    ptr_i_buffer = i_buffer_a;
    ptr_q_buffer = q_buffer_a;
    swap_buffer = true;
    len_buffer = 0;
    blocks = 1;
    set_rf_frequency();
    set_gain();
}
//-------------------------------------------------------------------------------------------
void rx_plutosdr::set_rf_frequency()
{
    if(!signal->frequency_changed) {
        end_wait_frequency_changed = clock();
        float mseconds = (end_wait_frequency_changed - start_wait_frequency_changed) /
                         (CLOCKS_PER_SEC / 1000);
        if(mseconds > 200) {
            signal->frequency_changed = true;
            emit radio_frequency(rf_frequency);
        }
    }
    if(signal->change_frequency) {
        signal->change_frequency = false;
        signal->correct_resample = signal->coarse_freq_offset / static_cast<double>(rf_frequency);
        rf_frequency += static_cast<uint64_t>(signal->coarse_freq_offset);
        int err = plutosdr_set_rxlo(device, rf_frequency);
        if(err < 0) {
            emit status(err);
        }
        else{
            signal->frequency_changed = false;
            start_wait_frequency_changed = clock();
        }
    }
}
//-------------------------------------------------------------------------------------------
void rx_plutosdr::set_gain()
{
    if(!signal->gain_changed) {
        end_wait_gain_changed = clock();
        float mseconds = (end_wait_gain_changed - start_wait_gain_changed) /
                         (CLOCKS_PER_SEC / 1000);
        if(mseconds > 70) {
            signal->gain_changed = true;
            emit level_gain(gain_db);
        }
    }
    if(agc && signal->change_gain) {
        signal->change_gain = false;
        gain_db += signal->gain_offset;
        if(gain_db == 71) {
            gain_db = 0;
        }
        int err = plutosdr_set_gain_mdb(device, gain_db * 1000);
        if(err < 0) {
            emit status(err);
        }
        else{
            signal->gain_changed = false;
            start_wait_gain_changed = clock();
        }
    }
}
//-------------------------------------------------------------------------------------------
int rx_plutosdr::plutosdr_callback(plutosdr_transfer* _transfer)
{
    if(!_transfer) return 0;

    int16_t *ptr_rx_i, *ptr_rx_q;
    ptr_rx_i = _transfer->i_samples;
    ptr_rx_q = _transfer->q_samples;
    rx_plutosdr *ctx;
    ctx = static_cast<rx_plutosdr*>(_transfer->ctx);

    ctx->rx_execute(ptr_rx_i, ptr_rx_q);

    if(!ctx->done) return -1;

    return 0;
}
//-------------------------------------------------------------------------------------------
void rx_plutosdr::rx_execute(int16_t* _rx_i, int16_t* _rx_q)
{
    for(unsigned int i = 0; i < len_out_device; ++i) {
        ptr_i_buffer[i] = _rx_i[i];
        ptr_q_buffer[i] = _rx_q[i];
    }
    len_buffer += len_out_device;
    ptr_i_buffer += len_out_device;
    ptr_q_buffer += len_out_device;

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
            emit execute(len_buffer, i_buffer_a, q_buffer_a, signal);
            ptr_i_buffer = i_buffer_b;
            ptr_q_buffer = q_buffer_b;
        }
        else {
            emit execute(len_buffer, i_buffer_b, q_buffer_b, signal);
            ptr_i_buffer = i_buffer_a;
            ptr_q_buffer = q_buffer_a;
        }
        swap_buffer = !swap_buffer;
        len_buffer = 0;
        blocks = 1;

        demodulator->mutex->unlock();

    }
    else {
        ++blocks;
        if(blocks > max_blocks) {
            fprintf(stderr, "reset buffer blocks: %d\n", blocks);
            blocks = 1;
            len_buffer = 0;
            if(swap_buffer) {
                ptr_i_buffer = i_buffer_a;
                ptr_q_buffer = q_buffer_a;
            }
            else {
                ptr_i_buffer = i_buffer_b;
                ptr_q_buffer = q_buffer_b;
            }
        }
    }
}
//-------------------------------------------------------------------------------------------
void rx_plutosdr::stop()
{
    done = false;
    reboot();
    emit stop_demodulator();
    if(thread->isRunning()) thread->wait(1000);
    delete [] i_buffer_a;
    delete [] q_buffer_a;
    delete [] i_buffer_b;
    delete [] q_buffer_b;
    delete signal;
    emit finished();
}
//-------------------------------------------------------------------------------------------
void rx_plutosdr::reboot()
{
    plutosdr_reboot(device);
    plutosdr_close(device);
    QThread::sleep(2);
    fprintf(stderr, "plutosdr close \n");
}
//-------------------------------------------------------------------------------------------
void rx_plutosdr::pluto_kernel_patch()
{
    ssh_session session = ssh_new();
    if (session == nullptr) {
        fprintf(stderr, "Unable to create SSH session \n");
        exit(-1);
    }
    ssh_options_set (session, SSH_OPTIONS_HOST, "root@192.168.3.1" );
    int verbosity = SSH_LOG_PROTOCOL;
    ssh_options_set (session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    int port = 22;
    ssh_options_set(session, SSH_OPTIONS_PORT, &port);
    int rc;
    rc = ssh_connect(session);
    if (rc != SSH_OK) {
        fprintf(stderr, "Error connecting to localhost: %s\n", ssh_get_error(session));
        exit(-1);
    }
    rc = ssh_userauth_password(session, NULL, "analog");
    if (rc != SSH_AUTH_SUCCESS) {
        fprintf(stderr, "Error authenticating with password: %s\n", ssh_get_error(session));
        ssh_disconnect(session);
        ssh_free(session);
        exit(-1);
    }
    ssh_scp scp;
    scp = ssh_scp_new(session, SSH_SCP_WRITE | SSH_SCP_RECURSIVE, "../");
    if (scp == NULL) {
        fprintf(stderr, "Error allocating scp session: %s\n", ssh_get_error(session));
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        ssh_disconnect(session);
        ssh_free(session);
        exit(-1);
    }
    rc = ssh_scp_init(scp);
    if (rc != SSH_OK) {
        fprintf(stderr, "Error initializing scp session: %s\n", ssh_get_error(session));
        ssh_scp_close(scp);
        ssh_scp_free(scp);
        ssh_disconnect(session);
        ssh_free(session);
        exit(-1);
    }

    rc = ssh_scp_push_directory(scp, "plutousbgadget", S_IRWXU);
    if (rc != SSH_OK) {
        fprintf(stderr, "Can't create remote directory: %s\n", ssh_get_error(session));
        exit(-1);
    }
    char* buffer;
    int size_file;
    QFile file_script(":/runme.sh");
    if(file_script.isOpen()) file_script.close();
    if (!file_script.open(QIODevice::ReadOnly)) exit(-1);
    size_file = file_script.size();
    rc = ssh_scp_push_file(scp, "runme.sh", size_file, S_IRWXU);
    if (rc != SSH_OK) {
        file_script.close();
        fprintf(stderr, "Can't open remote file: %s\n", ssh_get_error(session));
        exit(-1);
    }
    buffer = (char*)malloc(size_file);
    file_script.read(buffer, size_file);
    file_script.close();
    rc = ssh_scp_write(scp, buffer, size_file);
    if (rc != SSH_OK) {
        fprintf(stderr, "Can't write to remote file: %s\n", ssh_get_error(session));
        exit(-1);
    }
    QFile file_blob(":/plutousbgadget.ko");
    if(file_blob.isOpen()) file_blob.close();
    if (!file_blob.open(QIODevice::ReadOnly)) {
        qDebug() << "!file_blob.open(QIODevice::ReadOnly)";
        exit(-1);
    }
    size_file = file_blob.size();
    rc = ssh_scp_push_file(scp, "plutousbgadget.ko", size_file, S_IRWXU);
    if (rc != SSH_OK) {
        file_blob.close();
        fprintf(stderr, "Can't open remote file: %s\n", ssh_get_error(session));
        exit(-1);
    }
    buffer = (char*)malloc(size_file);
    file_blob.read(buffer, size_file);
    file_blob.close();
    rc = ssh_scp_write(scp, buffer, size_file);
    if (rc != SSH_OK) {
        fprintf(stderr, "Can't write to remote file: %s\n", ssh_get_error(session));
        exit(-1);
    }
    ssh_scp_close(scp);
    ssh_scp_free(scp);

    ssh_channel channel;

/*
    channel = ssh_channel_new(session);
    if (channel == NULL) exit(-1);
    rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK)
    {
        ssh_channel_free(channel);
        exit(-1);
    }
    rc = ssh_channel_request_exec(channel,
                                  "echo 1 > /sys/bus/iio/devices/iio:device4/scan_elements/in_voltage0_en");
    if (rc != SSH_OK)
    {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        exit(-1);
    }
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    channel = ssh_channel_new(session);
    if (channel == NULL) exit(-1);
    rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK)
    {
        ssh_channel_free(channel);
        exit(-1);
    }
    rc = ssh_channel_request_exec(channel,
                                  "echo 1 > /sys/bus/iio/devices/iio:device4/scan_elements/in_voltage1_en");
    if (rc != SSH_OK)
    {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        exit(-1);
    }
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
*/

    channel = ssh_channel_new(session);
    if (channel == NULL) exit(-1);
    rc = ssh_channel_open_session(channel);
    if (rc != SSH_OK) {
        ssh_channel_free(channel);
        exit(-1);
    }
    rc = ssh_channel_request_exec(channel, "/plutousbgadget/runme.sh");
    if (rc != SSH_OK) {
        ssh_channel_close(channel);
        ssh_channel_free(channel);
        exit(-1);
    }
    QThread::sleep(3);
    ssh_channel_send_eof(channel);
    ssh_channel_close(channel);
    ssh_channel_free(channel);
    ssh_disconnect(session);
    ssh_free(session);

    fprintf(stderr, "PlutoSDR kernel patch: Ok \n");
}
//-------------------------------------------------------------------------------------------
