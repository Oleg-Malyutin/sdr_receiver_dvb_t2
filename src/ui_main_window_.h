/********************************************************************************
** Form generated from reading UI file 'main_window.ui'
**
** Created by: Qt User Interface Compiler version 5.12.12
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAIN_WINDOW_H
#define UI_MAIN_WINDOW_H

#include <QtCore/QVariant>
#include <QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QRadioButton>
#include <QtWidgets/QSpacerItem>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTextEdit>
#include <QtWidgets/QWidget>
#include "qcustomplot.h"

QT_BEGIN_NAMESPACE

class Ui_main_window
{
public:
    QAction *action_exit;
    QAction *action_sdrplay;
    QAction *action_airspy;
    QAction *action_plutosdr;
    QWidget *centralwidget;
    QGridLayout *gridLayout_7;
    QTabWidget *tab_widget;
    QWidget *tab_device;
    QGridLayout *gridLayout_2;
    QGroupBox *group_box_info;
    QGridLayout *gridLayout_4;
    QLabel *label_name;
    QLabel *label_info_gain;
    QLabel *label_info_rf;
    QSpacerItem *horizontalSpacer;
    QLabel *label_ser_no;
    QLabel *label_hw_ver;
    QSpacerItem *verticalSpacer_2;
    QGroupBox *group_box_settings;
    QGridLayout *gridLayout_3;
    QLineEdit *line_edit_rf;
    QSpacerItem *verticalSpacer;
    QPushButton *push_button_stop;
    QCheckBox *check_box_agc;
    QPushButton *push_button_start;
    QLineEdit *line_edit_gain;
    QLabel *label_gain;
    QSpacerItem *horizontalSpacer_2;
    QLabel *label_rf;
    QSpacerItem *horizontalSpacer_3;
    QTextEdit *text_log;
    QWidget *tab_p1_symbol;
    QGridLayout *gridLayout;
    QCustomPlot *widget_fft_plot_p1;
    QCustomPlot *widget_constellation_plot_p1;
    QWidget *tab_p2_symbol;
    QGridLayout *gridLayout_5;
    QCustomPlot *widget_fft_plot_p2;
    QCustomPlot *widget_constellation_plot_p2;
    QGridLayout *gridLayout_10;
    QRadioButton *radio_button_data;
    QRadioButton *radio_button_l1postsignaling;
    QRadioButton *radio_button_l1presignaling;
    QSpacerItem *verticalSpacer_3;
    QSpacerItem *horizontalSpacer_6;
    QLabel *label;
    QLabel *label_8;
    QWidget *tab_data_symbol;
    QGridLayout *gridLayout_8;
    QCustomPlot *widget_fft_plot_data;
    QCustomPlot *widget_constellation_plot_data;
    QWidget *frame_closing_symbol;
    QGridLayout *gridLayout_6;
    QCustomPlot *widget_fft_plot_fc;
    QCustomPlot *widget_constellation_plot_fc;
    QWidget *tab_qam_demapper;
    QGridLayout *gridLayout_9;
    QCustomPlot *widget_qam_plot;
    QWidget *widget;
    QGridLayout *gridLayout_11;
    QLabel *label_2;
    QSpacerItem *verticalSpacer_4;
    QComboBox *combo_box_plp_id;
    QSpacerItem *horizontalSpacer_4;
    QLabel *label_snr;
    QWidget *tab_l1_signalling;
    QGridLayout *gridLayout_12;
    QLabel *label_3;
    QLabel *label_5;
    QLabel *label_4;
    QLabel *label_6;
    QTextEdit *text_edit_l1_postsignalling;
    QTextEdit *text_edit_l1_presignalling;
    QCheckBox *check_box_auto_update_l1_dynamic;
    QTextEdit *text_edit_l1_postsignalling_dynamic;
    QLabel *label_9;
    QLabel *label_10;
    QWidget *tab_transport_stream;
    QGridLayout *gridLayout_13;
    QGroupBox *group_box_ts_info;
    QGridLayout *gridLayout_15;
    QTextEdit *text_edit_ts_stage;
    QGroupBox *group_box_ts_settings;
    QGridLayout *gridLayout_14;
    QLineEdit *line_edit_ts_file_name;
    QSpacerItem *horizontalSpacer_5;
    QPushButton *push_button_ts_open_file;
    QRadioButton *radio_button_ts_net;
    QRadioButton *radio_button_ts_file;
    QPushButton *push_button_ts_apply;
    QLabel *label_7;
    QLineEdit *line_edit_ts_udp_port;
    QSpacerItem *verticalSpacer_5;
    QComboBox *combo_box_ts_plp;
    QWidget *tab;
    QGridLayout *gridLayout_16;
    QCustomPlot *widget_p1_correlation;
    QCustomPlot *widget_equalizer;
    QCustomPlot *widget_fq_offset;
    QMenuBar *menubar;
    QMenu *menu_file;
    QMenu *menu_open;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *main_window)
    {
        if (main_window->objectName().isEmpty())
            main_window->setObjectName(QString::fromUtf8("main_window"));
        main_window->resize(944, 601);
        main_window->setUnifiedTitleAndToolBarOnMac(true);
        action_exit = new QAction(main_window);
        action_exit->setObjectName(QString::fromUtf8("action_exit"));
        action_sdrplay = new QAction(main_window);
        action_sdrplay->setObjectName(QString::fromUtf8("action_sdrplay"));
        action_airspy = new QAction(main_window);
        action_airspy->setObjectName(QString::fromUtf8("action_airspy"));
        action_plutosdr = new QAction(main_window);
        action_plutosdr->setObjectName(QString::fromUtf8("action_plutosdr"));
        action_plutosdr->setCheckable(false);
        centralwidget = new QWidget(main_window);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout_7 = new QGridLayout(centralwidget);
        gridLayout_7->setObjectName(QString::fromUtf8("gridLayout_7"));
        tab_widget = new QTabWidget(centralwidget);
        tab_widget->setObjectName(QString::fromUtf8("tab_widget"));
        tab_widget->setEnabled(true);
        tab_widget->setMinimumSize(QSize(0, 0));
        tab_device = new QWidget();
        tab_device->setObjectName(QString::fromUtf8("tab_device"));
        gridLayout_2 = new QGridLayout(tab_device);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        group_box_info = new QGroupBox(tab_device);
        group_box_info->setObjectName(QString::fromUtf8("group_box_info"));
        QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(1);
        sizePolicy.setVerticalStretch(0);
        sizePolicy.setHeightForWidth(group_box_info->sizePolicy().hasHeightForWidth());
        group_box_info->setSizePolicy(sizePolicy);
        gridLayout_4 = new QGridLayout(group_box_info);
        gridLayout_4->setObjectName(QString::fromUtf8("gridLayout_4"));
        label_name = new QLabel(group_box_info);
        label_name->setObjectName(QString::fromUtf8("label_name"));

        gridLayout_4->addWidget(label_name, 0, 0, 1, 1);

        label_info_gain = new QLabel(group_box_info);
        label_info_gain->setObjectName(QString::fromUtf8("label_info_gain"));

        gridLayout_4->addWidget(label_info_gain, 4, 0, 1, 1);

        label_info_rf = new QLabel(group_box_info);
        label_info_rf->setObjectName(QString::fromUtf8("label_info_rf"));

        gridLayout_4->addWidget(label_info_rf, 3, 0, 1, 1);

        horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_4->addItem(horizontalSpacer, 0, 1, 1, 1);

        label_ser_no = new QLabel(group_box_info);
        label_ser_no->setObjectName(QString::fromUtf8("label_ser_no"));
        label_ser_no->setTextFormat(Qt::PlainText);
        label_ser_no->setWordWrap(true);

        gridLayout_4->addWidget(label_ser_no, 1, 0, 1, 2);

        label_hw_ver = new QLabel(group_box_info);
        label_hw_ver->setObjectName(QString::fromUtf8("label_hw_ver"));
        label_hw_ver->setTextFormat(Qt::PlainText);

        gridLayout_4->addWidget(label_hw_ver, 2, 0, 1, 2);

        verticalSpacer_2 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_4->addItem(verticalSpacer_2, 5, 0, 1, 2);


        gridLayout_2->addWidget(group_box_info, 0, 3, 1, 1);

        group_box_settings = new QGroupBox(tab_device);
        group_box_settings->setObjectName(QString::fromUtf8("group_box_settings"));
        sizePolicy.setHeightForWidth(group_box_settings->sizePolicy().hasHeightForWidth());
        group_box_settings->setSizePolicy(sizePolicy);
        gridLayout_3 = new QGridLayout(group_box_settings);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        line_edit_rf = new QLineEdit(group_box_settings);
        line_edit_rf->setObjectName(QString::fromUtf8("line_edit_rf"));
        line_edit_rf->setMinimumSize(QSize(160, 0));

        gridLayout_3->addWidget(line_edit_rf, 1, 2, 1, 1);

        verticalSpacer = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_3->addItem(verticalSpacer, 7, 0, 1, 1);

        push_button_stop = new QPushButton(group_box_settings);
        push_button_stop->setObjectName(QString::fromUtf8("push_button_stop"));
        QSizePolicy sizePolicy1(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(push_button_stop->sizePolicy().hasHeightForWidth());
        push_button_stop->setSizePolicy(sizePolicy1);

        gridLayout_3->addWidget(push_button_stop, 12, 2, 1, 1);

        check_box_agc = new QCheckBox(group_box_settings);
        check_box_agc->setObjectName(QString::fromUtf8("check_box_agc"));
        check_box_agc->setChecked(true);

        gridLayout_3->addWidget(check_box_agc, 5, 3, 1, 1);

        push_button_start = new QPushButton(group_box_settings);
        push_button_start->setObjectName(QString::fromUtf8("push_button_start"));
        sizePolicy1.setHeightForWidth(push_button_start->sizePolicy().hasHeightForWidth());
        push_button_start->setSizePolicy(sizePolicy1);

        gridLayout_3->addWidget(push_button_start, 12, 0, 1, 2);

        line_edit_gain = new QLineEdit(group_box_settings);
        line_edit_gain->setObjectName(QString::fromUtf8("line_edit_gain"));
        line_edit_gain->setEnabled(false);
        line_edit_gain->setMaxLength(32765);

        gridLayout_3->addWidget(line_edit_gain, 5, 2, 1, 1);

        label_gain = new QLabel(group_box_settings);
        label_gain->setObjectName(QString::fromUtf8("label_gain"));

        gridLayout_3->addWidget(label_gain, 5, 0, 1, 2);

        horizontalSpacer_2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_3->addItem(horizontalSpacer_2, 12, 3, 1, 1);

        label_rf = new QLabel(group_box_settings);
        label_rf->setObjectName(QString::fromUtf8("label_rf"));

        gridLayout_3->addWidget(label_rf, 1, 0, 1, 1);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_3->addItem(horizontalSpacer_3, 12, 4, 1, 1);


        gridLayout_2->addWidget(group_box_settings, 0, 1, 1, 1);

        text_log = new QTextEdit(tab_device);
        text_log->setObjectName(QString::fromUtf8("text_log"));
        text_log->setMaximumSize(QSize(16777215, 16777215));
        text_log->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
        text_log->setReadOnly(true);

        gridLayout_2->addWidget(text_log, 1, 1, 1, 3);

        tab_widget->addTab(tab_device, QString());
        tab_p1_symbol = new QWidget();
        tab_p1_symbol->setObjectName(QString::fromUtf8("tab_p1_symbol"));
        tab_p1_symbol->setEnabled(true);
        gridLayout = new QGridLayout(tab_p1_symbol);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        widget_fft_plot_p1 = new QCustomPlot(tab_p1_symbol);
        widget_fft_plot_p1->setObjectName(QString::fromUtf8("widget_fft_plot_p1"));

        gridLayout->addWidget(widget_fft_plot_p1, 0, 0, 1, 1);

        widget_constellation_plot_p1 = new QCustomPlot(tab_p1_symbol);
        widget_constellation_plot_p1->setObjectName(QString::fromUtf8("widget_constellation_plot_p1"));

        gridLayout->addWidget(widget_constellation_plot_p1, 0, 1, 1, 1);

        tab_widget->addTab(tab_p1_symbol, QString());
        tab_p2_symbol = new QWidget();
        tab_p2_symbol->setObjectName(QString::fromUtf8("tab_p2_symbol"));
        tab_p2_symbol->setEnabled(true);
        gridLayout_5 = new QGridLayout(tab_p2_symbol);
        gridLayout_5->setObjectName(QString::fromUtf8("gridLayout_5"));
        widget_fft_plot_p2 = new QCustomPlot(tab_p2_symbol);
        widget_fft_plot_p2->setObjectName(QString::fromUtf8("widget_fft_plot_p2"));
        sizePolicy.setHeightForWidth(widget_fft_plot_p2->sizePolicy().hasHeightForWidth());
        widget_fft_plot_p2->setSizePolicy(sizePolicy);

        gridLayout_5->addWidget(widget_fft_plot_p2, 1, 0, 1, 1);

        widget_constellation_plot_p2 = new QCustomPlot(tab_p2_symbol);
        widget_constellation_plot_p2->setObjectName(QString::fromUtf8("widget_constellation_plot_p2"));
        sizePolicy.setHeightForWidth(widget_constellation_plot_p2->sizePolicy().hasHeightForWidth());
        widget_constellation_plot_p2->setSizePolicy(sizePolicy);
        gridLayout_10 = new QGridLayout(widget_constellation_plot_p2);
        gridLayout_10->setObjectName(QString::fromUtf8("gridLayout_10"));
        gridLayout_10->setContentsMargins(-1, -1, -1, 0);
        radio_button_data = new QRadioButton(widget_constellation_plot_p2);
        radio_button_data->setObjectName(QString::fromUtf8("radio_button_data"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(radio_button_data->sizePolicy().hasHeightForWidth());
        radio_button_data->setSizePolicy(sizePolicy2);
        radio_button_data->setMinimumSize(QSize(0, 0));
        radio_button_data->setMaximumSize(QSize(80, 16777215));
        QFont font;
        font.setBold(false);
        font.setItalic(false);
        radio_button_data->setFont(font);
        radio_button_data->setAutoFillBackground(true);

        gridLayout_10->addWidget(radio_button_data, 4, 0, 1, 1);

        radio_button_l1postsignaling = new QRadioButton(widget_constellation_plot_p2);
        radio_button_l1postsignaling->setObjectName(QString::fromUtf8("radio_button_l1postsignaling"));
        sizePolicy2.setHeightForWidth(radio_button_l1postsignaling->sizePolicy().hasHeightForWidth());
        radio_button_l1postsignaling->setSizePolicy(sizePolicy2);
        radio_button_l1postsignaling->setMinimumSize(QSize(0, 0));
        radio_button_l1postsignaling->setMaximumSize(QSize(80, 16777215));
        radio_button_l1postsignaling->setFont(font);
        radio_button_l1postsignaling->setAutoFillBackground(true);

        gridLayout_10->addWidget(radio_button_l1postsignaling, 3, 0, 1, 1);

        radio_button_l1presignaling = new QRadioButton(widget_constellation_plot_p2);
        radio_button_l1presignaling->setObjectName(QString::fromUtf8("radio_button_l1presignaling"));
        sizePolicy2.setHeightForWidth(radio_button_l1presignaling->sizePolicy().hasHeightForWidth());
        radio_button_l1presignaling->setSizePolicy(sizePolicy2);
        radio_button_l1presignaling->setMinimumSize(QSize(0, 0));
        radio_button_l1presignaling->setMaximumSize(QSize(80, 16777215));
        radio_button_l1presignaling->setFont(font);
        radio_button_l1presignaling->setAutoFillBackground(true);
        radio_button_l1presignaling->setChecked(true);

        gridLayout_10->addWidget(radio_button_l1presignaling, 2, 0, 1, 1);

        verticalSpacer_3 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_10->addItem(verticalSpacer_3, 5, 2, 1, 1);

        horizontalSpacer_6 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_10->addItem(horizontalSpacer_6, 1, 2, 1, 1);

        label = new QLabel(widget_constellation_plot_p2);
        label->setObjectName(QString::fromUtf8("label"));
        label->setAutoFillBackground(true);
        label->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);

        gridLayout_10->addWidget(label, 1, 0, 1, 1);

        label_8 = new QLabel(widget_constellation_plot_p2);
        label_8->setObjectName(QString::fromUtf8("label_8"));

        gridLayout_10->addWidget(label_8, 0, 0, 1, 1);


        gridLayout_5->addWidget(widget_constellation_plot_p2, 1, 1, 1, 1);

        tab_widget->addTab(tab_p2_symbol, QString());
        tab_data_symbol = new QWidget();
        tab_data_symbol->setObjectName(QString::fromUtf8("tab_data_symbol"));
        tab_data_symbol->setEnabled(true);
        gridLayout_8 = new QGridLayout(tab_data_symbol);
        gridLayout_8->setObjectName(QString::fromUtf8("gridLayout_8"));
        widget_fft_plot_data = new QCustomPlot(tab_data_symbol);
        widget_fft_plot_data->setObjectName(QString::fromUtf8("widget_fft_plot_data"));

        gridLayout_8->addWidget(widget_fft_plot_data, 0, 0, 1, 1);

        widget_constellation_plot_data = new QCustomPlot(tab_data_symbol);
        widget_constellation_plot_data->setObjectName(QString::fromUtf8("widget_constellation_plot_data"));

        gridLayout_8->addWidget(widget_constellation_plot_data, 0, 1, 1, 1);

        tab_widget->addTab(tab_data_symbol, QString());
        frame_closing_symbol = new QWidget();
        frame_closing_symbol->setObjectName(QString::fromUtf8("frame_closing_symbol"));
        gridLayout_6 = new QGridLayout(frame_closing_symbol);
        gridLayout_6->setObjectName(QString::fromUtf8("gridLayout_6"));
        widget_fft_plot_fc = new QCustomPlot(frame_closing_symbol);
        widget_fft_plot_fc->setObjectName(QString::fromUtf8("widget_fft_plot_fc"));

        gridLayout_6->addWidget(widget_fft_plot_fc, 0, 0, 1, 1);

        widget_constellation_plot_fc = new QCustomPlot(frame_closing_symbol);
        widget_constellation_plot_fc->setObjectName(QString::fromUtf8("widget_constellation_plot_fc"));

        gridLayout_6->addWidget(widget_constellation_plot_fc, 0, 1, 1, 1);

        tab_widget->addTab(frame_closing_symbol, QString());
        tab_qam_demapper = new QWidget();
        tab_qam_demapper->setObjectName(QString::fromUtf8("tab_qam_demapper"));
        gridLayout_9 = new QGridLayout(tab_qam_demapper);
        gridLayout_9->setObjectName(QString::fromUtf8("gridLayout_9"));
        widget_qam_plot = new QCustomPlot(tab_qam_demapper);
        widget_qam_plot->setObjectName(QString::fromUtf8("widget_qam_plot"));
        QSizePolicy sizePolicy3(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy3.setHorizontalStretch(1);
        sizePolicy3.setVerticalStretch(1);
        sizePolicy3.setHeightForWidth(widget_qam_plot->sizePolicy().hasHeightForWidth());
        widget_qam_plot->setSizePolicy(sizePolicy3);

        gridLayout_9->addWidget(widget_qam_plot, 0, 1, 1, 1);

        widget = new QWidget(tab_qam_demapper);
        widget->setObjectName(QString::fromUtf8("widget"));
        QSizePolicy sizePolicy4(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        sizePolicy4.setHorizontalStretch(1);
        sizePolicy4.setVerticalStretch(0);
        sizePolicy4.setHeightForWidth(widget->sizePolicy().hasHeightForWidth());
        widget->setSizePolicy(sizePolicy4);
        gridLayout_11 = new QGridLayout(widget);
        gridLayout_11->setObjectName(QString::fromUtf8("gridLayout_11"));
        gridLayout_11->setVerticalSpacing(9);
        gridLayout_11->setContentsMargins(35, 21, -1, 22);
        label_2 = new QLabel(widget);
        label_2->setObjectName(QString::fromUtf8("label_2"));
        sizePolicy2.setHeightForWidth(label_2->sizePolicy().hasHeightForWidth());
        label_2->setSizePolicy(sizePolicy2);
        label_2->setMaximumSize(QSize(60, 24));

        gridLayout_11->addWidget(label_2, 1, 0, 1, 1);

        verticalSpacer_4 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_11->addItem(verticalSpacer_4, 4, 0, 1, 1);

        combo_box_plp_id = new QComboBox(widget);
        combo_box_plp_id->setObjectName(QString::fromUtf8("combo_box_plp_id"));

        gridLayout_11->addWidget(combo_box_plp_id, 1, 1, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_11->addItem(horizontalSpacer_4, 1, 2, 1, 1);

        label_snr = new QLabel(widget);
        label_snr->setObjectName(QString::fromUtf8("label_snr"));

        gridLayout_11->addWidget(label_snr, 5, 0, 1, 2);


        gridLayout_9->addWidget(widget, 0, 4, 1, 1);

        tab_widget->addTab(tab_qam_demapper, QString());
        tab_l1_signalling = new QWidget();
        tab_l1_signalling->setObjectName(QString::fromUtf8("tab_l1_signalling"));
        gridLayout_12 = new QGridLayout(tab_l1_signalling);
        gridLayout_12->setObjectName(QString::fromUtf8("gridLayout_12"));
        label_3 = new QLabel(tab_l1_signalling);
        label_3->setObjectName(QString::fromUtf8("label_3"));

        gridLayout_12->addWidget(label_3, 0, 0, 1, 1);

        label_5 = new QLabel(tab_l1_signalling);
        label_5->setObjectName(QString::fromUtf8("label_5"));

        gridLayout_12->addWidget(label_5, 1, 0, 1, 1);

        label_4 = new QLabel(tab_l1_signalling);
        label_4->setObjectName(QString::fromUtf8("label_4"));

        gridLayout_12->addWidget(label_4, 0, 1, 1, 1);

        label_6 = new QLabel(tab_l1_signalling);
        label_6->setObjectName(QString::fromUtf8("label_6"));

        gridLayout_12->addWidget(label_6, 1, 1, 1, 1);

        text_edit_l1_postsignalling = new QTextEdit(tab_l1_signalling);
        text_edit_l1_postsignalling->setObjectName(QString::fromUtf8("text_edit_l1_postsignalling"));
        QSizePolicy sizePolicy5(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy5.setHorizontalStretch(2);
        sizePolicy5.setVerticalStretch(0);
        sizePolicy5.setHeightForWidth(text_edit_l1_postsignalling->sizePolicy().hasHeightForWidth());
        text_edit_l1_postsignalling->setSizePolicy(sizePolicy5);
        text_edit_l1_postsignalling->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
        text_edit_l1_postsignalling->setReadOnly(true);

        gridLayout_12->addWidget(text_edit_l1_postsignalling, 2, 1, 2, 1);

        text_edit_l1_presignalling = new QTextEdit(tab_l1_signalling);
        text_edit_l1_presignalling->setObjectName(QString::fromUtf8("text_edit_l1_presignalling"));
        sizePolicy5.setHeightForWidth(text_edit_l1_presignalling->sizePolicy().hasHeightForWidth());
        text_edit_l1_presignalling->setSizePolicy(sizePolicy5);
        text_edit_l1_presignalling->setReadOnly(true);

        gridLayout_12->addWidget(text_edit_l1_presignalling, 2, 0, 2, 1);

        check_box_auto_update_l1_dynamic = new QCheckBox(tab_l1_signalling);
        check_box_auto_update_l1_dynamic->setObjectName(QString::fromUtf8("check_box_auto_update_l1_dynamic"));
        QSizePolicy sizePolicy6(QSizePolicy::Minimum, QSizePolicy::Fixed);
        sizePolicy6.setHorizontalStretch(1);
        sizePolicy6.setVerticalStretch(0);
        sizePolicy6.setHeightForWidth(check_box_auto_update_l1_dynamic->sizePolicy().hasHeightForWidth());
        check_box_auto_update_l1_dynamic->setSizePolicy(sizePolicy6);

        gridLayout_12->addWidget(check_box_auto_update_l1_dynamic, 3, 3, 1, 1);

        text_edit_l1_postsignalling_dynamic = new QTextEdit(tab_l1_signalling);
        text_edit_l1_postsignalling_dynamic->setObjectName(QString::fromUtf8("text_edit_l1_postsignalling_dynamic"));
        QSizePolicy sizePolicy7(QSizePolicy::Expanding, QSizePolicy::Expanding);
        sizePolicy7.setHorizontalStretch(1);
        sizePolicy7.setVerticalStretch(0);
        sizePolicy7.setHeightForWidth(text_edit_l1_postsignalling_dynamic->sizePolicy().hasHeightForWidth());
        text_edit_l1_postsignalling_dynamic->setSizePolicy(sizePolicy7);
        text_edit_l1_postsignalling_dynamic->setReadOnly(true);

        gridLayout_12->addWidget(text_edit_l1_postsignalling_dynamic, 2, 2, 1, 2);

        label_9 = new QLabel(tab_l1_signalling);
        label_9->setObjectName(QString::fromUtf8("label_9"));
        sizePolicy.setHeightForWidth(label_9->sizePolicy().hasHeightForWidth());
        label_9->setSizePolicy(sizePolicy);

        gridLayout_12->addWidget(label_9, 0, 2, 1, 2);

        label_10 = new QLabel(tab_l1_signalling);
        label_10->setObjectName(QString::fromUtf8("label_10"));
        sizePolicy.setHeightForWidth(label_10->sizePolicy().hasHeightForWidth());
        label_10->setSizePolicy(sizePolicy);

        gridLayout_12->addWidget(label_10, 1, 2, 1, 2);

        tab_widget->addTab(tab_l1_signalling, QString());
        tab_transport_stream = new QWidget();
        tab_transport_stream->setObjectName(QString::fromUtf8("tab_transport_stream"));
        tab_transport_stream->setEnabled(true);
        gridLayout_13 = new QGridLayout(tab_transport_stream);
        gridLayout_13->setObjectName(QString::fromUtf8("gridLayout_13"));
        group_box_ts_info = new QGroupBox(tab_transport_stream);
        group_box_ts_info->setObjectName(QString::fromUtf8("group_box_ts_info"));
        gridLayout_15 = new QGridLayout(group_box_ts_info);
        gridLayout_15->setObjectName(QString::fromUtf8("gridLayout_15"));
        text_edit_ts_stage = new QTextEdit(group_box_ts_info);
        text_edit_ts_stage->setObjectName(QString::fromUtf8("text_edit_ts_stage"));
        text_edit_ts_stage->setEnabled(true);

        gridLayout_15->addWidget(text_edit_ts_stage, 0, 0, 1, 1);


        gridLayout_13->addWidget(group_box_ts_info, 0, 1, 1, 1);

        group_box_ts_settings = new QGroupBox(tab_transport_stream);
        group_box_ts_settings->setObjectName(QString::fromUtf8("group_box_ts_settings"));
        gridLayout_14 = new QGridLayout(group_box_ts_settings);
        gridLayout_14->setObjectName(QString::fromUtf8("gridLayout_14"));
        line_edit_ts_file_name = new QLineEdit(group_box_ts_settings);
        line_edit_ts_file_name->setObjectName(QString::fromUtf8("line_edit_ts_file_name"));
        line_edit_ts_file_name->setReadOnly(true);

        gridLayout_14->addWidget(line_edit_ts_file_name, 4, 0, 1, 3);

        horizontalSpacer_5 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_14->addItem(horizontalSpacer_5, 0, 2, 1, 1);

        push_button_ts_open_file = new QPushButton(group_box_ts_settings);
        push_button_ts_open_file->setObjectName(QString::fromUtf8("push_button_ts_open_file"));
        push_button_ts_open_file->setMaximumSize(QSize(100, 16777215));

        gridLayout_14->addWidget(push_button_ts_open_file, 3, 1, 1, 1);

        radio_button_ts_net = new QRadioButton(group_box_ts_settings);
        radio_button_ts_net->setObjectName(QString::fromUtf8("radio_button_ts_net"));
        radio_button_ts_net->setEnabled(true);
        radio_button_ts_net->setCheckable(true);
        radio_button_ts_net->setChecked(true);

        gridLayout_14->addWidget(radio_button_ts_net, 2, 0, 1, 1);

        radio_button_ts_file = new QRadioButton(group_box_ts_settings);
        radio_button_ts_file->setObjectName(QString::fromUtf8("radio_button_ts_file"));
        radio_button_ts_file->setChecked(false);

        gridLayout_14->addWidget(radio_button_ts_file, 3, 0, 1, 1);

        push_button_ts_apply = new QPushButton(group_box_ts_settings);
        push_button_ts_apply->setObjectName(QString::fromUtf8("push_button_ts_apply"));
        QSizePolicy sizePolicy8(QSizePolicy::Preferred, QSizePolicy::Fixed);
        sizePolicy8.setHorizontalStretch(0);
        sizePolicy8.setVerticalStretch(0);
        sizePolicy8.setHeightForWidth(push_button_ts_apply->sizePolicy().hasHeightForWidth());
        push_button_ts_apply->setSizePolicy(sizePolicy8);
        push_button_ts_apply->setMaximumSize(QSize(100, 16777215));

        gridLayout_14->addWidget(push_button_ts_apply, 5, 0, 1, 1);

        label_7 = new QLabel(group_box_ts_settings);
        label_7->setObjectName(QString::fromUtf8("label_7"));
        QSizePolicy sizePolicy9(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy9.setHorizontalStretch(0);
        sizePolicy9.setVerticalStretch(0);
        sizePolicy9.setHeightForWidth(label_7->sizePolicy().hasHeightForWidth());
        label_7->setSizePolicy(sizePolicy9);
        label_7->setMaximumSize(QSize(60, 16777215));

        gridLayout_14->addWidget(label_7, 0, 0, 1, 1);

        line_edit_ts_udp_port = new QLineEdit(group_box_ts_settings);
        line_edit_ts_udp_port->setObjectName(QString::fromUtf8("line_edit_ts_udp_port"));
        sizePolicy8.setHeightForWidth(line_edit_ts_udp_port->sizePolicy().hasHeightForWidth());
        line_edit_ts_udp_port->setSizePolicy(sizePolicy8);
        line_edit_ts_udp_port->setMaximumSize(QSize(100, 16777215));

        gridLayout_14->addWidget(line_edit_ts_udp_port, 2, 1, 1, 1);

        verticalSpacer_5 = new QSpacerItem(20, 40, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_14->addItem(verticalSpacer_5, 1, 0, 1, 1);

        combo_box_ts_plp = new QComboBox(group_box_ts_settings);
        combo_box_ts_plp->setObjectName(QString::fromUtf8("combo_box_ts_plp"));
        combo_box_ts_plp->setMaximumSize(QSize(100, 16777215));

        gridLayout_14->addWidget(combo_box_ts_plp, 0, 1, 1, 1);


        gridLayout_13->addWidget(group_box_ts_settings, 0, 0, 1, 1);

        tab_widget->addTab(tab_transport_stream, QString());
        tab = new QWidget();
        tab->setObjectName(QString::fromUtf8("tab"));
        gridLayout_16 = new QGridLayout(tab);
        gridLayout_16->setObjectName(QString::fromUtf8("gridLayout_16"));
        widget_p1_correlation = new QCustomPlot(tab);
        widget_p1_correlation->setObjectName(QString::fromUtf8("widget_p1_correlation"));

        gridLayout_16->addWidget(widget_p1_correlation, 0, 0, 1, 1);

        widget_equalizer = new QCustomPlot(tab);
        widget_equalizer->setObjectName(QString::fromUtf8("widget_equalizer"));

        gridLayout_16->addWidget(widget_equalizer, 0, 1, 1, 1);

        tab_widget->addTab(tab, QString());

        gridLayout_7->addWidget(tab_widget, 0, 0, 1, 1);

        main_window->setCentralWidget(centralwidget);
        menubar = new QMenuBar(main_window);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 944, 21));
        menu_file = new QMenu(menubar);
        menu_file->setObjectName(QString::fromUtf8("menu_file"));
        menu_open = new QMenu(menu_file);
        menu_open->setObjectName(QString::fromUtf8("menu_open"));
        main_window->setMenuBar(menubar);
        statusbar = new QStatusBar(main_window);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        main_window->setStatusBar(statusbar);

        menubar->addAction(menu_file->menuAction());
        menu_file->addAction(menu_open->menuAction());
        menu_file->addAction(action_exit);
        menu_open->addAction(action_sdrplay);
        menu_open->addAction(action_airspy);
        menu_open->addAction(action_plutosdr);

        retranslateUi(main_window);

        tab_widget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(main_window);
    } // setupUi

    void retranslateUi(QMainWindow *main_window)
    {
        main_window->setWindowTitle(QApplication::translate("main_window", "MainWindow", nullptr));
        action_exit->setText(QApplication::translate("main_window", "Exit", nullptr));
        action_sdrplay->setText(QApplication::translate("main_window", "SdrPlay", nullptr));
        action_airspy->setText(QApplication::translate("main_window", "AirSpy", nullptr));
        action_plutosdr->setText(QApplication::translate("main_window", "PlutoSDR", nullptr));
        group_box_info->setTitle(QApplication::translate("main_window", "Info :", nullptr));
        label_name->setText(QApplication::translate("main_window", "Name :", nullptr));
        label_info_gain->setText(QApplication::translate("main_window", "gain  :", nullptr));
        label_info_rf->setText(QApplication::translate("main_window", "radio frequency (Hz) : ", nullptr));
        label_ser_no->setText(QApplication::translate("main_window", "Serial No : ", nullptr));
        label_hw_ver->setText(QApplication::translate("main_window", "Hardware ver:", nullptr));
        group_box_settings->setTitle(QApplication::translate("main_window", "Settings :", nullptr));
        line_edit_rf->setText(QApplication::translate("main_window", "586000000", nullptr));
        push_button_stop->setText(QApplication::translate("main_window", "Stop", nullptr));
        check_box_agc->setText(QApplication::translate("main_window", "AGC", nullptr));
        push_button_start->setText(QApplication::translate("main_window", "Sart", nullptr));
        line_edit_gain->setText(QApplication::translate("main_window", "72", nullptr));
        label_gain->setText(QApplication::translate("main_window", "gain  :", nullptr));
        label_rf->setText(QApplication::translate("main_window", "radio frequency (Hz) :", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab_device), QApplication::translate("main_window", "Device", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab_p1_symbol), QApplication::translate("main_window", "P1 symbol", nullptr));
        radio_button_data->setText(QApplication::translate("main_window", "Data", nullptr));
        radio_button_l1postsignaling->setText(QApplication::translate("main_window", "L1 post", nullptr));
        radio_button_l1presignaling->setText(QApplication::translate("main_window", "L1 pre", nullptr));
        label->setText(QApplication::translate("main_window", "Modulation : ", nullptr));
        label_8->setText(QString());
        tab_widget->setTabText(tab_widget->indexOf(tab_p2_symbol), QApplication::translate("main_window", "P2 symbol", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab_data_symbol), QApplication::translate("main_window", "Data symbols", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(frame_closing_symbol), QApplication::translate("main_window", "Frame closing symbol", nullptr));
        label_2->setText(QApplication::translate("main_window", "PLP id : ", nullptr));
        label_snr->setText(QApplication::translate("main_window", "SNR", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab_qam_demapper), QApplication::translate("main_window", "QAM demapper", nullptr));
        label_3->setText(QApplication::translate("main_window", "L1 presignalling data:", nullptr));
        label_5->setText(QApplication::translate("main_window", "name:         value:", nullptr));
        label_4->setText(QApplication::translate("main_window", "L1postsignalling data:", nullptr));
        label_6->setText(QApplication::translate("main_window", "plp:  name:         value:", nullptr));
        check_box_auto_update_l1_dynamic->setText(QApplication::translate("main_window", "Auto update", nullptr));
        label_9->setText(QApplication::translate("main_window", "L1 postsignalling dynamic data:", nullptr));
        label_10->setText(QApplication::translate("main_window", "plp: name:          value:", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab_l1_signalling), QApplication::translate("main_window", "L1 signalling data", nullptr));
        group_box_ts_info->setTitle(QApplication::translate("main_window", "Output stage info :", nullptr));
        group_box_ts_settings->setTitle(QApplication::translate("main_window", "Output settings :", nullptr));
        push_button_ts_open_file->setText(QApplication::translate("main_window", "Open file", nullptr));
        radio_button_ts_net->setText(QApplication::translate("main_window", "Network. UDP port :", nullptr));
        radio_button_ts_file->setText(QApplication::translate("main_window", "File", nullptr));
        push_button_ts_apply->setText(QApplication::translate("main_window", "Apply", nullptr));
        label_7->setText(QApplication::translate("main_window", "PLP :", nullptr));
        line_edit_ts_udp_port->setInputMask(QApplication::translate("main_window", "99999", nullptr));
        line_edit_ts_udp_port->setText(QApplication::translate("main_window", "7654", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab_transport_stream), QApplication::translate("main_window", "Transport stream", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab), QApplication::translate("main_window", "Miscellaneous plot", nullptr));
        menu_file->setTitle(QApplication::translate("main_window", "File", nullptr));
        menu_open->setTitle(QApplication::translate("main_window", "Open", nullptr));
    } // retranslateUi

};

namespace Ui {
    class main_window: public Ui_main_window {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAIN_WINDOW_H
