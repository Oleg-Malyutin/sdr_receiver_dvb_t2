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
#ifndef PLOT_H
#define PLOT_H

#include <QObject>
#include <QApplication>

#include <complex>

#include "qcustomplot.h"

typedef std::complex<float> complex;
enum type_graph{
    type_spectrograph = 0,
    type_constelation,
    type_oscilloscope,
    type_oscilloscope_2,
};
class plot : public QObject
{
    Q_OBJECT
public:
    explicit plot(QCustomPlot *_plot, type_graph _type, QString _name, QObject *parent = nullptr);
    ~plot();

signals:
    void repaint_plot();

public slots:
    void replace_spectrograph(const int _len_data, complex* _data);
    void replace_constelation(const int _len_data, complex* _data);
    void replace_oscilloscope(const int _len_data, complex* _data);

private:
    QCustomPlot* current_plot;
    QVector<double> x_data;
    QVector<double> y_data;
    QVector<double> x_data_2;
    QVector<double> y_data_2;
    int check_len_data = 0;
    type_graph type;
    QString name;
    void greate_graph(int _len_data, complex *_data = nullptr);

    void calc_frame_per_sec();
};

#endif // PLOT_H
