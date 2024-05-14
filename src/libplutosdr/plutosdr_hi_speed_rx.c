/*
* lib for plutosdr
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


#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(_WIN32) || defined(__CYGWIN__)
#include "libusb/libusb.h"
#else
#include <libusb-1.0/libusb.h>
#endif

#include <pthread.h>

#include "plutosdr_hi_speed_rx.h"

#ifndef LIBUSB_CALL
#define LIBUSB_CALL
#endif

#ifndef bool
typedef int bool;
#define true 1
#define false 0
#endif

#define PLUTO_USB_ITF_IDX 3
#define PLUTO_USB_READ_EPNUM 0x84
#define DEFAULT_BUF_NUMBER 8
#define DEFAULT_BUF_LENGTH	(262144 / 4) // 262144 // 512 * 6 * 512
#define CTRL_TIMEOUT 300
#define BULK_TIMEOUT 0
#define PLUTO_CTL_OUT (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_OUT | LIBUSB_RECIPIENT_INTERFACE)
#define PLUTO_CTL_IN (LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_ENDPOINT_IN | LIBUSB_RECIPIENT_INTERFACE)
//linux doesn't like the windows limit of 4096
#define PLUTO_CTLT_SIZE 2048

static void free_transfers(plutosdr_device_t *device);

//-----------------------------------------------------------------------------------------------------------------
enum plutosdr_ctl_commands {
    ENABLE_BUFSTREAM = 0,
    GLOBALPHYSTORE,
    RXLO,
    HWGAIN,         // milli-db!
    SAMPFREQ,
    GAINCTLMODE,    // always manual.
    SETFIRCOEFF,
    BUFFER_CHANNEL,
    REBOOT = 0xfe
};
//-----------------------------------------------------------------------------------------------------------------
typedef struct plutosdr_device {
    struct libusb_context* usb_context;
    struct libusb_device_handle* usb_device_handle;
    struct libusb_transfer** transfers;
    uint32_t transfer_count;
    uint8_t* usb_transfer_buffer[DEFAULT_BUF_NUMBER];
    uint32_t usb_transfer_buffer_size;
    plutosdr_sample_block_cb_fn callback;
    volatile bool streaming;
    volatile bool stop_requested;
    pthread_t transfer_thread;
    pthread_t consumer_thread;
    pthread_cond_t consumer_cv;
    pthread_mutex_t consumer_mp;

    int16_t* i_samples;
    int16_t* q_samples;
    uint32_t samples_count;
    volatile int head;
    volatile int tail;
    volatile int received_buffer_count;
    void* ctx;
    //buf for ctrl transfers
    char ctrl_buffer[PLUTO_CTLT_SIZE];
    unsigned int bb_rate_cache;
//    int test;
} device_t;
//-----------------------------------------------------------------------------------------------------------------
/*
static int16_t fir_128_4[] = {
    -15,   -27,   -23,   -6,   17,   33,   31,   9,   -23,   -47,   -45,   -13,   34,   69,   67,   21,
    -49,  -102,   -99,  -32,   69,  146,  143,  48,   -96,  -204,  -200,   -69,  129,  278,  275,   97,
    -170, -372,  -371, -135,  222,  494,  497, 187,  -288,  -654,  -665,  -258,  376,  875,  902,  363,
    -500,-1201, -1265, -530,  699, 1748, 1906, 845, -1089, -2922, -3424, -1697, 2326, 7714,12821,15921,
    15921,12821, 7714, 2326,-1697,-3424,-2922,-1089,  845,  1906,  1748,   699, -530,-1265,-1201, -500,
    363,   902,   875,  376, -258, -665, -654,-288,   187,   497,   494,   222, -135, -371, -372, -170,
    97,    275,   278,  129,  -69, -200, -204, -96,    48,   143,   146,    69,  -32,  -99, -102,  -49,
    21, 	67,    69,   34,  -13,  -45,  -47, -23,     9,    31,    33,    17,   -6,  -23,  -27,  -15 };

char firstring[] = "RX 3 GAIN -6 DEC 4\nTX 3 GAIN 0 INT 4\n-15,-15\n-27,-27\n-23,-23\n-6,-6\n17,"
                   "17\n33,33\n31,31\n9,9\n-23,-23\n-47,-47\n-45,-45\n-13,-13\n34,34\n69,69\n67,"
                   "67\n21,21\n-49,-49\n-102,-102\n-99,-99\n-32,-32\n69,69\n146,146\n143,143\n48,"
                   "48\n-96,-96\n-204,-204\n-200,-200\n-69,-69\n129,129\n278,278\n275,275\n97,"
                   "97\n-170,-170\n-372,-372\n-371,-371\n-135,-135\n222,222\n494,494\n497,497\n187,"
                   "187\n-288,-288\n-654,-654\n-665,-665\n-258,-258\n376,376\n875,875\n902,902\n363,"
                   "363\n-500,-500\n-1201,-1201\n-1265,-1265\n-530,-530\n699,699\n1748,1748\n1906,"
                   "1906\n845,845\n-1089,-1089\n-2922,-2922\n-3424,-3424\n-1697,-1697\n2326,"
                   "2326\n7714,7714\n12821,12821\n15921,15921\n15921,15921\n12821,12821\n7714,"
                   "7714\n2326,2326\n-1697,-1697\n-3424,-3424\n-2922,-2922\n-1089,-1089\n845,"
                   "845\n1906,1906\n1748,1748\n699,699\n-530,-530\n-1265,-1265\n-1201,-1201\n-500,"
                   "-500\n363,363\n902,902\n875,875\n376,376\n-258,-258\n-665,-665\n-654,-654\n-288,"
                   "-288\n187,187\n497,497\n494,494\n222,222\n-135,-135\n-371,-371\n-372,-372\n-170,"
                   "-170\n97,97\n275,275\n278,278\n129,129\n-69,-69\n-200,-200\n-204,-204\n-96,"
                   "-96\n48,48\n143,143\n146,146\n69,69\n-32,-32\n-99,-99\n-102,-102\n-49,-49\n21,"
                   "21\n67,67\n69,69\n34,34\n-13,-13\n-45,-45\n-47,-47\n-23,-23\n9,9\n31,31\n33,"
                   "33\n17,17\n-6,-6\n-23,-23\n-27,-27\n-15,-15\n\n";
*/
//-----------------------------------------------------------------------------------------------------------------
uint32_t plutosdr_get_device_count(void)
{
	int i, r;
	libusb_context *ctx;
	libusb_device **list;
	uint32_t device_count = 0;
    struct libusb_device_descriptor device_descriptor;
	ssize_t cnt;
	r = libusb_init(&ctx);

    if (r < 0) return 0;

	cnt = libusb_get_device_list(ctx, &list);
    for (i = 0; i < cnt; ++i) {
        libusb_get_device_descriptor(list[i], &device_descriptor);
        if (0x0456 == device_descriptor.idVendor && 0xB673 == device_descriptor.idProduct) device_count++;
	}
	libusb_free_device_list(list, 1);
	libusb_exit(ctx);
	return device_count;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_open(plutosdr_device_t** device, uint8_t idx_device, plutosdr_info_t* info)
{
    int err;
    libusb_device** device_list;
    libusb_device* device_found = NULL;
    struct libusb_device_descriptor found_device_descriptor;
    uint8_t device_count = 0;
    ssize_t count;
    plutosdr_device_t* open_device = NULL;
    open_device = (plutosdr_device_t*)calloc(1, sizeof(plutosdr_device_t));

    if (open_device == NULL) return PLUTOSDR_ERROR_NO_MEM;

    err = libusb_init(&open_device->usb_context);
    if(err < 0) {
        free(open_device);
        fprintf(stderr, "libusb_init %d\n", err);

        return PLUTOSDR_ERROR_LIBUSB;

    }
    count = libusb_get_device_list(open_device->usb_context, &device_list);
    for (int i = 0; i < count; ++i) {
        device_found = device_list[i];
        libusb_get_device_descriptor(device_list[i], &found_device_descriptor);
        if (0x0456 == found_device_descriptor.idVendor && 0xB673 == found_device_descriptor.idProduct){

            if (idx_device == device_count) break;

            device_count++;
        }
        device_found = NULL;
    }
    libusb_free_device_list(device_list, 1);

    if (!device_found) return PLUTOSDR_ERROR_NOT_FOUND;

    err = libusb_open(device_found, &open_device->usb_device_handle);
    if (err < 0) {
        fprintf(stderr, "libusb_open plutosdr %d\n", err);

        // return err;

    }
    err = libusb_kernel_driver_active(open_device->usb_device_handle, PLUTO_USB_ITF_IDX);
    if (err == 1) {
        err = libusb_detach_kernel_driver(open_device->usb_device_handle, PLUTO_USB_ITF_IDX);

        if (err != 0) return err;

    }
    else if(err != 0) {

        return err;

    }
    err = libusb_claim_interface(open_device->usb_device_handle, PLUTO_USB_ITF_IDX);

    if (err < 0) return err;

    int serial_descriptor_index;
    serial_descriptor_index = found_device_descriptor.iSerialNumber;
    fprintf(stderr, "serial_descriptor_index %d\n", serial_descriptor_index);

    info->serial_number_len = libusb_get_string_descriptor_ascii(open_device->usb_device_handle,
                                                           serial_descriptor_index,
                                                           info->serial_number,
                                                           2048);
    fprintf(stderr, "serial_number %s\n", info->serial_number);

    open_device->transfers = NULL;
    open_device->callback = NULL;
    open_device->transfer_count = DEFAULT_BUF_NUMBER;
    open_device->usb_transfer_buffer_size = DEFAULT_BUF_LENGTH;
    switch(info->samples_type){
    case IQ:
       open_device->samples_count = DEFAULT_BUF_LENGTH / 4;
        break;
    case REAL:
        open_device->samples_count = DEFAULT_BUF_LENGTH / 2;
        break;
    case RAW:
        open_device->samples_count = DEFAULT_BUF_LENGTH;
        break;
    }
    info->len_out = open_device->samples_count;
    open_device->streaming = false;
    open_device->stop_requested = false;
    *device = open_device;

    return PLUTOSDR_SUCCESS;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_close(plutosdr_device_t* device)
{
    if (!device) return PLUTOSDR_ERROR_NO_DEVICE;

    int err;
    err = plutosdr_stop_rx(device);
    pthread_cond_destroy(&device->consumer_cv);
    pthread_mutex_destroy(&device->consumer_mp);
    libusb_release_interface(device->usb_device_handle, PLUTO_USB_ITF_IDX);
    err = libusb_kernel_driver_active(device->usb_device_handle, PLUTO_USB_ITF_IDX);
    if (err == 1) err = libusb_detach_kernel_driver(device->usb_device_handle, PLUTO_USB_ITF_IDX);
    libusb_close(device->usb_device_handle);
    device->usb_device_handle = NULL;
    libusb_exit(device->usb_context);
    device->usb_context = NULL;
    free_transfers(device);
    free(device);

    return err;
}
//-----------------------------------------------------------------------------------------------------------------
static void* transfer_thread_proc(void* arg)
{
    plutosdr_device_t* device = (plutosdr_device_t*)arg;
    int error;
    struct timeval timeout = {0, 500000};;
#ifdef _WIN32

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

#endif
    while (device->streaming && !device->stop_requested) {
        error = libusb_handle_events_timeout_completed(device->usb_context, &timeout, NULL);

        if (error < 0) {
            fprintf(stderr, "transfer_thread_proc libusb_handle_events_timeout_completed: %d\n", error);

            if (error != LIBUSB_ERROR_INTERRUPTED) device->streaming = false;

        }
    }
    pthread_exit(NULL);

    return NULL;
}
//-----------------------------------------------------------------------------------------------------------------
static void* consumer_thread_proc(void* arg)
{
    plutosdr_device_t* device = (plutosdr_device_t*)arg;
    plutosdr_transfer_t transfer;
    int16_t* ptr_iq_samples;
    uint32_t i;
    transfer.device = device;
    transfer.ctx = device->ctx;
    transfer.sample_count = device->samples_count;
#ifdef _WIN32

    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

#endif
    pthread_mutex_lock(&device->consumer_mp);
    while (device->streaming && !device->stop_requested) {
        while (device->received_buffer_count == 0 && device->streaming && !device->stop_requested) {

            pthread_cond_wait(&device->consumer_cv, &device->consumer_mp);

        }    
        ptr_iq_samples = (int16_t*)device->usb_transfer_buffer[device->tail];
        device->tail = (device->tail + 1) & (DEFAULT_BUF_NUMBER - 1);

        pthread_mutex_unlock(&device->consumer_mp);

        for(i = 0; i < device->samples_count; ++i) {
            device->i_samples[i] = *ptr_iq_samples++;
            device->q_samples[i] = *ptr_iq_samples++;
        }
        transfer.i_samples  = device->i_samples;
        transfer.q_samples  = device->q_samples;

        if (device->callback(&transfer) != 0) device->stop_requested = true;

        pthread_mutex_lock(&device->consumer_mp);

        device->received_buffer_count--;
    }
    pthread_mutex_unlock(&device->consumer_mp);
    pthread_exit(NULL);
    return NULL;
}

//-----------------------------------------------------------------------------------------------------------------
static void libusb_transfer_callback(struct libusb_transfer* usb_transfer)
{
    uint8_t *temp;
    plutosdr_device_t *device = (plutosdr_device_t*)usb_transfer->user_data;

    if (!device->streaming || device->stop_requested) return;

    if (usb_transfer->status == LIBUSB_TRANSFER_COMPLETED && usb_transfer->actual_length == usb_transfer->length) {

        pthread_mutex_lock(&device->consumer_mp);

        temp = device->usb_transfer_buffer[device->head];
        device->usb_transfer_buffer[device->head] = usb_transfer->buffer;
        usb_transfer->buffer = temp;
        device->head = (device->head + 1) & (DEFAULT_BUF_NUMBER - 1);
        device->received_buffer_count++;
        if(device->received_buffer_count == DEFAULT_BUF_NUMBER) {
            device->received_buffer_count--;
            fprintf(stderr, "dropped buffer\n");
        }

        pthread_mutex_unlock(&device->consumer_mp);
        pthread_cond_signal(&device->consumer_cv);

        int err = libusb_submit_transfer(usb_transfer);

        if (err != 0) device->streaming = false;

    }
    else {

        device->streaming = false;

    }
}
//-----------------------------------------------------------------------------------------------------------------
static int cancel_transfers(plutosdr_device_t* device)
{
    int i;
    if (device->transfers != NULL) {
        for (i = 0; i < DEFAULT_BUF_NUMBER; ++i) {
            if (device->transfers[i] != NULL) {
                libusb_cancel_transfer(device->transfers[i]);
            }
        }

        return PLUTOSDR_SUCCESS;
    }
    else {

        return PLUTOSDR_ERROR_OTHER;

    }
}
//-----------------------------------------------------------------------------------------------------------------
static int allocate_transfers(plutosdr_device_t* const device)
{
    unsigned int i;
    int err;
    if (device->transfers == NULL) {
        device->i_samples = (int16_t*)calloc(device->samples_count,sizeof(int16_t));;
        if (device->i_samples == NULL) {

            return PLUTOSDR_ERROR_NO_MEM;

        }
        device->q_samples = (int16_t*)calloc(device->samples_count,sizeof(int16_t));
        if (device->q_samples == NULL) {

            return PLUTOSDR_ERROR_NO_MEM;

        }
        device->transfers = (struct libusb_transfer**) calloc(device->transfer_count,
                             sizeof(struct libusb_transfer));

        if (device->transfers == NULL) return PLUTOSDR_ERROR_NO_MEM;

        for (i = 0; i < DEFAULT_BUF_NUMBER; ++i) {
            device->transfers[i] = libusb_alloc_transfer(0);

            if (device->transfers[i] == NULL) return PLUTOSDR_ERROR_LIBUSB;

            device->usb_transfer_buffer[i] = (uint8_t*)malloc(device->usb_transfer_buffer_size);

            if (device->usb_transfer_buffer[i] == NULL) return PLUTOSDR_ERROR_NO_MEM;

            memset(device->usb_transfer_buffer[i], 0, device->usb_transfer_buffer_size);

            libusb_fill_bulk_transfer(device->transfers[i],
                                      device->usb_device_handle,
                                      PLUTO_USB_READ_EPNUM,
                                      device->usb_transfer_buffer[i],
                                      device->usb_transfer_buffer_size,
                                      libusb_transfer_callback,
                                      (void*)device,
                                      BULK_TIMEOUT
                                      );

            if (device->transfers[i]->buffer == NULL) return PLUTOSDR_ERROR_NO_MEM;

            err = libusb_submit_transfer(device->transfers[i]);

            if (err != 0) return err;

        }

        return PLUTOSDR_SUCCESS;

    }
    else {

        return PLUTOSDR_ERROR_BUSY;

    }
}
//-----------------------------------------------------------------------------------------------------------------
static void free_transfers(plutosdr_device_t* device)
{
    int i;
    if (device->transfers != NULL) {
        for (i = 0; i < DEFAULT_BUF_NUMBER; ++i) {
            if (device->transfers[i] != NULL) {
                free(device->transfers[i]->buffer);
                libusb_free_transfer(device->transfers[i]);
                device->transfers[i] = NULL;
            }

        }
        if (device->i_samples != NULL) {
            free(device->i_samples);
            device->i_samples = NULL;
        }
        if (device->q_samples != NULL) {
            free(device->q_samples);
            device->q_samples = NULL;
        }
        free(device->transfers);
    }
}
//-----------------------------------------------------------------------------------------------------------------
static int create_io_threads(plutosdr_device_t* device, plutosdr_sample_block_cb_fn callback)
{
    int err;
    pthread_attr_t attr;
    if (!device->streaming && !device->stop_requested) {
        device->callback = callback;
        device->streaming = true;
        device->head = 0;
        device->tail = 0;
        device->received_buffer_count = 0;
        err = allocate_transfers(device);

        if (err != PLUTOSDR_SUCCESS) return err;

        pthread_cond_init(&device->consumer_cv, NULL);
        pthread_mutex_init(&device->consumer_mp, NULL);

        if (err != PLUTOSDR_SUCCESS) return err;

        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        err = pthread_create(&device->consumer_thread, &attr, consumer_thread_proc, device);

        if (err != 0) return PLUTOSDR_ERROR_THREAD;

        err = pthread_create(&device->transfer_thread, &attr, transfer_thread_proc, device);

        if (err != 0) return PLUTOSDR_ERROR_THREAD;

        pthread_attr_destroy(&attr);
    }
    else {

        return PLUTOSDR_ERROR_BUSY;
    }

    return PLUTOSDR_SUCCESS;
}
//-----------------------------------------------------------------------------------------------------------------
static int kill_io_threads(plutosdr_device_t* device)
{
    if (device->streaming) {
        device->stop_requested = true;
        cancel_transfers(device);
        pthread_mutex_lock(&device->consumer_mp);
        pthread_cond_signal(&device->consumer_cv);
        pthread_mutex_unlock(&device->consumer_mp);
        pthread_join(device->transfer_thread, NULL);
        pthread_join(device->consumer_thread, NULL);
        device->stop_requested = false;
        device->streaming = false;
    }

    return PLUTOSDR_SUCCESS;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_reboot(plutosdr_device_t *device)
{
    int ret;
    device->ctrl_buffer[0] = '0';
    device->ctrl_buffer[1] = 0;
    unsigned char* ctrl_buffer = (unsigned char*)device->ctrl_buffer;
    ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, REBOOT, 0x1234, ctrl_buffer, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr,"ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine0",  device->ctrl_buffer);
    return ret;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_buffer_channel_enable(plutosdr_device_t* device, uint32_t channel, uint32_t enable)
{
    int ret;
    unsigned char* ctrl_buffer = (unsigned char*)device->ctrl_buffer;
    switch (channel) {
    case 0:
        if(enable) snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=1", "in_voltage0_en");
        else snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=0", "in_voltage0_en");
        break;
    case 1:
        if(enable) snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=1", "in_voltage1_en");
        else snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=0", "in_voltage1_en");
        break;
    }
    ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, BUFFER_CHANNEL, 0x1234, ctrl_buffer, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr, "ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine", device->ctrl_buffer);
    return ret;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_bufstream_enable(plutosdr_device_t* dev, uint32_t enable)
{
    int ret;
    dev->ctrl_buffer[0] = enable ? '1' : '0';
    dev->ctrl_buffer[1] = 0;
    unsigned char* ctrl_buffer = (unsigned char*)dev->ctrl_buffer;
    ret = libusb_control_transfer(dev->usb_device_handle, PLUTO_CTL_OUT, 0xff, ENABLE_BUFSTREAM, 0x1234, ctrl_buffer, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr,"ENABLE_BUFSTREAM ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine1",  dev->ctrl_buffer);
    if(ret < 0) return ret;
    snprintf(dev->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=0", "in_voltage_bb_dc_offset_tracking_en");
    ret = libusb_control_transfer(dev->usb_device_handle, PLUTO_CTL_OUT, 0xff, GLOBALPHYSTORE, 0x1234, ctrl_buffer, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr, "ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine2", dev->ctrl_buffer);
    if(ret < 0) return ret;
    snprintf(dev->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=0", "in_voltage_rf_dc_offset_tracking_en");
    ret = libusb_control_transfer(dev->usb_device_handle, PLUTO_CTL_OUT, 0xff, GLOBALPHYSTORE, 0x1234, ctrl_buffer, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr, "ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine3", dev->ctrl_buffer);
    if(ret < 0) return ret;
    snprintf(dev->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=0", "in_voltage_quadrature_tracking_en");
    ret = libusb_control_transfer(dev->usb_device_handle, PLUTO_CTL_OUT, 0xff, GLOBALPHYSTORE, 0x1234, ctrl_buffer, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr, "ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine4", dev->ctrl_buffer);
    if(ret < 0) return ret;
//    snprintf(dev->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=0", "out_altvoltage1_TX_LO_powerdown");
//    ret = libusb_control_transfer(dev->usb_device_handle, PLUTO_CTL_OUT, 0xff, GLOBALPHYSTORE, 0x1234, ctrl_buffer, PLUTO_CTLT_SIZE, 0);
//    fprintf(stderr, "ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine", dev->ctrl_buffer);
    return ret;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_start_rx(plutosdr_device_t* device, plutosdr_sample_block_cb_fn callback, void* ctx)
{
    int ret;
    device->ctx = ctx;
    libusb_clear_halt(device->usb_device_handle, LIBUSB_ENDPOINT_IN | 4);
    ret = create_io_threads(device, callback);
    return ret;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_stop_rx(plutosdr_device_t* device)
{
    int err1, err2;
    err1 = kill_io_threads(device);
    err2 = plutosdr_bufstream_enable(device, 0);

    if (err2 != PLUTOSDR_SUCCESS) return err2;

    return err1;
}
//-----------------------------------------------------------------------------------------------------------------
int ad9361_set_bb_rate(plutosdr_device_t* device, uint32_t rate)
{
    int ret;
    uint8_t *buf = (uint8_t*)device->ctrl_buffer;
    snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=0", "in_out_voltage_filter_fir_en");
    ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, GLOBALPHYSTORE, 0x1234, buf, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr, "ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine", device->ctrl_buffer);

    //int i,  len = 0;
	//memset(buf, 0, PLUTO_CTLTSZ);
	//len += snprintf(buf + len, PLUTO_CTLTSZ - len, "RX 3 GAIN -6 DEC %d\n", 4);
	//len += snprintf(buf + len, PLUTO_CTLTSZ - len, "TX 3 GAIN 0 INT %d\n", 4);
    //for (i = 0; i < 128; i++) len += snprintf(buf + len, PLUTO_CTLTSZ - len, "%d,%d\n", fir_128_4[i], fir_128_4[i]);
	//len += snprintf(buf + len, PLUTO_CTLTSZ - len, "\n");
	//fprintf(stderr, "fir len: %d:%s\n", len, buf);

//    plutosdr_set_fir_coeff(device, firstring);
//	if (rate <= (25000000 / 12)) {
//        snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=1", "in_out_voltage_filter_fir_en");
//        ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, GLOBALPHYSTORE, 0x1234, buf, PLUTO_CTLT_SIZE, 0);
//        fprintf(stderr, "ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine", device->ctrl_buffer);
//        snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%u", rate);
//        ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, SAMPFREQ, 0x1234, buf, PLUTO_CTLT_SIZE, 0);
//        fprintf(stderr, "ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine", device->ctrl_buffer);
//	}
//	else {
//        snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%u", rate);
//        ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, SAMPFREQ, 0x1234, buf, PLUTO_CTLT_SIZE, 0);
//        fprintf(stderr, "ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine", device->ctrl_buffer);
//        snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=1", "in_out_voltage_filter_fir_en");
//        ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, GLOBALPHYSTORE, 0x1234, buf, PLUTO_CTLT_SIZE, 0);
//        fprintf(stderr, "ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine", device->ctrl_buffer);
//        return ret;
//	}

    return ret;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_set_rfbw(plutosdr_device_t* device, uint32_t rfbw_hz)
{
    int ret;
    uint8_t *buf = (uint8_t*)device->ctrl_buffer;
    if (rfbw_hz == 0) rfbw_hz = device->bb_rate_cache;
    snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=%d", "in_voltage_rf_bandwidth", rfbw_hz);
    ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, GLOBALPHYSTORE, 0x1234, buf, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr,"plutosdr_set_rfb ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine",  device->ctrl_buffer);
    return ret;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_set_sample_rate(plutosdr_device_t* dev, uint32_t sampfreq_hz)
{
    int ret;
    uint8_t *buf = (uint8_t*)dev->ctrl_buffer;
    snprintf(dev->ctrl_buffer, PLUTO_CTLT_SIZE, "%u", sampfreq_hz);
    ret = libusb_control_transfer(dev->usb_device_handle, PLUTO_CTL_OUT, 0xff, SAMPFREQ, 0x1234, buf, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr,"ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine",  dev->ctrl_buffer);
    dev->bb_rate_cache = sampfreq_hz;
//	ad9361_set_bb_rate(dev, sampfreq_hz);
    return ret;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_set_rxlo(plutosdr_device_t* device, uint64_t rf_frequency_hz)
{
    int ret;
    uint8_t *buf = (uint8_t*)device->ctrl_buffer;
    //RX_LO
    snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=%lu", "frequency", rf_frequency_hz);
    ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, RXLO, 0x1234, buf, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr,"plutosdr_set_rxlo ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine",  device->ctrl_buffer);
    return ret;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_set_gainctl_manual(plutosdr_device_t* device)
{
    int ret;
    uint8_t *buf = (uint8_t*)device->ctrl_buffer;
    //gain control mode
    snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%s=%s", "gain_control_mode", "manual");
    ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, GAINCTLMODE, 0x1234, buf, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr,"ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine",  device->ctrl_buffer);
    return ret;
}
//-----------------------------------------------------------------------------------------------------------------
int plutosdr_set_gain_mdb(plutosdr_device_t* device, uint32_t gain_in_millib)
{
    int ret;
    uint8_t *buf = (uint8_t*)device->ctrl_buffer;
    //hw gain in milli-dB
    snprintf(device->ctrl_buffer, PLUTO_CTLT_SIZE, "%u", gain_in_millib);
    ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, HWGAIN, 0x1234, buf, PLUTO_CTLT_SIZE, 0);
    fprintf(stderr,"ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine",  device->ctrl_buffer);
    return ret;
}
//-----------------------------------------------------------------------------------------------------------------
void plutosdr_set_fir_coeff(plutosdr_device_t* device, char* buf)
{
	int ret;
    uint8_t *p_ctrl_buffer = (uint8_t*)device->ctrl_buffer;
    if (strlen(buf) + 1 > PLUTO_CTLT_SIZE) {
		fprintf(stderr, "err: fir coeff too long!");
		return;
	}
	//dev->ctrlbuf[PLUTO_CTLTSZ - 1] = 0;
    strncpy(device->ctrl_buffer, buf, PLUTO_CTLT_SIZE - 1);
    ret = libusb_control_transfer(device->usb_device_handle, PLUTO_CTL_OUT, 0xff, SETFIRCOEFF, 0x1234, p_ctrl_buffer, PLUTO_CTLT_SIZE, 0);
	fprintf(stderr, "ctl %d %s req was:%s\n", ret, ret < 0 ? libusb_strerror(ret) : "fine", "fir filter coeff");
}
//-----------------------------------------------------------------------------------------------------------------
