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
#ifndef BB_DE_HEADER_H
#define BB_DE_HEADER_H

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QUdpSocket>
#include <QFile>

#include "dvbt2_definition.h"

#define BB_HEADER_LENGTH_BITS 80
#define TS_GS_TRANSPORT          3
#define TS_GS_GENERIC_PACKETIZED 0
#define TS_GS_GENERIC_CONTINUOUS 1
#define TS_GS_RESERVED           2
#define SIS_MIS_SINGLE   1
#define SIS_MIS_MULTIPLE 0
#define CCM 1
#define ACM 0
#define ISSYI_ACTIVE     1
#define ISSYI_NOT_ACTIVE 0
#define NPD_ACTIVE       1
#define NPD_NOT_ACTIVE   0

#define TRANSPORT_PACKET_LENGTH 188

class bb_de_header : public QObject
{
    Q_OBJECT
public:
    explicit bb_de_header(QWaitCondition* _signal_in, QMutex* _mutex_in, QObject *parent = nullptr);
    ~bb_de_header();

    enum id_out{
        out_network = 0,
        out_file,
    };

signals:
    void finished();
    void ts_stage(QString _info);

public slots:
    void execute(int _plp_id, l1_postsignalling _l1_post, int _len_in, uint8_t* _in);
    void set_out(bb_de_header::id_out _id_current_out, int _num_port_udp, QString _file_name, int _need_plp);
    void stop();

private:
    QWaitCondition* signal_in;
    QMutex* mutex_in;
    QMutex* mutex_out;
    int plp_id = 0;
    uint8_t  crc = 0;
    uint8_t crc_table[256];
    void init_crc8_table();
    uint8_t check_crc8_mode(uint8_t *_in, int _len_in);
    struct bb_header{
        int ts_gs;
        int sis_mis;
        int ccm_acm;
        int issyi;
        int npd;
        int ext;
        int isi;
        int upl;
        int dfl;
        int sync;
        int syncd;
    };
    int idx_packet = 0;
    int idx_buffer = 0;
    bool split = false;
    uint8_t buffer[TRANSPORT_PACKET_LENGTH];

    int need_plp = 0;
    uint8_t* begin_out;
    uint8_t* out;
    int len;
    char* buffer_out;
    QFile* file = nullptr;
    QDataStream* stream;
    QUdpSocket* socket;
    int id_current_out = out_network;
    QString file_name = "out_dvbt2.ts";
    unsigned short num_port_udp = 7654;

    bool info_already_set = false;
    QString info = "";
    int next_plp_info = 0;
    void set_info(int _plp_id, l1_postsignalling _l1_post, dvbt2_inputmode_t mode, bb_header header);
};

#endif // BB_DE_HEADER_H
