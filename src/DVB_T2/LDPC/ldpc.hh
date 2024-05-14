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

#ifndef LDPC_HH
#define LDPC_HH

struct LDPCInterface
{
  virtual LDPCInterface *clone() = 0;
  virtual int code_len() = 0;
  virtual int data_len() = 0;
  virtual int group_len() = 0;
  virtual int links_total() = 0;
  virtual int links_max_cn() = 0;
  virtual int bit_deg() = 0;
  virtual int *acc_pos() = 0;
  virtual void first_bit() = 0;
  virtual void next_bit() = 0;
  virtual ~LDPCInterface() = default;
};

template <typename TABLE>
class LDPC : public LDPCInterface
{
  static const int M = TABLE::M;
  static const int N = TABLE::N;
  static const int K = TABLE::K;
  static const int R = N-K;
  static const int q = R/M;

  int acc_pos_[TABLE::DEG_MAX];
  const int *row_ptr;
  int bit_deg_;
  int grp_num;
  int grp_len;
  int grp_cnt;
  int row_cnt;

  void next_group()
  {
    if (grp_cnt >= grp_len) {
      grp_len = TABLE::LEN[grp_num];
      bit_deg_ = TABLE::DEG[grp_num];
      grp_cnt = 0;
      ++grp_num;
    }
    for (int i = 0; i < bit_deg_; ++i)
      acc_pos_[i] = row_ptr[i];
    row_ptr += bit_deg_;
    ++grp_cnt;
  }
public:
  LDPCInterface *clone()
  {
    return new LDPC<TABLE>();
  }
  int code_len()
  {
    return N;
  }
  int data_len()
  {
    return K;
  }
  int group_len()
  {
    return M;
  }
  int links_total()
  {
    return TABLE::LINKS_TOTAL;
  }
  int links_max_cn()
  {
    return TABLE::LINKS_MAX_CN;
  }
  int bit_deg()
  {
    return bit_deg_;
  }
  int *acc_pos()
  {
    return acc_pos_;
  }
  void next_bit()
  {
    if (++row_cnt < M) {
      for (int i = 0; i < bit_deg_; ++i)
        acc_pos_[i] += q;
      for (int i = 0; i < bit_deg_; ++i)
        acc_pos_[i] %= R;
    } else {
      next_group();
      row_cnt = 0;
    }
  }
  void first_bit()
  {
    grp_num = 0;
    grp_len = 0;
    grp_cnt = 0;
    row_cnt = 0;
    row_ptr = TABLE::POS;
    next_group();
  }
};

#endif
