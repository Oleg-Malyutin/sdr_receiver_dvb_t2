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
#include "bch_decoder.h"

//------------------------------------------------------------------------------------------
bch_decoder::bch_decoder(QWaitCondition *_signal_in, QMutex *_mutex_in, QObject *parent) :
    QObject(parent),
    signal_in(_signal_in),
    mutex_in(_mutex_in)
{
    buffer_a = new uint8_t[53840];
    buffer_b = new uint8_t[53840];
    out = buffer_a;

    init_descrambler();

    mutex_out = new QMutex;
    signal_out = new QWaitCondition;
    deheader = new bb_de_header(signal_out, mutex_out);
    thread = new QThread;
    deheader->moveToThread(thread);
    connect(this, &bch_decoder::bit_descramble, deheader, &bb_de_header::execute);
    connect(this, &bch_decoder::stop_deheader, deheader, &bb_de_header::stop);
    connect(deheader, &bb_de_header::finished, deheader, &bb_de_header::deleteLater);
    connect(deheader, &bb_de_header::finished, thread, &QThread::quit, Qt::DirectConnection);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}
//------------------------------------------------------------------------------------------
bch_decoder::~bch_decoder()
{
    emit stop_deheader();
    if(thread->isRunning()) thread->wait(1000);
    delete [] buffer_a;
    delete [] buffer_b;
}
//------------------------------------------------------------------------------------------
void bch_decoder::init_descrambler()
    {
      int sr = 0x4A80;
      for (int i = 0; i < 54000; i++) {
        uint8_t b = ((sr) ^ (sr >> 1)) & 1;
        descrambler[i] = b;
        sr >>= 1;
        if(b) {
          sr |= 0x4000;
        }
      }
    }
//------------------------------------------------------------------------------------------
void bch_decoder::execute(int *_idx_plp_simd, l1_postsignalling _l1_post, int _len_in, uint8_t* _in)
{
    mutex_in->lock();
    signal_in->wakeOne();

//        mutex_in->unlock();
//        return;

    int* plp_id = _idx_plp_simd;
    l1_postsignalling l1_post = _l1_post;
    int len_in = _len_in;
    uint8_t* in = _in;
    int k_bch;
    int n_bch;
    dvbt2_fectype_t fec_type = static_cast<dvbt2_fectype_t>(l1_post.plp[plp_id[0]].plp_fec_type);
    dvbt2_code_rate_t code_rate = static_cast<dvbt2_code_rate_t>(l1_post.plp[plp_id[0]].plp_cod);
    if(fec_type == FEC_FRAME_NORMAL){
        switch(code_rate){
        case C1_2:
            k_bch = 32208;
            n_bch = 32400;
            break;
        case C3_5:
            k_bch = 38688;
            n_bch = 38880;
            break;
        case C2_3:
            k_bch = 43040;
            n_bch = 43200;
            break;
        case C3_4:
            k_bch = 48408;
            n_bch = 48600;
            break;
        case C4_5:
            k_bch = 51648;
            n_bch = 51840;
            break;
        case C5_6:
            k_bch = 53840;
            n_bch = 54000;
            break;
        }
    }
    else{
        switch(code_rate){
        case C1_2:
            k_bch = 7032;
            n_bch = 7200;
            break;
        case C3_5:
            k_bch = 9552;
            n_bch = 9720;
            break;
        case C2_3:
            k_bch = 10632;
            n_bch = 10800;
            break;
        case C3_4:
            k_bch = 11712;
            n_bch = 11880;
            break;
        case C4_5:
            k_bch = 12432;
            n_bch = 12600;
            break;
        case C5_6:
            k_bch = 13152;
            n_bch = 13320;
            break;
        }
    }

    // TODO BCH decode

    int n = 0;
    for(int j = 0; j < len_in; j += n_bch) {
        for (int i = 0; i < k_bch; ++i) {
            out[i] = in[j + i] ^ descrambler[i];
        }
        if(swap_buffer) {
            swap_buffer = false;
            mutex_out->lock();
            emit bit_descramble(plp_id[n], l1_post, k_bch, buffer_a);
            signal_out->wait(mutex_out);
            mutex_out->unlock();
            ++n;
            out = buffer_b;
        }
        else {
            swap_buffer = true;
            mutex_out->lock();
            emit bit_descramble(plp_id[n], l1_post, k_bch, buffer_b);
            signal_out->wait(mutex_out);
            mutex_out->unlock();
            ++n;
            out = buffer_a;
        }
    }

     mutex_in->unlock();
}
//------------------------------------------------------------------------------------------
void bch_decoder::stop()
{
    emit finished();
}
//------------------------------------------------------------------------------------------

