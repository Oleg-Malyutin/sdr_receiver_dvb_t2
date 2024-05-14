/*
* libosmoplutosdr
* Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
* Copyright (C) 2017 by Hoernchen <la@tfc-server.de>
* plutosdr_hi_speed_rx
* Copyright (c) 2020 Oleg Malyutin <oleg.radioprograms@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef PLUTOSDR_HI_SPEED_RX_H
#define PLUTOSDR_HI_SPEED_RX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "plutosdr_hi_speed_rx_export.h"
#include <stdint.h>

typedef struct plutosdr_device plutosdr_device_t;

enum plutosdr_error
{
    PLUTOSDR_SUCCESS                = 0,
    PLUTOSDR_TRUE                   = 1,
    PLUTOSDR_ERROR_INVALID_PARAM    = -2,
    PLUTOSDR_ERROR_ACCESS           = -3,
    PLUTOSDR_ERROR_NO_DEVICE        = -4,
    PLUTOSDR_ERROR_NOT_FOUND        = -5,
    PLUTOSDR_ERROR_BUSY             = -6,
    PLUTOSDR_ERROR_NO_MEM           = -11,
    PLUTOSDR_ERROR_LIBUSB           = -99,
    PLUTOSDR_ERROR_THREAD           = -1001,
    PLUTOSDR_ERROR_STREAMING_THREAD_ERR = -1002,
    PLUTOSDR_ERROR_STREAMING_STOPPED    = -1003,
    PLUTOSDR_ERROR_OTHER            = -1000,
};

enum plutosdr_samples_type
{
    IQ = 0,
    REAL,
    RAW,
};

typedef struct
{
    unsigned char serial_number[2048];
    int serial_number_len;
    enum plutosdr_samples_type samples_type;
    uint32_t len_out;
} plutosdr_info_t;

typedef struct
{
    struct plutosdr_device* device;
    void* ctx;
    int16_t* i_samples;
    int16_t* q_samples;
    int sample_count;
} plutosdr_transfer_t, plutosdr_transfer;

typedef int (*plutosdr_sample_block_cb_fn)(plutosdr_transfer* transfer);

PLUTOSDR_API uint32_t plutosdr_get_device_count(void);
PLUTOSDR_API int plutosdr_reboot(plutosdr_device_t* device);
PLUTOSDR_API int plutosdr_buffer_channel_enable(plutosdr_device_t* device, uint32_t channel, uint32_t enable);
PLUTOSDR_API int plutosdr_bufstream_enable(plutosdr_device_t* dev, uint32_t enable);
PLUTOSDR_API int plutosdr_open(plutosdr_device_t** device, uint8_t idx_device, plutosdr_info_t* info);
PLUTOSDR_API int plutosdr_close(plutosdr_device_t* device);
PLUTOSDR_API int plutosdr_stop_rx(plutosdr_device_t* device);
PLUTOSDR_API int plutosdr_start_rx(plutosdr_device_t* device, plutosdr_sample_block_cb_fn cb, void* ctx);
PLUTOSDR_API int plutosdr_set_rfbw(plutosdr_device_t* device, uint32_t rfbw_hz);
PLUTOSDR_API int plutosdr_set_sample_rate(plutosdr_device_t* device, uint32_t sampfreq_hz);
PLUTOSDR_API int plutosdr_set_rxlo(plutosdr_device_t* device, uint64_t rf_frequency_hz);
PLUTOSDR_API int plutosdr_set_gainctl_manual(plutosdr_device_t* device);
PLUTOSDR_API int plutosdr_set_gain_mdb(plutosdr_device_t* device, uint32_t gain_in_millib);
PLUTOSDR_API void plutosdr_set_fir_coeff(plutosdr_device_t* device, char* buf);

#ifdef __cplusplus
}
#endif

#endif // PLUTOSDR_HI_SPEED_RX_H
