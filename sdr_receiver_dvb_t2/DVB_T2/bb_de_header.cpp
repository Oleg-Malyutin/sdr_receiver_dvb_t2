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
#include "bb_de_header.h"

#include <QMessageBox>
#include <QTextStream>

#include <QDebug>

#define CRC_POLY 0xAB
#define CRC_POLYR 0xD5
#define TRANSPORT_ERROR_INDICATOR 0x80
#define BIT_PACKET_LENGTH TRANSPORT_PACKET_LENGTH * 8

//------------------------------------------------------------------------------------------
bb_de_header::bb_de_header(QWaitCondition *_signal_in, QMutex *_mutex_in, QObject *parent) :
    QObject(parent),
    signal_in(_signal_in),
    mutex_in(_mutex_in)
{
    init_crc8_table();

    out = new uint8_t[53840 / 8];
    begin_out = out;
    buffer_out = new char[53840 / 8];

    stream = new QDataStream;
    stream->setVersion(QDataStream::Qt_5_1);

    socket = new QUdpSocket(this);
}
//------------------------------------------------------------------------------------------
bb_de_header::~bb_de_header()
{
    if(socket->isOpen()) socket->close();
    delete socket;
    if(file != nullptr) {
        if(file->isOpen()) file->close();
        delete file;
    }
    delete [] out;
}
//------------------------------------------------------------------------------------------
void bb_de_header::init_crc8_table()
{
    int r, crc;
    for (int i = 0; i < 256; ++i) {
        r = i;
        crc = 0;
        for (int j = 7; j >= 0; --j) {
            if ((r & (1 << j) ? 1 : 0) ^ ((crc & 0x80) ? 1 : 0)) crc = (crc << 1) ^ CRC_POLYR;
            else  crc <<= 1;
        }
        crc_table[i] = static_cast<uint8_t>(crc);
    }
}
//------------------------------------------------------------------------------------------
uint8_t  bb_de_header::check_crc8_mode(uint8_t *_in, int _len_in)
{
    uint8_t crc = 0;
    uint8_t b;
    int len_in = _len_in;
    uint8_t* in = _in;
    for (int i = 0; i < len_in; ++i) {
        b = in[i] ^ (crc & 0x01);
        crc >>= 1;
        if (b) crc ^= CRC_POLY;
    }
    return crc;
}
//------------------------------------------------------------------------------------------
void bb_de_header::execute(int _plp_id, l1_postsignalling _l1_post, int _len_in, uint8_t* _in)
{
    mutex_in->lock();
    signal_in->wakeOne();
    l1_postsignalling l1_post = _l1_post;
    int len_in = _len_in;
    uint8_t* in = _in;
    dvbt2_inputmode_t mode;
    bool check_mode;
    int errors = 0;
    int len_split = 0;
    int len_out = 0;
    uint8_t temp;
    uint8_t* ptr_error_indicator = nullptr;
    switch(check_crc8_mode(in, BB_HEADER_LENGTH_BITS)){
    case 0:
        mode = INPUTMODE_NORMAL;
        check_mode = true;
        break;
    case CRC_POLY:
        mode = INPUTMODE_HIEFF;
        check_mode = true;
        break;
    default:
        info_already_set = false;
        emit ts_stage("Baseband header CRC8 error.");
        mutex_in->unlock();
        return;
    }
    bb_header header;
    header.ts_gs = *in++ << 1;
    header.ts_gs |= *in++;
    header.sis_mis = *in++;
    header.ccm_acm = *in++;
    header.issyi = *in++;
    header.npd = *in++;
    header.ext = *in++ << 1;
    header.ext |= *in++;
    header.isi = 0;
    if (header.sis_mis == 0) {
      for (int i = 7; i >= 0; --i) {
        header.isi |= *in++ << i;
      }
    }
    else {
      in += 8;
    }

    if(!info_already_set) set_info(_plp_id, l1_post, mode, header);

    if(need_plp != _plp_id) {
        mutex_in->unlock();
        return;
    }

    header.upl = 0;
    for (int i = 15; i >= 0; --i) {
      header.upl |= *in++ << i;
    }
    header.dfl = 0;
    for (int i = 15; i >= 0; --i) {
      header.dfl |= *in++ << i;
    }
    header.sync = 0;
    for (int i = 7; i >= 0; --i) {
      header.sync |= *in++ << i;
    }
    header.syncd = 0;
    for (int i = 15; i >= 0; --i) {
      header.syncd |= *in++ << i;
    }
    if(header.syncd == 65535) {
        mutex_in->unlock();
        return;
    }
    in += 8;

    if (mode == INPUTMODE_NORMAL) {
        if(split){
            split = false;
            *out++ = buffer[0];
            ptr_error_indicator = out;
            ++len_out;
            for (int i = 1; i < idx_buffer; ++i) {
                *out++ = buffer[i];
                ++len_out;
            }
            len_split = TRANSPORT_PACKET_LENGTH - idx_packet;
            int syncd_byte = header.syncd / 8;
            if(len_split == syncd_byte){
                for (int i = 0; i < len_split; ++i) {
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    crc = crc_table[temp ^ crc];
                    *out++ = temp;
                    ++len_out;
                    ++idx_packet;
                }
                temp = 0;
                for (int n = 7; n >= 0; n--) {
                    temp |= *in++ << n;
                }
                if(temp != crc){
                    ++errors;
                    *ptr_error_indicator |= TRANSPORT_ERROR_INDICATOR;
                }
                crc = 0;
            }
            else if(len_split < syncd_byte){
                for (int i = 0; i < syncd_byte; ++i) {
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    crc = crc_table[temp ^ crc];
                    *out++ = temp;
                    ++len_out;
                    ++idx_packet;
                }
                temp = 0;
                for (int n = 7; n >= 0; n--) {
                    temp |= *in++ << n;
                }
                if(temp != crc){
                    ++errors;
                    *ptr_error_indicator |= TRANSPORT_ERROR_INDICATOR;
                }
                crc = 0;
                emit ts_stage("Baseband header resynchronizing.");
            }
            else{
                for (int i = 0; i < syncd_byte; ++i) {
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    *out++ = temp;
                    ++len_out;
                    ++idx_packet;
                }
                int dump = len_split - syncd_byte;
                for (int i = 0; i < dump; ++i) {
                    *out++ = 0xF0;
                    ++len_out;
                    ++idx_packet;
                }
                ++errors;
                *ptr_error_indicator |= TRANSPORT_ERROR_INDICATOR;
                emit ts_stage("Baseband header resynchronizing.");
            }
        }
        else {
            in += header.syncd + 8;
        }
        header.dfl -= header.syncd + 8;
        while (header.dfl > 0) {
            if (header.dfl < BIT_PACKET_LENGTH) {
                split = true;
                len_split = header.dfl / 8;
                idx_buffer = 0;
                for (int i = 0; i < len_split; ++i) {
                    if(idx_packet == TRANSPORT_PACKET_LENGTH) {
                        idx_packet = 0;
                        temp = 0;
                        for (int n = 7; n >= 0; n--) {
                            temp |= *in++ << n;
                        }
                        if(temp != crc){
                            ++errors;
                            *ptr_error_indicator |= TRANSPORT_ERROR_INDICATOR;
                        }
                        crc = 0;
                        buffer[idx_buffer++] = 0x47;//static_cast<unsigned char>(header.sync);
                        ++idx_packet;
                    }
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    crc = crc_table[temp ^ crc];
                    buffer[idx_buffer++] = temp;
                    ++idx_packet;
                }
                header.dfl = 0;
            }
            else{
                if(idx_packet == TRANSPORT_PACKET_LENGTH){
                    idx_packet = 0;
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    if(temp != crc){
                        ++errors;
                        *ptr_error_indicator |= TRANSPORT_ERROR_INDICATOR;
                    }
                    crc = 0;
                    *out++ = 0x47;//static_cast<unsigned char>(header.sync);
                    ++len_out;
                    ++idx_packet;
                    ptr_error_indicator = out;
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    crc = crc_table[temp ^ crc];
                    *out++ = temp;
                    ++len_out;
                    ++idx_packet;
                    header.dfl -= 8;
                }
                else if(idx_packet == 0){
                    *out++ = 0x47;//static_cast<unsigned char>(header.sync);
                    ++len_out;
                    ++idx_packet;
                    ptr_error_indicator = out;
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    crc = crc_table[temp ^ crc];
                    *out++ = temp;
                    ++len_out;
                    ++idx_packet;
                    header.dfl -= 8;
                }
                else{
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    crc = crc_table[temp ^ crc];
                    *out++ = temp;
                    ++len_out;
                    ++idx_packet;
                    header.dfl -= 8;
                }
            }
        }
    }
    else {
        if(split){
            split = false;
            for (int i = 0; i < idx_buffer; ++i) {
                *out++ = buffer[i];
                ++len_out;
            }
            len_split = TRANSPORT_PACKET_LENGTH - idx_packet;
            int syncd_byte = header.syncd / 8;
            if(len_split == syncd_byte) {
                for (int i = 0; i < len_split; ++i) {
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    *out++ = temp;
                    ++len_out;
                    ++idx_packet;
                }
            }
            else if(len_split < syncd_byte){
                for (int i = 0; i < len_split; ++i) {
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    *out++ = temp;
                    ++len_out;
                    ++idx_packet;
                }
                in += header.syncd - len_split * 8;
                emit ts_stage("Baseband header resynchronizing.");
            }
            else{
                for (int i = 0; i < syncd_byte; ++i) {
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    *out++ = temp;
                    ++len_out;
                    ++idx_packet;
                }
                int dump = len_split - syncd_byte;
                for (int i = 0; i < dump; ++i) {
                    *out++ = 0xF0;
                    ++len_out;
                    ++idx_packet;
                }
                emit ts_stage("Baseband header resynchronizing.");
            }
        }
        else {
            in += header.syncd;
        }
        header.dfl -= header.syncd;
        while (header.dfl > 0) {
            if (header.dfl < BIT_PACKET_LENGTH) {
                split = true;
                len_split = header.dfl / 8;
                idx_buffer = 0;
                for (int i = 0; i < len_split; ++i) {
                    if(idx_packet == TRANSPORT_PACKET_LENGTH) {
                        idx_packet = 0;
                        buffer[idx_buffer++] = 0x47;//static_cast<unsigned char>(header.sync);
                        ++idx_packet;
                    }
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    buffer[idx_buffer++] = temp;
                    ++idx_packet;
                }
                header.dfl = 0;
            }
            else{
                if(idx_packet == TRANSPORT_PACKET_LENGTH || idx_packet == 0){
                    idx_packet = 0;
                    *out++ = 0x47;//static_cast<unsigned char>(header.sync);
                    ++len_out;
                    ++idx_packet;
                }
                else{
                    temp = 0;
                    for (int n = 7; n >= 0; n--) {
                        temp |= *in++ << n;
                    }
                    *out++ = temp;
                    ++len_out;
                    ++idx_packet;
                    header.dfl -= 8;
                }
            }
        }
    }
    out = begin_out;
    memcpy(buffer_out, out, sizeof(uint8_t) * static_cast<unsigned long>(len_out));

    switch(id_current_out){
    case out_network:
        socket->writeDatagram(buffer_out, len_out, QHostAddress::LocalHost, num_port_udp);// $ vlc udp://@:7654
        break;
    case out_file:
        stream->writeRawData(buffer_out, len_out);
        break;
    }

    if(errors != 0) emit ts_stage("TS error.");

    mutex_in->unlock();
}
//_____________________________________________________________________________________________
void bb_de_header::set_info(int _plp_id, l1_postsignalling _l1_post,
                            dvbt2_inputmode_t mode, bb_header header)
{
    if(_plp_id != next_plp_info) return;

    QString temp;
    info += "PLP :\t" + QString::number(_plp_id) + "\n";
    if(mode == INPUTMODE_HIEFF) temp = "HEM";
    else temp = "NM";
    info += "Mode\t\t" + temp + "\n";
    switch(header.ts_gs){
    case 0:
        temp = "GFPS(not supported)";
    break;
    case 1:
        temp = "GCS(not supported)";
    break;
    case 2:
        temp = "GSE(not supported)";
    break;
    case 3:
        temp = "TS";
    break;
    default:
        temp = "unknow";
    break;
    }
    info += "TS/GS\t\t" + temp + "\n";
    if(header.sis_mis) temp = "single";
    else temp = "multiple";
    info += "SIS/MIS\t\t " + temp + "\n";
    if(header.issyi) temp = "yes";
    else temp = "no";
    info += "ISSYI\t\t" + temp + "\n";
    if(header.npd)temp = "yes";
    else temp = "no";
    info += "NDP\t\t" + temp;

    ++next_plp_info;
    if(next_plp_info == _l1_post.num_plp) {
        next_plp_info = 0;
        info_already_set = true;
        emit ts_stage(info);
    }
    else{
        info += "\n";
    }
}
//_____________________________________________________________________________________________
void bb_de_header::set_out(id_out _id_current_out, int _num_port_udp,
                           QString _file_name, int _need_plp)
{
    id_current_out = _id_current_out;
    file_name = _file_name;
    num_port_udp = static_cast<unsigned short>(_num_port_udp);
    need_plp = _need_plp;
    if(id_current_out == out_file){
        if(file != nullptr) {
            if(file->isOpen()) file->close();
            delete file;
        }
        file = new QFile(file_name);
        if(file->open(QIODevice::WriteOnly)) {
            stream->setDevice(file);
        }
        else{
            QMessageBox::information(nullptr, "error", file->errorString());
        }
    }
    else{
        if(file != nullptr) {
            if(file->isOpen()) file->close();
        }
    }
}
//_____________________________________________________________________________________________
void bb_de_header::stop()
{
    emit finished();
}
//_____________________________________________________________________________________________
