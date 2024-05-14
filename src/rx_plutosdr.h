#ifndef RX_PLUTOSDR_H
#define RX_PLUTOSDR_H

#include <QObject>

#include "libplutosdr/plutosdr_hi_speed_rx.h"
#include "DVB_T2/dvbt2_demodulator.h"

class rx_plutosdr : public QObject
{
    Q_OBJECT
public:
    explicit rx_plutosdr(QObject* parent = nullptr);

    std::string error (int _err);
    int get(std::string &_ser_no, std::string &_hw_ver);
    int init(uint64_t _rf_frequence_hz, int _gain);
    dvbt2_demodulator* demodulator;
    void rx_execute(int16_t *_rx_i, int16_t *_rx_q);
    void reboot();

signals:
    void execute(int _len_in, int16_t* _i_in, int16_t* _q_in, signal_estimate* signal_);
    void status(int _err);
    void radio_frequency(double _rf);
    void level_gain(int _gain);
    void stop_demodulator();
    void finished();

public slots:
    void start();
    void stop();

private:
    const char* port_name;
    void pluto_kernel_patch();

    QThread* thread;

    plutosdr_device_t* device;
    uint32_t len_out_device;
    const int max_blocks = 32 * 8;
    uint32_t max_len_out;
    int blocks = 1;
    int len_buffer = 0;
    int16_t *i_buffer_a, *q_buffer_a;
    int16_t *i_buffer_b, *q_buffer_b;
    int16_t *ptr_i_buffer, *ptr_q_buffer;
    bool swap_buffer = true;

    signal_estimate* signal;

    int64_t rf_bandwidth_hz;
    int64_t sample_rate_hz;
    clock_t start_wait_frequency_changed;
    clock_t end_wait_frequency_changed;
    int64_t rf_frequency;
    int64_t ch_frequency;
    clock_t start_wait_gain_changed;
    clock_t end_wait_gain_changed;
    uint32_t gain_db;
    bool agc = false;
    bool done = true;

    static int plutosdr_callback(plutosdr_transfer* _transfer);

    void reset();
    void set_rf_frequency();
    void set_gain();
};

#endif // RX_PLUTOSDR_H
