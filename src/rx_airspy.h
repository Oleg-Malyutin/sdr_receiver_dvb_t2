#ifndef RX_AIRSPY_H
#define RX_AIRSPY_H

#include <QObject>
#include <QTimer>
#include <string>

#include "libairspy/src/airspy.h"
#include "DVB_T2/dvbt2_demodulator.h"

class rx_airspy : public QObject
{
    Q_OBJECT
public:
    explicit rx_airspy(QObject *parent = nullptr);
    ~rx_airspy();

    std::string error (int err);
    int get(std::string &_ser_no, std::string &_hw_ver);
    int init(uint32_t _rf_frequence_hz, int _gain);
    dvbt2_demodulator *demodulator;
    void rx_execute(int16_t *_ptr_rx_buffer, int _len_out_device);

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
    QThread* thread;

    uint64_t serials[10];
    struct airspy_device* device = nullptr;
    int gain;
    uint32_t rf_frequency;
    uint32_t ch_frequency;
    float sample_rate;
    bool agc = false;

    int16_t* buffer_a;
    int16_t* buffer_b;
    int16_t* ptr_buffer;
    int  blocks = 1;
    const int max_blocks = 256 * 4;
    int len_buffer = 0;
    bool swap_buffer = true;

    static int rx_callback(airspy_transfer_t* transfer);

    signal_estimate* signal;

    float frequency_offset = 0.0f;
    bool change_frequency = false;
    bool frequency_changed = true;
    clock_t start_wait_frequency_changed;
    clock_t end_wait_frequency_changed;
    int gain_offset = 0;
    bool change_gain = false;
    bool gain_changed = true;
    clock_t start_wait_gain_changed;
    clock_t end_wait_gain_changed;

    void reset();
    void set_rf_frequency();
    void set_gain();

};

#endif // RX_AIRSPY_H
