# sdr_receiver_dvb_t2
Author: Oleg Malyutin
Email: <oleg.radioprograms@gmail.com>

This is a Software Defined Radio(SDR) project implementing a DVB-T2 receiver.

The project was created using the Qt5 framework.
To build the project, you need to install
the library FFTW3: http://www.fftw.org

Supported devices:
1. SdrPlay (need to install API/HW driver – V2.*(but not V3.*) from https://www.sdrplay.com) 
2. AirSpy (need to install libusb-1.0 from https://libusb.info)
3. PlutoSDR (Only Linux and firmware 0.31. The program makes a temporary kernel patch for plutosdr-fw 0.31)


Received DVB-T2 signal parameters:
1. SISO and one RT-TX
2. 8MHz 16K or 32K only (32K tested)
3. Output Transport Stream(TS) (HEM mode tested)
Not supported:
1. FEF part
2. T2-Lite

Tested configurations:
OS: Linux Mint 20 Ulyana 64-bit. Windows 10 22H2
Processor: Intel© Core™ i5-8600 CPU @ 3.10GHz × 6

Used in the project Qt C++ widget QCustomPlot
(necessary files are included in the project):
https://www.qcustomplot.com/
The LDPC block is a port of the code in:
https://github.com/drmpeg/gr-dvbs2rx
The PLUTOSDR kernel patch has been ported and modified from:
https://github.com/Hoernchen/plutosdr-fw



To view the video signal, a VLC player v2.x (not v.3x) with 
udp://@:7654 parameters was used (URL)



If you find this project useful, consider donating:

[Donate via PayPal](https://www.paypal.com/donate?hosted_button_id=A4EMYB46V67WJ)

**Bitcoin**: `bc1qem3sk3gumc3u8h6dx0nmffy4272vj8sx9s750p`





