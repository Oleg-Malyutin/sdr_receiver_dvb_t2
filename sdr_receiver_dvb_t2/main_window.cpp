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
#include "main_window.h"
#include "ui_main_window.h"

//----------------------------------------------------------------------------------------------------
main_window::main_window(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::main_window)
{
    ui->setupUi(this);

    connect(ui->action_sdrplay, SIGNAL(triggered()), this, SLOT(open_sdrplay()));
    connect(ui->action_airspy, SIGNAL(triggered()), this, SLOT(open_airspy()));
    connect(ui->action_exit, SIGNAL(triggered()), this, SLOT(close()));

    ui->tab_widget->setCurrentIndex(0);
    for(int i = 1; i < ui->tab_widget->count(); ++i) ui->tab_widget->setTabEnabled(i, false);
    ui->push_button_start->setEnabled(false);
    ui->push_button_stop->setEnabled(false);
    ui->push_button_ts_open_file->setEnabled(false);
    ui->push_button_ts_apply->setEnabled(false);

    p1_spectrograph = new plot(ui->widget_fft_plot_p1, type_spectrograph, "Fast Fourier transform spectrum");
    p1_constelation = new plot(ui->widget_constellation_plot_p1, type_constelation, "Constellation diagram");
    p1_correlation_oscilloscope = new plot(ui->widget_p1_correlation, type_oscilloscope, "P1 preamble correlation");
    p2_spectrograph = new plot(ui->widget_fft_plot_p2, type_spectrograph, "Fast Fourier transform spectrum");
    p2_constelation = new plot(ui->widget_constellation_plot_p2, type_constelation, "Constellation diagram");
    data_spectrograph = new plot(ui->widget_fft_plot_data, type_spectrograph, "Fast Fourier transform spectrum");
    data_constelation = new plot(ui->widget_constellation_plot_data, type_constelation, "Constellation diagram");
    fc_spectrograph = new plot(ui->widget_fft_plot_fc, type_spectrograph, "Fast Fourier transform spectrum");
    fc_constelation = new plot(ui->widget_constellation_plot_fc, type_constelation, "Constellation diagram");
    qam_constelation = new plot(ui->widget_qam_plot, type_constelation, "Constellation diagram");
    equalizer_oscilloscope = new plot(ui->widget_equalizer, type_oscilloscope_2, "Equalizer estimation");

    button_group_p2_symbol = new QButtonGroup;
    button_group_p2_symbol->addButton(ui->radio_button_l1presignaling, 0);
    button_group_p2_symbol->addButton(ui->radio_button_l1postsignaling, 1);
    button_group_p2_symbol->addButton(ui->radio_button_data, 2);
    connect(button_group_p2_symbol, static_cast<void(QButtonGroup::*)(int)>(&QButtonGroup::buttonClicked),
            this, &main_window::set_show_p2_symbol);
}
//----------------------------------------------------------------------------------------------------
main_window::~main_window()
{
    delete ui;
    delete p1_spectrograph;
    delete p1_constelation;
    delete p1_correlation_oscilloscope;
    delete p2_spectrograph;
    delete p2_constelation;
    delete data_spectrograph;
    delete data_constelation;
    delete fc_spectrograph;
    delete fc_constelation;
    delete qam_constelation;
    delete equalizer_oscilloscope;
    delete button_group_p2_symbol;
}
//----------------------------------------------------------------------------------------------------
void main_window::get_device()
{

}
//----------------------------------------------------------------------------------------------------
void main_window::on_check_box_agc_stateChanged(int arg1)
{
    if(arg1 == 2) ui->line_edit_gain->setEnabled(false);
    else ui->line_edit_gain->setEnabled(true);
}
//----------------------------------------------------------------------------------------------------
void main_window::open_sdrplay()
{
    int err;
    char* ser_no = nullptr;
    unsigned char hw_ver;
    ptr_sdrplay = new rx_sdrplay;
    err = ptr_sdrplay->get_sdrplay(ser_no, hw_ver);
    ui->text_log->insertPlainText("Get SdrPlay:"  " " +
                                  QString::fromStdString(ptr_sdrplay->error_sdrplay(err)) + "\n");
    if(err !=0) return;

    ui->label_name->setText("Name : SdrPlay");
    ui->label_ser_no->setText("Serial No : " + QString::fromUtf8(ser_no));
    ui->label_hw_ver->setText("Hardware ver : " + QString::number(hw_ver));
    ui->label_gain->setText("gain reduction(0-85):");

    id_device = id_sdrplay;
    ui->push_button_start->setEnabled(true);
}
//----------------------------------------------------------------------------------------------------
void main_window::start_sdrplay()
{
    double rf_fraquency;
    int gain_db;
    int err;
    rf_fraquency = ui->line_edit_rf->text().toDouble();
    gain_db = ui->line_edit_gain->text().toInt();
    if(ui->check_box_agc->isChecked()) gain_db = -1;
    err = ptr_sdrplay->init_sdrplay(rf_fraquency, gain_db);
    ui->text_log->insertPlainText("Init SdrPlay:"  " "  +
                                  QString::fromStdString(ptr_sdrplay->error_sdrplay(err)) + "\n");
    if(err !=0) return;

    thread = new QThread;
    ptr_sdrplay->moveToThread(thread);
    connect(thread, SIGNAL(started()), ptr_sdrplay, SLOT(start()));
    connect(this,SIGNAL(stop_device()),ptr_sdrplay,SLOT(stop()),Qt::DirectConnection);
    connect(ptr_sdrplay, SIGNAL(finished()), ptr_sdrplay, SLOT(deleteLater()));
    connect(ptr_sdrplay, SIGNAL(finished()), thread, SLOT(quit()),Qt::DirectConnection);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start(QThread::TimeCriticalPriority);

    connect(ptr_sdrplay, &rx_sdrplay::sdrplay_status, this, &main_window::sdrplay_status);
    connect(ptr_sdrplay, &rx_sdrplay::radio_frequency, this, &main_window::radio_frequency);
    connect(ptr_sdrplay, &rx_sdrplay::device_gain, this, &main_window::device_gain);
}
//----------------------------------------------------------------------------------------------------
void main_window::sdrplay_status(int _err)
{
    ui->text_log->insertPlainText("Status SdrPlay:"  " "  +
                                  QString::fromStdString(ptr_sdrplay->error_sdrplay(_err)) + "\n");
}
//----------------------------------------------------------------------------------------------------
void main_window::airspy_status(int _err)
{
    ui->text_log->insertPlainText("Status AirSpy:"  " "  +
                                  QString::fromStdString(ptr_airspy->error_airspy(_err)) + "\n");
}
//----------------------------------------------------------------------------------------------------
void main_window::open_airspy()
{
    int err;
    string ser_no;
    string hw_ver;
    ptr_airspy = new rx_airspy;
    err = ptr_airspy->get_airspy(ser_no, hw_ver);
    ui->text_log->insertPlainText("Get AirSpy:"  " " +
                                  QString::fromStdString(ptr_airspy->error_airspy(err)) + "\n");
    if(err < 0) return;

    ui->label_name->setText("Name : AirSpy");
    ui->label_ser_no->setText("Serial No : " + QString::fromStdString(ser_no));
    ui->label_hw_ver->setText("HW: " + QString::fromStdString(hw_ver));
    ui->label_gain->setText("gain (0-21):");

    id_device = id_airspy;
    ui->push_button_start->setEnabled(true);

}
//----------------------------------------------------------------------------------------------------
void main_window::start_airspy()
{
    uint32_t rf_fraquency_hz;
    int gain;
    int err;
    rf_fraquency_hz = static_cast<uint32_t>(ui->line_edit_rf->text().toULong());
    gain = static_cast<uint8_t>(ui->line_edit_gain->text().toUInt());
    if(ui->check_box_agc->isChecked()) gain = -1;
    err = ptr_airspy->init_airspy(rf_fraquency_hz, gain);
    ui->text_log->insertPlainText("Init AirSpy:"  " "  +
                                  QString::fromStdString(ptr_airspy->error_airspy(err)) + "\n");
    if(err !=0) return;

    thread = new QThread;
    ptr_airspy->moveToThread(thread);
    connect(thread, SIGNAL(started()), ptr_airspy, SLOT(start()));
    connect(this,SIGNAL(stop_device()),ptr_airspy,SLOT(stop()),Qt::DirectConnection);
    connect(ptr_airspy, SIGNAL(finished()), ptr_airspy, SLOT(deleteLater()));
    connect(ptr_airspy, SIGNAL(finished()), thread, SLOT(quit()),Qt::DirectConnection);
    connect(thread, SIGNAL(finished()), thread, SLOT(deleteLater()));
    thread->start(QThread::TimeCriticalPriority);

    connect(ptr_airspy, &rx_airspy::airspy_status, this, &main_window::airspy_status);
    connect(ptr_airspy, &rx_airspy::radio_frequency, this, &main_window::radio_frequency);
    connect(ptr_airspy, &rx_airspy::device_gain, this, &main_window::device_gain);
}
//----------------------------------------------------------------------------------------------------
void main_window::on_push_button_start_clicked()
{
    ui->push_button_stop->setEnabled(true);
    switch (id_device) {
    case id_sdrplay:
        start_sdrplay();
        dvbt2 = ptr_sdrplay->frame;
        break;
    case id_airspy:
        start_airspy();
        dvbt2 = ptr_airspy->frame;
        break;
    }
    for(int i = 1; i < ui->tab_widget->count(); ++i) ui->tab_widget->setTabEnabled(i, true);
    connect_info();
    ui->menu_open->setEnabled(false);
    ui->push_button_start->setEnabled(false);
    ui->line_edit_rf->setEnabled(false);
    ui->line_edit_gain->setEnabled(false);
    ui->check_box_agc->setEnabled(false);
}
//----------------------------------------------------------------------------------------------------
void main_window::connect_info()
{
    connect(dvbt2->p1_demod, &p1_symbol::bad_signal, this, &main_window::bad_signal);
    connect(dvbt2->p2_demod, &p2_symbol::view_l1_presignalling, this, &main_window::view_l1_presignalling);
    connect(dvbt2->p2_demod, &p2_symbol::view_l1_postsignalling, this, &main_window::view_l1_postsignalling);
    connect(dvbt2->p2_demod, &p2_symbol::view_l1_dynamic, this, &main_window::view_l1_dynamic);
    connect(dvbt2, &dvbt2_frame::amount_plp, this, &main_window::amount_plp);
    connect(dvbt2->deinterleaver->qam, &llr_demapper::signal_noise_ratio,
            this, &main_window::signal_noise_ratio);
    connect(dvbt2->deinterleaver->qam->decoder->decoder->deheader, &bb_de_header::ts_stage,
            this, &main_window::ts_stage);
    qRegisterMetaType<bb_de_header::id_out>();
    connect(this, &main_window::set_out,
            dvbt2->deinterleaver->qam->decoder->decoder->deheader, &bb_de_header::set_out);
}
//----------------------------------------------------------------------------------------------------
void main_window::bad_signal()
{
    if(ui->push_button_stop->isEnabled()) {
        on_push_button_stop_clicked();
        QMessageBox mes_box;
        mes_box.setText("Bad signal !\nСheck gain and frequency.");
        mes_box.exec();
    }
}
//----------------------------------------------------------------------------------------------------
void main_window::on_push_button_stop_clicked()
{
    disconnect_signals();
    disconnect_info();
    ui->tab_widget->setCurrentIndex(0);
    for(int i = 1; i < ui->tab_widget->count(); ++i) ui->tab_widget->setTabEnabled(i, false);
    ui->push_button_start->setEnabled(false);
    ui->push_button_stop->setEnabled(false);
    ui->push_button_ts_open_file->setEnabled(false);
    ui->push_button_ts_apply->setEnabled(false);
    emit stop_device();
    if(thread->isRunning()) thread->wait();
    ui->label_name->setText("Name :");
    ui->label_ser_no->setText("Serial No :");
    ui->label_hw_ver->setText("Hardware ver :");
    ui->label_info_rf->setText("radio frequency (Hz) : ");
    ui->label_info_gain->setText("gain reducton (dB) : ");
    ui->menu_open->setEnabled(true);
    ui->line_edit_rf->setEnabled(true);
    ui->line_edit_gain->setEnabled(true);
    ui->check_box_agc->setEnabled(true);
}
//----------------------------------------------------------------------------------------------------
void main_window::disconnect_info()
{
    disconnect(dvbt2->p1_demod, &p1_symbol::bad_signal, this, &main_window::bad_signal);
    disconnect(dvbt2->p2_demod, &p2_symbol::view_l1_presignalling, this, &main_window::view_l1_presignalling);
    disconnect(dvbt2->p2_demod, &p2_symbol::view_l1_postsignalling, this, &main_window::view_l1_postsignalling);
    disconnect(dvbt2->p2_demod, &p2_symbol::view_l1_dynamic, this, &main_window::view_l1_dynamic);
    disconnect(dvbt2, &dvbt2_frame::amount_plp, this, &main_window::amount_plp);
    disconnect(dvbt2->deinterleaver->qam, &llr_demapper::signal_noise_ratio,
               this, &main_window::signal_noise_ratio);
    disconnect(dvbt2->deinterleaver->qam->decoder->decoder->deheader, &bb_de_header::ts_stage,
               this, &main_window::ts_stage);
    disconnect(this, &main_window::set_out,
               dvbt2->deinterleaver->qam->decoder->decoder->deheader, &bb_de_header::set_out);
}
//----------------------------------------------------------------------------------------------------
void main_window::radio_frequency(double _rf)
{
    ui->label_info_rf->setText("radio frequency (Hz) : " + QString::number(_rf, 'f', 0));
}
//----------------------------------------------------------------------------------------------------
void main_window::device_gain(int _gain)
{
    QString str_gain = "";
    switch (id_device) {
    case id_sdrplay:
        str_gain = "gain reduction :  ";
        break;
    case id_airspy:
        str_gain = "gain :   ";
        break;
    }
    ui->label_info_gain->setText(str_gain + QString::number(_gain));
}
//----------------------------------------------------------------------------------------------------
void main_window::on_tab_widget_currentChanged(int index)
{

    switch(index){
    case 1:
        disconnect_signals();
        connect(dvbt2->p1_demod, &p1_symbol::replace_spectrograph,
                p1_spectrograph, &plot::replace_spectrograph);
        connect(dvbt2->p1_demod, &p1_symbol::replace_constelation,
                p1_constelation, &plot::replace_constelation);
        break;
    case 2:
        disconnect_signals();
        connect(dvbt2->p2_demod, &p2_symbol::replace_spectrograph,
                p2_spectrograph, &plot::replace_spectrograph);
        connect(dvbt2->p2_demod, &p2_symbol::replace_constelation,
                p2_constelation, &plot::replace_constelation);
        break;
    case 3:
        disconnect_signals();
        connect(dvbt2->data_demod, &data_symbol::replace_spectrograph,
                data_spectrograph, &plot::replace_spectrograph);
        connect(dvbt2->data_demod, &data_symbol::replace_constelation,
                data_constelation, &plot::replace_constelation);
        break;
    case 4:
        disconnect_signals();
        connect(dvbt2->fc_demod, &fc_symbol::replace_spectrograph,
                fc_spectrograph, &plot::replace_spectrograph);
        connect(dvbt2->fc_demod, &fc_symbol::replace_constelation,
                fc_constelation, &plot::replace_constelation);
        break;
    case 5:
        disconnect_signals();
        connect(dvbt2->deinterleaver, &time_deinterleaver::replace_constelation,
                qam_constelation, &plot::replace_constelation);
        break;
    case 8:
        disconnect_signals();
        connect(dvbt2->p1_demod, &p1_symbol::replace_oscilloscope,
                p1_correlation_oscilloscope, &plot::replace_oscilloscope);
        connect(dvbt2->p2_demod, &p2_symbol::replace_oscilloscope,
                equalizer_oscilloscope, &plot::replace_oscilloscope);
        break;
    }
}
//----------------------------------------------------------------------------------------------------
void main_window::disconnect_signals()
{
    disconnect(dvbt2->p1_demod, &p1_symbol::replace_spectrograph,
            p1_spectrograph, &plot::replace_spectrograph);
    disconnect(dvbt2->p1_demod, &p1_symbol::replace_constelation,
            p1_constelation, &plot::replace_constelation);
    disconnect(dvbt2->p2_demod, &p2_symbol::replace_spectrograph,
            p2_spectrograph, &plot::replace_spectrograph);
    disconnect(dvbt2->p2_demod, &p2_symbol::replace_constelation,
            p2_constelation, &plot::replace_constelation);
    disconnect(dvbt2->data_demod, &data_symbol::replace_constelation,
            data_spectrograph, &plot::replace_constelation);
    disconnect(dvbt2->data_demod, &data_symbol::replace_spectrograph,
            data_spectrograph, &plot::replace_spectrograph);
    disconnect(dvbt2->data_demod, &data_symbol::replace_constelation,
            data_constelation, &plot::replace_constelation);
    disconnect(dvbt2->fc_demod, &fc_symbol::replace_spectrograph,
            fc_spectrograph, &plot::replace_spectrograph);
    disconnect(dvbt2->fc_demod, &fc_symbol::replace_constelation,
            fc_constelation, &plot::replace_constelation);
    disconnect(dvbt2->deinterleaver, &time_deinterleaver::replace_constelation,
            qam_constelation, &plot::replace_constelation);
    disconnect(dvbt2->p1_demod, &p1_symbol::replace_oscilloscope,
            p1_correlation_oscilloscope, &plot::replace_oscilloscope);
    disconnect(dvbt2->p2_demod, &p2_symbol::replace_oscilloscope,
               equalizer_oscilloscope, &plot::replace_oscilloscope);
}
//----------------------------------------------------------------------------------------------------
void main_window::set_show_p2_symbol(int _id)
{
    switch(_id){
    case 0:
        dvbt2->p2_demod->id_show = p2_symbol::p2_l1_pre;
        break;
    case 1:
        dvbt2->p2_demod->id_show = p2_symbol::p2_l1_post;
        break;
    case 2:
        dvbt2->p2_demod->id_show = p2_symbol::p2_data;
        break;
    }
}
//----------------------------------------------------------------------------------------------------
void main_window::amount_plp(int _num_plp)
{
    ui->combo_box_plp_id->clear();
    for(int i = 0; i < _num_plp; ++i) ui->combo_box_plp_id->addItem(QString::number(i));
    ui->combo_box_ts_plp->clear();
    for(int i = 0; i < _num_plp; ++i) ui->combo_box_ts_plp->addItem(QString::number(i));
}
//----------------------------------------------------------------------------------------------------
void main_window::on_combo_box_plp_id_currentIndexChanged(int index)
{
    dvbt2->deinterleaver->idx_show_plp = index;
}
//----------------------------------------------------------------------------------------------------
void main_window::signal_noise_ratio(float _snr)
{
    ui->label_snr->setText("SNR : " + QString::number(static_cast<int>(_snr)) + "dB");
}
//----------------------------------------------------------------------------------------------------
void main_window::view_l1_presignalling(QString _text)
{
    ui->text_edit_l1_presignalling->clear();
    ui->text_edit_l1_presignalling->append(_text);
}
//----------------------------------------------------------------------------------------------------
void main_window::view_l1_postsignalling(QString _text)
{
    ui->text_edit_l1_postsignalling->clear();
    ui->text_edit_l1_postsignalling->append(_text);
}
//----------------------------------------------------------------------------------------------------
void main_window::view_l1_dynamic(QString _text, bool _force_update)
{
    static bool update = true;
    if(update || ui->check_box_auto_update_l1_dynamic->isChecked()) {
        update = false;
        ui->text_edit_l1_postsignalling_dynamic->clear();
        ui->text_edit_l1_postsignalling_dynamic->append(_text);
    }
    if(_force_update) update = true;
}
//----------------------------------------------------------------------------------------------------
void main_window::ts_stage(QString _info)
{
    static QString old_stage = "";
    static int count = 0;
    QString info = "";
    ui->text_edit_ts_stage->clear();
    if(_info != old_stage) {  
        info = _info;
        old_stage = _info;
        count = 0;
    }
    else {
        ++count;
        info = _info + " (count.. " + QString::number(count) + ")";
    }
    ui->text_edit_ts_stage->append(info);
}
//----------------------------------------------------------------------------------------------------
void main_window::on_push_button_ts_apply_clicked()
{
    bb_de_header::id_out id_current_out = bb_de_header::out_network;
    int num_port_udp = 7654;
    QString port = ui->line_edit_ts_udp_port->text();
    if(port != "") num_port_udp = port.toInt();
    else ui->line_edit_ts_udp_port->setText("7654");

    if(ui->radio_button_ts_file->isChecked()) id_current_out = bb_de_header::out_file;
    QString file_name = ui->line_edit_ts_file_name->text();

    QString plp = ui->combo_box_ts_plp->currentText();
    int need_plp = 0;
    need_plp = plp.toInt();

    emit set_out(id_current_out, num_port_udp, file_name, need_plp);

    ui->push_button_ts_apply->setEnabled(false);
}
//----------------------------------------------------------------------------------------------------
void main_window::on_combo_box_ts_plp_currentIndexChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    ui->push_button_ts_apply->setEnabled(true);
}
//---------------------------------------------------------------------------------------------------
void main_window::on_radio_button_ts_net_toggled(bool checked)
{
    if(checked) {
        ui->line_edit_ts_udp_port->setEnabled(true);
        ui->push_button_ts_apply->setEnabled(true);
    }
    else {
        ui->line_edit_ts_udp_port->setEnabled(false);
    }
}
//----------------------------------------------------------------------------------------------------
void main_window::on_line_edit_ts_udp_port_textChanged(const QString &arg1)
{
    Q_UNUSED(arg1)
    ui->push_button_ts_apply->setEnabled(true);
}
//----------------------------------------------------------------------------------------------------
void main_window::on_radio_button_ts_file_toggled(bool checked)
{
    if(checked) ui->push_button_ts_open_file->setEnabled(true);
    else ui->push_button_ts_open_file->setEnabled(false);
}
//----------------------------------------------------------------------------------------------------
void main_window::on_push_button_ts_open_file_clicked()
{
    QString file_name = "";
    file_name = QFileDialog::getSaveFileName(this);
    ui->line_edit_ts_file_name->setText(file_name);
    if(file_name != "") ui->push_button_ts_apply->setEnabled(true);
    else ui->radio_button_ts_net->setChecked(false);
}
//----------------------------------------------------------------------------------------------------


