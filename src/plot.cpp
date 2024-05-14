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
#include "plot.h"

//#include <QDebug>

//-------------------------------------------------------------------------------------------
plot::plot(QCustomPlot *_plot, type_graph _type, QString _name, QObject *parent) :
    QObject(parent),
    current_plot(_plot),
    type(_type),
    name(_name)
{
    type = _type;
    current_plot = _plot;
    current_plot->plotLayout()->insertRow(0);
    current_plot->plotLayout()->addElement(0, 0, new QCPTextElement(current_plot, name));
    connect(this, SIGNAL(repaint_plot()), current_plot, SLOT(replot()));

    if(type == type_null_indicator){

        QPen pen;
        current_plot->addGraph();

        x_data.resize(1);
        y_data.resize(1);
        current_plot->xAxis->setRange(0, 3);
        current_plot->yAxis->setRange(-3, 3);
//        current_plot->xAxis->setLabel("samplerate - frequency");
        current_plot->yAxis->setLabel("Samplerate Hz");
        pen.setWidth(42);
        pen.setColor(Qt::green);
        current_plot->graph(0)->setPen(pen);
        current_plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 3));
        current_plot->graph(0)->setLineStyle(QCPGraph::lsImpulse);

        x_data_2.resize(1);
        y_data_2.resize(1);
        current_plot->addGraph(current_plot->xAxis2, current_plot->yAxis2);
        current_plot->yAxis2->setVisible(true);
        current_plot->xAxis2->setRange(0, 3);
        current_plot->yAxis2->setRange(-100, 100);
//        current_plot->xAxis2->setLabel("samplerate - frequency");
        current_plot->yAxis2->setLabel("Frequency Hz");
        pen.setWidth(42);
        pen.setColor(Qt::red);
        current_plot->graph(1)->setPen(pen);
        current_plot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssNone, 3));
        current_plot->graph(1)->setLineStyle(QCPGraph::lsImpulse);

        QVector<double> ticks;
        QVector<QString> labels;
        ticks << 1 << 2;
        labels << "samplerate" << "frequency";
        QSharedPointer<QCPAxisTickerText> textTicker(new QCPAxisTickerText);
        textTicker->addTicks(ticks, labels);
        current_plot->xAxis->setTicker(textTicker);
        current_plot->xAxis->setTickLabelRotation(180);
        current_plot->xAxis->setSubTicks(false);
        current_plot->xAxis->setTickLength(0, 4);

        // current_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom/* | QCP::iSelectPlottables*/);
//        current_plot->graph(0)->setAntialiasedFill(false);
//        current_plot->graph(0)->data()->clear();



    }
}
//-------------------------------------------------------------------------------------------
plot::~plot()
{

}
//-------------------------------------------------------------------------------------------
void plot::replace_spectrograph(const int _len_data, complex *_data)
{
    const int len_data = _len_data;
    greate_graph(len_data);
    for (int i = 0; i < len_data; i++){
        float y = 10.0f * log10f(norm(_data[i]) / len_data);
        y_data[i] = static_cast<double>(y);
        x_data[i]= i;
    }
    current_plot->graph(0)->data()->clear();
    current_plot->graph(0)->setData(x_data, y_data);

//    calc_frame_per_sec();

    emit  repaint_plot();
}
//-------------------------------------------------------------------------------------------
void plot::replace_constelation(const int _len_data, complex *_data)
{
    int len_data = _len_data;
    greate_graph(len_data);
    static int v = 0;
    if (v == 1){
        current_plot->graph(0)->data()->clear();
        v = 0;
    }
    v++;
    for (int j = 0; j < len_data; j++){
        x_data[j] = static_cast<double>(_data[j].real());
        y_data[j] = static_cast<double>(_data[j].imag());
    }
    current_plot->graph(0)->addData(x_data, y_data);

    emit  repaint_plot();
}
//-------------------------------------------------------------------------------------------
void plot::replace_oscilloscope(const int _len_data, complex *_data)
{
    const int len_data = _len_data;
    greate_graph(len_data, _data);
    for (int i = 0; i < len_data; i++){
        y_data[i] = static_cast<double>(_data[i].real());
        x_data[i] = i;
    }
    current_plot->graph(0)->data()->clear();
    current_plot->graph(0)->setData(x_data, y_data);
    if(type == type_oscilloscope_2){
        for (int i = 0; i < len_data; i++){
            y_data_2[i] = static_cast<double>(_data[i].imag());
            x_data_2[i] = i;
        }
        current_plot->graph(1)->data()->clear();
        current_plot->graph(1)->setData(x_data_2, y_data_2);
    }

    emit  repaint_plot();
}
//-------------------------------------------------------------------------------------------
void plot::replace_null_indicator(const float _b1, const float _b2)
{
    y_data[0] = _b1;
    x_data[0] = 1;

    current_plot->graph(0)->data()->clear();
    current_plot->graph(0)->setData(x_data, y_data);

    y_data_2[0] = _b2;
    x_data_2[0] = 2;

    current_plot->graph(1)->data()->clear();
    current_plot->graph(1)->setData(x_data_2, y_data_2);

    emit  repaint_plot();
}
//-------------------------------------------------------------------------------------------
void plot::greate_graph(int _len_data, complex *_data)
{
    if (check_len_data == _len_data) return;

    const int len_data = _len_data;
    QPen pen;
    double max = 0, min = 0;
    check_len_data = len_data;
    x_data.resize(len_data);
    y_data.resize(len_data);

    current_plot->addGraph();

    switch (type) {
    case type_spectrograph:
        current_plot->xAxis->setRange(0, _len_data);
        current_plot->yAxis->setRange(-60, 6);
        current_plot->xAxis->setLabel("Samples");
        current_plot->yAxis->setLabel("dB");
        pen.setWidth(1);
        pen.setColor(Qt::blue);
        current_plot->graph(0)->setPen(pen);
        current_plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlusCircle, 3));
        current_plot->graph(0)->setLineStyle(QCPGraph::lsStepCenter); 
        break;
    case type_constelation:
        current_plot->xAxis->setLabel("Inphase");
        current_plot->yAxis->setLabel("Quadrature");
        current_plot->xAxis->setRange(-2.0, 2.0);
        current_plot->yAxis->setRange(-2.0, 2.0);
        current_plot->graph(0)->setLineStyle(QCPGraph::lsNone);
        pen.setWidth(5);
        pen.setColor(Qt::blue);
        current_plot->graph(0)->setPen(pen);
        current_plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssDot, 0));
        break;
    case type_oscilloscope:
        double d;
        for(int i = 0; i < len_data; ++i){
            d = static_cast<double>(_data[i].real());
            if(d > max) max = d;
            if(d < min) min = d;
        }
        current_plot->xAxis->setLabel("Samples");
        current_plot->xAxis->setRange(0, len_data);
        current_plot->yAxis->setLabel("Amplitude");
        current_plot->yAxis->setRange(0/*min*/, 1.5e+6/*max*/);
        pen.setColor(Qt::green);
        current_plot->graph(0)->setPen(pen);
        current_plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlusCircle, 3));
        connect(current_plot->xAxis, SIGNAL(rangeChanged(QCPRange)),
                current_plot->xAxis2, SLOT(setRange(QCPRange)));
        break;
    case type_oscilloscope_2:
        double d2;
