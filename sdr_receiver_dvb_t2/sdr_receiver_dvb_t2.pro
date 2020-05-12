
#  Copyright 2020 Oleg Malyutin.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

QT       += core gui
QT       += charts multimedia
QT       += network
QT       += printsupport
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
CONFIG += thread

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

QMAKE_CXXFLAGS += -Ofast
QMAKE_CXXFLAGS += -mavx2

SOURCES += \
    DVB_T2/LDPC/tables_handler.cc \
    DVB_T2/address_freq_deinterleaver.cpp \
    DVB_T2/bb_de_header.cpp \
    DVB_T2/bch_decoder.cpp \
    DVB_T2/data_symbol.cpp \
    DVB_T2/dvbt2_definition.cpp \
    DVB_T2/dvbt2_frame.cpp \
    DVB_T2/fc_symbol.cpp \
    DVB_T2/ldpc_decoder.cpp \
    DVB_T2/llr_demapper.cpp \
    DVB_T2/p1_symbol.cpp \
    DVB_T2/p2_symbol.cpp \
    DVB_T2/pilot_generator.cpp \
    DVB_T2/time_deinterleaver.cpp \
    main.cpp \
    main_window.cpp \
    plot.cpp \
    qcustomplot.cpp \
    sdrplay.cpp

HEADERS += \
    DSP/buffers.hh \
    DSP/fast_fourier_transform.h \
    DSP/fast_math.h \
    DSP/filter_decimator.h \
    DSP/interpolator_farrow.hh \
    DSP/loop_filters.hh \
    DVB_T2/LDPC/algorithms.hh \
    DVB_T2/LDPC/avx2.hh \
    DVB_T2/LDPC/dvb_t2_tables.hh \
    DVB_T2/LDPC/exclusive_reduce.hh \
    DVB_T2/LDPC/generic.hh \
    DVB_T2/LDPC/layered_decoder.hh \
    DVB_T2/LDPC/ldpc.hh \
    DVB_T2/LDPC/neon.hh \
    DVB_T2/LDPC/simd.hh \
    DVB_T2/LDPC/sse4_1.hh \
    DVB_T2/address_freq_deinterleaver.h \
    DVB_T2/bb_de_header.h \
    DVB_T2/bch_decoder.h \
    DVB_T2/data_symbol.h \
    DVB_T2/dvbt2_definition.h \
    DVB_T2/dvbt2_frame.h \
    DVB_T2/fc_symbol.h \
    DVB_T2/ldpc_decoder.h \
    DVB_T2/llr_demapper.h \
    DVB_T2/p1_symbol.h \
    DVB_T2/p2_symbol.h \
    DVB_T2/pilot_generator.h \
    DVB_T2/time_deinterleaver.h \
    main_window.h \
    plot.h \
    qcustomplot.h \
    sdrplay.h

FORMS += \
    main_window.ui

unix|win32: LIBS += -lfftw3f
unix|win32: LIBS += -lmirsdrapi-rsp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

