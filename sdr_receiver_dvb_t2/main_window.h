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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "rx_sdrplay.h"
#include "rx_airspy.h"
#include "plot.h"
#include "DVB_T2/dvbt2_frame.h"

Q_DECLARE_METATYPE(bb_de_header::id_out);

QT_BEGIN_NAMESPACE
namespace Ui { class main_window; }
QT_END_NAMESPACE

class main_window : public QMainWindow
{
    Q_OBJECT

public:
    main_window(QWidget *parent = nullptr);
    ~main_window();

signals:
    void p2_show(int _id_show);
    void set_out(bb_de_header::id_out _id_current_out, int _num_port_udp,
                 QString _file_name, int _need_plp);
    void stop_device();

private slots:
    void get_device();
    void open_sdrplay();
    void sdrplay_status(int _err);
    void airspy_status(int _err);
    void open_airspy();
    void radio_frequency(double _rf);
    void device_gain(int _gain);
    void bad_signal();
    void on_push_button_start_clicked();
    void on_push_button_stop_clicked();
    void on_tab_widget_currentChanged(int index);
    void set_show_p2_symbol(int _id);
    void amount_plp(int _num_plp);
    void signal_noise_ratio(float _snr);
    void on_combo_box_plp_id_currentIndexChanged(int index);
    void view_l1_presignalling(QString _text);
    void view_l1_postsignalling(QString _text);
    void view_l1_dynamic(QString _text, bool _force_update);
    void ts_stage(QString _info);
    void on_push_button_ts_open_file_clicked();
    void on_push_button_ts_apply_clicked();
    void on_combo_box_ts_plp_currentIndexChanged(const QString &arg1);
    void on_radio_button_ts_net_toggled(bool checked);
    void on_radio_button_ts_file_toggled(bool checked);
    void on_line_edit_ts_udp_port_textChanged(const QString &arg1);

    void on_check_box_agc_stateChanged(int arg1);

private:
    Ui::main_window *ui;

    id_device_t id_device;
    dvbt2_frame* dvbt2;
    QThread* thread = nullptr;
    rx_sdrplay* ptr_sdrplay;
    rx_airspy* ptr_airspy;
    void start_sdrplay();
    void start_airspy();
    void connect_info();
    void disconnect_info();
    QButtonGroup* button_group_p2_symbol;
    type_graph type;
    plot* p1_spectrograph;
    plot* p1_constelation;
    plot* p2_spectrograph;
    plot* p2_constelation;
    plot* data_spectrograph;
    plot* data_constelation;
    plot* fc_spectrograph;
    plot* fc_constelation;
    plot* qam_constelation;
    plot* p1_correlation_oscilloscope;
    plot* equalizer_oscilloscope;

    void disconnect_signals();
};
#endif // MAINWINDOW_H
