/* -*- c++ -*- */
/* 
 * Copyright 2018,2019 Ahmet Inan, Ron Economos.
 * 
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 * 
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef LAYERED_DECODER_HH
#define LAYERED_DECODER_HH

#include <stdlib.h>
#include <stdint.h>
#include <new>
#include "ldpc.hh"

//#if __cplusplus >= 201703L && defined _MSVC_LANG
//#include <malloc.h>

//#undef  free
//#define free  aligned_free

//extern "C"
//{
//static inline __attribute__((__always_inline__))
//void *aligned_alloc(size_t align, size_t wanted){return _aligned_malloc( wanted, align);}

//static inline __attribute__((__always_inline__))
//void aligned_free(void *ptr){return _aligned_free( ptr );}
//}

//namespace std
//{ using ::aligned_alloc;
//  using ::aligned_free;
//}
//#endif  // C++17 && __MINGW32__

template <typename TYPE, typename ALG>
class LDPCDecoder
{
  TYPE *bnl, *pty;
  uint16_t *pos;
  uint8_t *cnc;
  ALG alg;
  int M, N, K, R, q, CNL, LT;
  bool initialized;

  void reset()
  {
    for (int i = 0; i < LT; ++i)
      bnl[i] = alg.zero();
  }
  bool bad(TYPE *data, TYPE *parity, int blocks)
  {
    for (int i = 0; i < q; ++i) {
      int cnt = cnc[i];
      for (int j = 0; j < M; ++j) {
        TYPE cnv = alg.sign(alg.one(), parity[M*i+j]);
        if (i)
          cnv = alg.sign(cnv, parity[M*(i-1)+j]);
        else if (j)
          cnv = alg.sign(cnv, parity[j+(q-1)*M-1]);
        for (int c = 0; c < cnt; ++c)
          cnv = alg.sign(cnv, data[pos[CNL*(M*i+j)+c]]);
        if (alg.bad(cnv, blocks))
          return true;
      }
    }
    return false;
  }
  void update(TYPE *data, TYPE *parity)
  {
    TYPE *bl = bnl;
    for (int i = 0; i < q; ++i) {
      int cnt = cnc[i];
      for (int j = 0; j < M; ++j) {
        int deg = cnt + 2 - !(i|j);
        TYPE inp[deg], out[deg];
        for (int c = 0; c < cnt; ++c)
          inp[c] = out[c] = alg.sub(data[pos[CNL*(M*i+j)+c]], bl[c]);
        inp[cnt] = out[cnt] = alg.sub(parity[M*i+j], bl[cnt]);
        if (i)
          inp[cnt+1] = out[cnt+1] = alg.sub(parity[M*(i-1)+j], bl[cnt+1]);
        else if (j)
          inp[cnt+1] = out[cnt+1] = alg.sub(parity[j+(q-1)*M-1], bl[cnt+1]);
        alg.finalp(out, deg);
        for (int c = 0; c < cnt; ++c)
          data[pos[CNL*(M*i+j)+c]] = alg.add(inp[c], out[c]);
        parity[M*i+j] = alg.add(inp[cnt], out[cnt]);
        if (i)
          parity[M*(i-1)+j] = alg.add(inp[cnt+1], out[cnt+1]);
        else if (j)
          parity[j+(q-1)*M-1] = alg.add(inp[cnt+1], out[cnt+1]);
        for (int d = 0; d < deg; ++d)
          alg.update(bl++, out[d]);
      }
    }
  }
public:
  LDPCDecoder() : initialized(false)
  {
  }
  void init(LDPCInterface *it)
  {
    if (initialized) {
      free(bnl);
      free(pty);
      delete[] cnc;
      delete[] pos;
    }
    initialized = true;
    LDPCInterface *ldpc = it->clone();
    N = ldpc->code_len();
    K = ldpc->data_len();
    M = ldpc->group_len();
    R = N - K;
    q = R / M;
    CNL = ldpc->links_max_cn() - 2;

//    pos = new uint16_t[R * CNL];
//    cnc = new uint8_t[R];

    pos = new(std::align_val_t(32))uint16_t[R * CNL];
    cnc = new(std::align_val_t(32))uint8_t[R ];

    for (int i = 0; i < R; ++i)
      cnc[i] = 0;
    ldpc->first_bit();
    for (int j = 0; j < K; ++j) {
      int *acc_pos = ldpc->acc_pos();
      int bit_deg = ldpc->bit_deg();
      for (int n = 0; n < bit_deg; ++n) {
        int i = acc_pos[n];
        pos[CNL*i+cnc[i]++] = j;
      }
      ldpc->next_bit();
    }
    LT = ldpc->links_total();
    delete ldpc;
//    bnl = reinterpret_cast<TYPE *>(aligned_alloc(sizeof(TYPE), sizeof(TYPE) * LT));
//    pty = reinterpret_cast<TYPE *>(aligned_alloc(sizeof(TYPE), sizeof(TYPE) * R));

    bnl = new(std::align_val_t(sizeof(TYPE))) TYPE[sizeof(TYPE) * LT];
    pty = new(std::align_val_t(sizeof(TYPE))) TYPE[sizeof(TYPE) * R];


    uint16_t *tmp = new uint16_t[R * CNL];
    for (int i = 0; i < q; ++i)
      for (int j = 0; j < M; ++j)
        for (int c = 0; c < CNL; ++c)
          tmp[CNL*(M*i+j)+c] = pos[CNL*(q*j+i)+c];
    // delete[] pos;
    operator delete[] (pos, std::align_val_t(32));
    pos = tmp;
  }
  int operator()(TYPE *data, TYPE *parity, int trials = 25, int blocks = 1)
  {
    reset();
    for (int i = 0; i < q; ++i)
      for (int j = 0; j < M; ++j)
        pty[M*i+j] = parity[q*j+i];
    while (bad(data, pty, blocks) && --trials >= 0)
      update(data, pty);
    for (int i = 0; i < q; ++i)
      for (int j = 0; j < M; ++j)
        parity[q*j+i] = pty[M*i+j];
    return trials;
  }
  ~LDPCDecoder()
  {
    if (initialized) {
      free(bnl);
      free(pty);
      delete[] cnc;
      delete[] pos;
    }
  }
};

#endif
