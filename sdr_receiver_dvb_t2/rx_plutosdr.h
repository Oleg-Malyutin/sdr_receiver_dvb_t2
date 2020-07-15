#ifndef RX_PLUTOSDR_H
#define RX_PLUTOSDR_H

#include <QObject>

#include "libplutosdr/plutosdr_hi_speed_rx.h"
#include "DVB_T2/dvbt2_frame.h"

typedef std::string string;

class rx_plutosdr : public QObject
{
    Q_OBJECT
public:
    explicit rx_plutosdr(QObject* parent = nullptr);

    string	error (int _err);
    int get(string &_ser_no, string &_hw_ver);
    int init(uint64_t _rf_frequence_hz, int _gain);
    dvbt2_frame* frame;
    void rx_execute(int16_t *_rx_i, int16_t *_rx_q);
    void reboot();

signals:
    void execute(int _len_in, int16_t* _i_in, int16_t* _q_in, bool _frequence_changed, bool _gain_changed);
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
    QWaitCondition* signal_out;
    QMutex* mutex_out;

    plutosdr_device_t* device;
    uint32_t len_out_device;
    const int max_blocks = 32;
    uint32_t max_len_out;
    int blocks = 1;
    int len_buffer = 0;
    int16_t* i_buffer_a;
    int16_t* q_buffer_a;
    int16_t* i_buffer_b;
    int16_t* q_buffer_b;
    int16_t* ptr_i_buffer;
    int16_t* ptr_q_buffer;
    bool swap_buffer = true;
    int64_t rf_bandwidth_hz;
    int64_t sample_rate_hz;
    clock_t start_wait_frequency_changed;
    clock_t end_wait_frequency_changed;
    int64_t rf_frequency;
    float frequency_offset = 0.0f;
    bool change_frequency = false;
    bool frequency_changed = true;
    clock_t start_wait_gain_changed;
    clock_t end_wait_gain_changed;
    uint32_t gain_db;
    bool agc = false;
    int gain_offset = 0;
    bool change_gain = false;
    bool gain_changed = true;
    bool done = true;
    static int plutosdr_callback(plutosdr_transfer* _transfer);
};

#endif // RX_PLUTOSDR_H