//        for(int i = 0; i < len_data; ++i){
//            d2 = static_cast<double>(_data[i].real());
//            if(d2 > max) max = d2;
//            if(d2 < min) min = d2;
//        }
        current_plot->xAxis->setLabel("Samples");
        current_plot->xAxis->setRange(0, len_data);
        current_plot->yAxis->setLabel("Angle (rad)");
//        current_plot->yAxis->setRange(min, max);
        current_plot->yAxis->setRange(-10, 10);
        pen.setColor(Qt::green);
        current_plot->graph(0)->setPen(pen);
        current_plot->graph(0)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlusCircle, 3));
        current_plot->graph(0)->setName("angle pilot sinal");
        x_data_2.resize(len_data);
        y_data_2.resize(len_data);
        current_plot->addGraph(current_plot->xAxis2, current_plot->yAxis2);
        for(int i = 0; i < len_data; ++i){
            d2 = static_cast<double>(_data[i].imag());
            if(d2 > max) max = d2;
            if(d2 < min) min = d2;
        }
//        current_plot->xAxis2->setVisible(true);
        current_plot->xAxis2->setRange(0, len_data);
        current_plot->yAxis2->setVisible(true);
        current_plot->yAxis2->setLabel("Amplitude");
        current_plot->yAxis2->setRange(min - 100, max + 100);
        pen.setColor(Qt::red);
        current_plot->graph(1)->setPen(pen);
        current_plot->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlusCircle, 3));
        connect(current_plot->xAxis, SIGNAL(rangeChanged(QCPRange)),
                current_plot->xAxis2, SLOT(setRange(QCPRange)));
//        connect(current_plot->yAxis, SIGNAL(rangeChanged(QCPRange)),
//                current_plot->yAxis2, SLOT(setRange(QCPRange)));
        current_plot->graph(1)->setName("amplitude pilot sinal");
        current_plot->graph(1)->setAntialiasedFill(false);
        current_plot->graph(1)->data()->clear();
        current_plot->legend->setVisible(true);
        current_plot->axisRect()->insetLayout()->setInsetAlignment(0, Qt::AlignBottom|Qt::AlignRight);
        break;
    case type_null_indicator:

        break;
    }
    current_plot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom/* | QCP::iSelectPlottables*/);
    current_plot->graph(0)->setAntialiasedFill(false);
    current_plot->graph(0)->data()->clear();
}
//-------------------------------------------------------------------------------------------
void plot::calc_frame_per_sec()
{
    //calculate frames per second:
    float keyt = 0.0f;
    keyt = QDateTime::currentDateTime().toMSecsSinceEpoch()/1000.0f;
//    qDebug() << "keyt " << keyt;
    static float lastFpsKey;
    static int frameCount = 0;
    ++frameCount;
    if (keyt - lastFpsKey > 0.00001f) // average fps over 2 seconds
    {
//        ui->label->setText(QString::number(frameCount/(keyt-lastFpsKey)));
//        qDebug() << "frameCount " << frameCount/(keyt - lastFpsKey);

        lastFpsKey = keyt;
        frameCount = 0;
    }
}
//-------------------------------------------------------------------------------------------
