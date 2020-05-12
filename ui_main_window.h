/********************************************************************************
** Form generated from reading UI file 'main_window.ui'
**
** Created by: Qt User Interface Compiler version 5.12.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_MAIN_WINDOW_H
#define UI_MAIN_WINDOW_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_main_window
{
public:
    QWidget *centralwidget;
    QGridLayout *gridLayout;
    QTabWidget *tab_widget;
    QWidget *tab_device;
    QWidget *tab_p1_symbol;
    QWidget *tab_p2_symbol;
    QWidget *tab_data_symbol;
    QWidget *tab_transport_stream;
    QMenuBar *menubar;
    QStatusBar *statusbar;

    void setupUi(QMainWindow *main_window)
    {
        if (main_window->objectName().isEmpty())
            main_window->setObjectName(QString::fromUtf8("main_window"));
        main_window->resize(800, 600);
        centralwidget = new QWidget(main_window);
        centralwidget->setObjectName(QString::fromUtf8("centralwidget"));
        gridLayout = new QGridLayout(centralwidget);
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        tab_widget = new QTabWidget(centralwidget);
        tab_widget->setObjectName(QString::fromUtf8("tab_widget"));
        tab_device = new QWidget();
        tab_device->setObjectName(QString::fromUtf8("tab_device"));
        tab_widget->addTab(tab_device, QString());
        tab_p1_symbol = new QWidget();
        tab_p1_symbol->setObjectName(QString::fromUtf8("tab_p1_symbol"));
        tab_widget->addTab(tab_p1_symbol, QString());
        tab_p2_symbol = new QWidget();
        tab_p2_symbol->setObjectName(QString::fromUtf8("tab_p2_symbol"));
        tab_widget->addTab(tab_p2_symbol, QString());
        tab_data_symbol = new QWidget();
        tab_data_symbol->setObjectName(QString::fromUtf8("tab_data_symbol"));
        tab_widget->addTab(tab_data_symbol, QString());
        tab_transport_stream = new QWidget();
        tab_transport_stream->setObjectName(QString::fromUtf8("tab_transport_stream"));
        tab_widget->addTab(tab_transport_stream, QString());

        gridLayout->addWidget(tab_widget, 0, 0, 1, 1);

        main_window->setCentralWidget(centralwidget);
        menubar = new QMenuBar(main_window);
        menubar->setObjectName(QString::fromUtf8("menubar"));
        menubar->setGeometry(QRect(0, 0, 800, 28));
        main_window->setMenuBar(menubar);
        statusbar = new QStatusBar(main_window);
        statusbar->setObjectName(QString::fromUtf8("statusbar"));
        main_window->setStatusBar(statusbar);

        retranslateUi(main_window);

        tab_widget->setCurrentIndex(0);


        QMetaObject::connectSlotsByName(main_window);
    } // setupUi

    void retranslateUi(QMainWindow *main_window)
    {
        main_window->setWindowTitle(QApplication::translate("main_window", "MainWindow", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab_device), QApplication::translate("main_window", "Device", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab_p1_symbol), QApplication::translate("main_window", "P1 symbol", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab_p2_symbol), QApplication::translate("main_window", "P2 symbol", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab_data_symbol), QApplication::translate("main_window", "Data symbols", nullptr));
        tab_widget->setTabText(tab_widget->indexOf(tab_transport_stream), QApplication::translate("main_window", "Transport stream", nullptr));
    } // retranslateUi

};

namespace Ui {
    class main_window: public Ui_main_window {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_MAIN_WINDOW_H
