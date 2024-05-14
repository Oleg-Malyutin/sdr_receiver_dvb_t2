/*
* rtl_sdr tool for plutosdr
* Copyright (C) 2012 by Steve Markgraf <steve@steve-m.de>
* Copyright (C) 2017 by Hoernchen <la@tfc-server.de>
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

#ifndef _WIN32
#include <unistd.h>
#else
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include "getopt/getopt.h"
#endif

#include "osmoplutosdr.h"

#ifdef _WIN32
  #define atoll(x) _atoi64(x)
#endif

#define DEFAULT_SAMPLE_RATE		5000000
#define DEFAULT_BUF_LENGTH		(512 * 16 * 100)
#define MINIMAL_BUF_LENGTH		512
#define MAXIMAL_BUF_LENGTH		(512 * 16 * 2000)

static int do_exit = 0;
static int32_t bytes_to_read = 0;
static plutosdr_dev_t *dev = NULL;

void usage(void)
{
    fprintf(stderr,
        "rtl_sdr, an I/Q recorder for RTL2832 based DVB-T receivers\n\n"
        "Usage:\t -f frequency_to_tune_to [Hz]\n"
        "\t[-s samplerate (default: %d Hz)]\n"
        "\t[-d device_index (default: 0)]\n"
        "\t[-g gain in milli-dB (1/1000)]\n"
        "\t[-b output_block_size (default: %d)]\n"
        "\t[-n number of samples to read (default: 0, infinite)]\n"
        "\tfilename (a '-' dumps samples to stdout)\n\n", DEFAULT_SAMPLE_RATE, DEFAULT_BUF_LENGTH);
    exit(1);
}

#ifdef _WIN32
BOOL WINAPI
sighandler(int signum)
{
    if (CTRL_C_EVENT == signum) {
        fprintf(stderr, "Signal caught, exiting!\n");
        do_exit = 1;
        plutosdr_cancel_async(dev);
        return TRUE;
    }
    return FALSE;
}
#else
static void sighandler(int signum)
{
    fprintf(stderr, "Signal caught, exiting!\n");
    do_exit = 1;
    plutosdr_cancel_async(dev);
}
#endif

static void plutosdr_callback(unsigned char *buf, int32_t len, void *ctx)
{
    if (ctx) {
        if (do_exit)
            return;

        if ((bytes_to_read > 0) && (bytes_to_read < len)) {
            len = bytes_to_read;
            do_exit = 1;
            plutosdr_cancel_async(dev);
        }

        if (fwrite(buf, 1, len, (FILE*)ctx) != len) {
            fprintf(stderr, "Short write, samples lost, exiting!\n");
            plutosdr_cancel_async(dev);
        }

        if (bytes_to_read > 0)
            bytes_to_read -= len;
    }
}

int main_2(int argc, char **argv)
{
#ifndef _WIN32
    struct sigaction sigact;
#endif
    char *filename = NULL;
    int r, opt;
	int32_t gain = 10*1000;
    FILE *file;
    int dev_index = 0;
    int dev_given = 0;
    uint64_t frequency = 100000000;
    uint32_t samp_rate = DEFAULT_SAMPLE_RATE;
    uint32_t out_block_size = DEFAULT_BUF_LENGTH;

    while ((opt = getopt(argc, argv, "d:f:g:s:b:n:p:S")) != -1) {
        switch (opt) {
        case 'd':
            dev_index = atoi(optarg);
            dev_given = 1;
            break;
        case 'f':
            frequency = atoll(optarg);
            break;
        case 'g':
            gain = atoi(optarg);
            break;
        case 's':
            samp_rate = (uint32_t)atoi(optarg);
            break;
        case 'b':
            out_block_size = (uint32_t)atoi(optarg);
            break;
        case 'n':
            bytes_to_read = (uint32_t)atoi(optarg) * 4;
            break;
        default:
            usage();
            break;
        }
    }

    if (argc <= optind) {
        usage();
    } else {
        filename = argv[optind];
    }

    if(out_block_size < MINIMAL_BUF_LENGTH ||
       out_block_size > MAXIMAL_BUF_LENGTH ){
        fprintf(stderr,
            "Output block size wrong value, falling back to default\n");
        fprintf(stderr,
            "Minimal length: %u\n", MINIMAL_BUF_LENGTH);
        fprintf(stderr,
            "Maximal length: %u\n", MAXIMAL_BUF_LENGTH);
        out_block_size = DEFAULT_BUF_LENGTH;
    }


    r = plutosdr_open(&dev, (uint32_t)dev_index);
    if (r < 0) {
        fprintf(stderr, "Failed to open rtlsdr device #%d.\n", dev_index);
        exit(1);
    }
#ifndef _WIN32
    sigact.sa_handler = sighandler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;
    sigaction(SIGINT, &sigact, NULL);
    sigaction(SIGTERM, &sigact, NULL);
    sigaction(SIGQUIT, &sigact, NULL);
    sigaction(SIGPIPE, &sigact, NULL);
#else
    SetConsoleCtrlHandler( (PHANDLER_ROUTINE) sighandler, TRUE );
#endif

    plutosdr_set_rfbw(dev, samp_rate);
    plutosdr_set_sample_rate(dev, samp_rate);
    plutosdr_set_rxlo(dev, frequency);
    plutosdr_set_gainctl_manual(dev);
    plutosdr_set_gain_mdb(dev, gain);


    if(strcmp(filename, "-") == 0) { /* Write samples to stdout */
        file = stdout;
#ifdef _WIN32
        _setmode(_fileno(stdin), _O_BINARY);
#endif
    } else {
        file = fopen(filename, "wb");
        if (!file) {
            fprintf(stderr, "Failed to open %s\n", filename);
            goto out;
        }
    }

    plutosdr_bufstream_enable(dev, 1);

    fprintf(stderr, "Reading samples in async mode...\n");
    r = plutosdr_read_async(dev, plutosdr_callback, (void *)file,
              0, out_block_size);


    if (do_exit)
        fprintf(stderr, "\nUser cancel, exiting...\n");
    else
        fprintf(stderr, "\nLibrary error %d, exiting...\n", r);

    if (file != stdout)
        fclose(file);

    plutosdr_close(dev);
out:
    return r >= 0 ? r : -r;
}
