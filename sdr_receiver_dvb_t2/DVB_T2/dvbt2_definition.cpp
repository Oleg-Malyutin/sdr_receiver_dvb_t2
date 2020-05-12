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
#include "dvbt2_definition.h"

#include <QMetaType>

//-----------------------------------------------------------------------------------------------
void dvbt2_p2_parameters_init(dvbt2_parameters &_dvbt2)
{qRegisterMetaType<l1_postsignalling>();
    if ((_dvbt2.preamble == T2_SISO) || (_dvbt2.preamble == T2_LITE_SISO)){
        _dvbt2.miso = FALSE;
        switch (_dvbt2.fft_mode){
        case FFTSIZE_1K:
            _dvbt2.n_p2 = 16;
            _dvbt2.c_p2 = 558;
            break;
        case FFTSIZE_2K:
            _dvbt2.n_p2 = 8;
            _dvbt2.c_p2 = 1118;
            break;
        case FFTSIZE_4K:
            _dvbt2.n_p2 = 4;
            _dvbt2.c_p2 = 2236;
            break;
        case FFTSIZE_8K:
        case FFTSIZE_8K_T2GI:
            _dvbt2.n_p2 = 2;
            _dvbt2.c_p2 = 4472;
            break;
        case FFTSIZE_16K:
        case FFTSIZE_16K_T2GI:
            _dvbt2.n_p2 = 1;
            _dvbt2.c_p2 = 8944;
            break;
        case FFTSIZE_32K:
        case FFTSIZE_32K_T2GI:
            _dvbt2.n_p2 = 1;
            _dvbt2.c_p2 = 22432;
            break;
        }
    }
    else{
        _dvbt2.miso = TRUE;
        switch (_dvbt2.fft_mode){
        case FFTSIZE_1K:
            _dvbt2.n_p2 = 16;
            _dvbt2.c_p2 = 546;
            break;
        case FFTSIZE_2K:
            _dvbt2.n_p2 = 8;
            _dvbt2.c_p2 = 1098;
            break;
        case FFTSIZE_4K:
            _dvbt2.n_p2 = 4;
            _dvbt2.c_p2 = 2198;
            break;
        case FFTSIZE_8K:
        case FFTSIZE_8K_T2GI:
            _dvbt2.n_p2 = 2;
            _dvbt2.c_p2 = 4398;
            break;
        case FFTSIZE_16K:
        case FFTSIZE_16K_T2GI:
            _dvbt2.n_p2 = 1;
            _dvbt2.c_p2 = 8814;
            break;
        case FFTSIZE_32K:
        case FFTSIZE_32K_T2GI:
            _dvbt2.n_p2 = 1;
            _dvbt2.c_p2 = 17612;
            break;
        }
    }

    _dvbt2.carrier_mode = CARRIERS_EXTENDED;
    dvbt2_bwt_ext_parameters_init(_dvbt2);
}
//-----------------------------------------------------------------------------------------------
void dvbt2_bwt_ext_parameters_init(dvbt2_parameters &_dvbt2)
{
    switch (_dvbt2.fft_mode){
    case FFTSIZE_1K:
        _dvbt2.fft_size = 1024;
        _dvbt2.k_total = 853;
        _dvbt2.k_ext = 0;
        _dvbt2.k_offset = 0;
        break;
    case FFTSIZE_2K:
        _dvbt2.fft_size = 2048;
        _dvbt2.k_total = 1705;
        _dvbt2.k_ext = 0;
        _dvbt2.k_offset = 0;
        break;
    case FFTSIZE_4K:
        _dvbt2.fft_size = 4096;
        _dvbt2.k_total = 3409;
        _dvbt2.k_ext = 0;
        _dvbt2.k_offset = 0;
        break;
    case FFTSIZE_8K:
    case FFTSIZE_8K_T2GI:
        _dvbt2.fft_size = 8192;
        if (_dvbt2.carrier_mode == CARRIERS_NORMAL){
            _dvbt2.k_total = 6817;
            _dvbt2.k_ext = 0;
            _dvbt2.k_offset = 48;
        }
        else{
            _dvbt2.k_total = 6913;
            _dvbt2.k_ext = 48;
            _dvbt2.k_offset = 0;
        }
        break;
    case FFTSIZE_16K:
    case FFTSIZE_16K_T2GI:
        _dvbt2.fft_size = 16384;
        if (_dvbt2.carrier_mode == CARRIERS_NORMAL){
            _dvbt2.k_total = 13633;
            _dvbt2.k_ext = 0;
            _dvbt2.k_offset = 144;
        }
        else{
            _dvbt2.k_total = 13921;
            _dvbt2.k_ext = 144;
            _dvbt2.k_offset = 0;
        }
        break;
    case FFTSIZE_32K:
    case FFTSIZE_32K_T2GI:
        _dvbt2.fft_size = 32768;
        if (_dvbt2.carrier_mode == CARRIERS_NORMAL){
            _dvbt2.k_total = 27265;
            _dvbt2.k_ext = 0;
            _dvbt2.k_offset = 288;
        }
        else{
            _dvbt2.k_total = 27841;
            _dvbt2.k_ext = 288;
            _dvbt2.k_offset = 0;
        }
        break;
    }

    _dvbt2.l_nulls = ((_dvbt2.fft_size - _dvbt2.k_total) / 2) + 1;
}
//-----------------------------------------------------------------------------------------------
void dvbt2_data_parameters_init(dvbt2_parameters &_dvbt2)
{
    switch (_dvbt2.fft_mode){
    case FFTSIZE_1K:
        switch (_dvbt2.pilot_pattern){
        case PP1:
            _dvbt2.c_data = 764;
            _dvbt2.n_fc = 568;
            _dvbt2.c_fc = 402;
            break;
        case PP2:
            _dvbt2.c_data = 768;
            _dvbt2.n_fc = 710;
            _dvbt2.c_fc = 654;
            break;
        case PP3:
            _dvbt2.c_data = 798;
            _dvbt2.n_fc = 710;
            _dvbt2.c_fc = 490;
            break;
        case PP4:
            _dvbt2.c_data = 804;
            _dvbt2.n_fc = 780;
            _dvbt2.c_fc = 707;
            break;
        case PP5:
            _dvbt2.c_data = 818;
            _dvbt2.n_fc = 780;
            _dvbt2.c_fc = 544;
            break;
        case PP6:
            _dvbt2.c_data = 0;
            _dvbt2.n_fc = 0;
            _dvbt2.c_fc = 0;
            break;
        case PP7:
            _dvbt2.c_data = 0;
            _dvbt2.n_fc = 0;
            _dvbt2.c_fc = 0;
            break;
        case PP8:
            _dvbt2.c_data = 0;
            _dvbt2.n_fc = 0;
            _dvbt2.c_fc = 0;
            break;
        }
        if (_dvbt2.papr_mode == PAPR_TR || _dvbt2.papr_mode == PAPR_BOTH){
            if (_dvbt2.c_data != 0) _dvbt2.c_data -= 10;
            if (_dvbt2.n_fc != 0) _dvbt2.n_fc -= 10;
            if (_dvbt2.c_fc != 0) _dvbt2.c_fc -= 10;
        }
        break;
    case FFTSIZE_2K:
        switch (_dvbt2.pilot_pattern)
        {
        case PP1:
            _dvbt2.c_data = 1522;
            _dvbt2.n_fc = 1136;
            _dvbt2.c_fc = 804;
            break;
        case PP2:
            _dvbt2.c_data = 1532;
            _dvbt2.n_fc = 1420;
            _dvbt2.c_fc = 1309;
            break;
        case PP3:
            _dvbt2.c_data = 1596;
            _dvbt2.n_fc = 1420;
            _dvbt2.c_fc = 980;
            break;
        case PP4:
            _dvbt2.c_data = 1602;
            _dvbt2.n_fc = 1562;
            _dvbt2.c_fc = 1415;
            break;
        case PP5:
            _dvbt2.c_data = 1632;
            _dvbt2.n_fc = 1562;
            _dvbt2.c_fc = 1088;
            break;
        case PP6:
            _dvbt2.c_data = 0;
            _dvbt2.n_fc = 0;
            _dvbt2.c_fc = 0;
            break;
        case PP7:
            _dvbt2.c_data = 1646;
            _dvbt2.n_fc = 1632;
            _dvbt2.c_fc = 1396;
            break;
        case PP8:
            _dvbt2.c_data = 0;
            _dvbt2.n_fc = 0;
            _dvbt2.c_fc = 0;
            break;
        }
        if (_dvbt2.papr_mode == PAPR_TR || _dvbt2.papr_mode == PAPR_BOTH){
            if (_dvbt2.c_data != 0) _dvbt2.c_data -= 18;
            if (_dvbt2.n_fc != 0) _dvbt2.n_fc -= 18;
            if (_dvbt2.c_fc != 0) _dvbt2.c_fc -= 18;
        }
        break;
    case FFTSIZE_4K:
        switch (_dvbt2.pilot_pattern){
        case PP1:
            _dvbt2.c_data = 3084;
            _dvbt2.n_fc = 2272;
            _dvbt2.c_fc = 1609;
            break;
        case PP2:
            _dvbt2.c_data = 3092;
            _dvbt2.n_fc = 2840;
            _dvbt2.c_fc = 2619;
            break;
        case PP3:
            _dvbt2.c_data = 3228;
            _dvbt2.n_fc = 2840;
            _dvbt2.c_fc = 1961;
            break;
        case PP4:
            _dvbt2.c_data = 3234;
            _dvbt2.n_fc = 3124;
            _dvbt2.c_fc = 2831;
            break;
        case PP5:
            _dvbt2.c_data = 3298;
            _dvbt2.n_fc = 3124;
            _dvbt2.c_fc = 2177;
            break;
        case PP6:
            _dvbt2.c_data = 0;
            _dvbt2.n_fc = 0;
            _dvbt2.c_fc = 0;
            break;
        case PP7:
            _dvbt2.c_data = 3328;
            _dvbt2.n_fc = 3266;
            _dvbt2.c_fc = 2792;
            break;
        case PP8:
            _dvbt2.c_data = 0;
            _dvbt2.n_fc = 0;
            _dvbt2.c_fc = 0;
            break;
        }
        if (_dvbt2.papr_mode == PAPR_TR || _dvbt2.papr_mode == PAPR_BOTH){
            if (_dvbt2.c_data != 0) _dvbt2.c_data -= 36;
            if (_dvbt2.n_fc != 0) _dvbt2.n_fc -= 36;
            if (_dvbt2.c_fc != 0) _dvbt2.c_fc -= 36;
        }
        break;
    case FFTSIZE_8K:
    case FFTSIZE_8K_T2GI:
        if (_dvbt2.carrier_mode == CARRIERS_NORMAL){
            switch (_dvbt2.pilot_pattern){
            case PP1:
                _dvbt2.c_data = 6208;
                _dvbt2.n_fc = 4544;
                _dvbt2.c_fc = 3218;
                break;
            case PP2:
                _dvbt2.c_data = 6214;
                _dvbt2.n_fc = 5680;
                _dvbt2.c_fc = 5238;
                break;
            case PP3:
                _dvbt2.c_data = 6494;
                _dvbt2.n_fc = 5680;
                _dvbt2.c_fc = 3922;
                break;
            case PP4:
                _dvbt2.c_data = 6498;
                _dvbt2.n_fc = 6248;
                _dvbt2.c_fc = 5662;
                break;
            case PP5:
                _dvbt2.c_data = 6634;
                _dvbt2.n_fc = 6248;
                _dvbt2.c_fc = 4354;
                break;
            case PP6:
                _dvbt2.c_data = 0;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            case PP7:
                _dvbt2.c_data = 6698;
                _dvbt2.n_fc = 6532;
                _dvbt2.c_fc = 5585;
                break;
            case PP8:
                _dvbt2.c_data = 6698;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            }
        }
        else{
            switch (_dvbt2.pilot_pattern){
            case PP1:
                _dvbt2.c_data = 6296;
                _dvbt2.n_fc = 4608;
                _dvbt2.c_fc = 3264;
                break;
            case PP2:
                _dvbt2.c_data = 6298;
                _dvbt2.n_fc = 5760;
                _dvbt2.c_fc = 5312;
                break;
            case PP3:
                _dvbt2.c_data = 6584;
                _dvbt2.n_fc = 5760;
                _dvbt2.c_fc = 3978;
                break;
            case PP4:
                _dvbt2.c_data = 6588;
                _dvbt2.n_fc = 6336;
                _dvbt2.c_fc = 5742;
                break;
            case PP5:
                _dvbt2.c_data = 6728;
                _dvbt2.n_fc = 6336;
                _dvbt2.c_fc = 4416;
                break;
            case PP6:
                _dvbt2.c_data = 0;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            case PP7:
                _dvbt2.c_data = 6788;
                _dvbt2.n_fc = 6624;
                _dvbt2.c_fc = 5664;
                break;
            case PP8:
                _dvbt2.c_data = 6788;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            }
        }
        if (_dvbt2.papr_mode == PAPR_TR || _dvbt2.papr_mode == PAPR_BOTH){
            if (_dvbt2.c_data != 0) _dvbt2.c_data -= 72;
            if (_dvbt2.n_fc != 0) _dvbt2.n_fc -= 72;
            if (_dvbt2.c_fc != 0) _dvbt2.c_fc -= 72;
        }
        break;
    case FFTSIZE_16K:
    case FFTSIZE_16K_T2GI:
        if (_dvbt2.carrier_mode == CARRIERS_NORMAL){
            switch (_dvbt2.pilot_pattern){
            case PP1:
                _dvbt2.c_data = 12418;
                _dvbt2.n_fc = 9088;
                _dvbt2.c_fc = 6437;
                break;
            case PP2:
                _dvbt2.c_data = 12436;
                _dvbt2.n_fc = 11360;
                _dvbt2.c_fc = 10476;
                break;
            case PP3:
                _dvbt2.c_data = 12988;
                _dvbt2.n_fc = 11360;
                _dvbt2.c_fc = 7845;
                break;
            case PP4:
                _dvbt2.c_data = 13002;
                _dvbt2.n_fc = 12496;
                _dvbt2.c_fc = 11324;
                break;
            case PP5:
                _dvbt2.c_data = 13272;
                _dvbt2.n_fc = 12496;
                _dvbt2.c_fc = 8709;
                break;
            case PP6:
                _dvbt2.c_data = 13288;
                _dvbt2.n_fc = 13064;
                _dvbt2.c_fc = 11801;
                break;
            case PP7:
                _dvbt2.c_data = 13416;
                _dvbt2.n_fc = 13064;
                _dvbt2.c_fc = 11170;
                break;
            case PP8:
                _dvbt2.c_data = 13406;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            }
        }
        else{
            switch (_dvbt2.pilot_pattern){
            case PP1:
                _dvbt2.c_data = 12678;
                _dvbt2.n_fc = 9280;
                _dvbt2.c_fc = 6573;
                break;
            case PP2:
                _dvbt2.c_data = 12698;
                _dvbt2.n_fc = 11600;
                _dvbt2.c_fc = 10697;
                break;
            case PP3:
                _dvbt2.c_data = 13262;
                _dvbt2.n_fc = 11600;
                _dvbt2.c_fc = 8011;
                break;
            case PP4:
                _dvbt2.c_data = 13276;
                _dvbt2.n_fc = 12760;
                _dvbt2.c_fc = 11563;
                break;
            case PP5:
                _dvbt2.c_data = 13552;
                _dvbt2.n_fc = 12760;
                _dvbt2.c_fc = 8893;
                break;
            case PP6:
                _dvbt2.c_data = 13568;
                _dvbt2.n_fc = 13340;
                _dvbt2.c_fc = 12051;
                break;
            case PP7:
                _dvbt2.c_data = 13698;
                _dvbt2.n_fc = 13340;
                _dvbt2.c_fc = 11406;
                break;
            case PP8:
                _dvbt2.c_data = 13688;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            }
        }
        if (_dvbt2.papr_mode == PAPR_TR || _dvbt2.papr_mode == PAPR_BOTH){
            if (_dvbt2.c_data != 0) _dvbt2.c_data -= 144;
            if (_dvbt2.n_fc != 0) _dvbt2.n_fc -= 144;
            if (_dvbt2.c_fc != 0) _dvbt2.c_fc -= 144;
        }
        break;
    case FFTSIZE_32K:
    case FFTSIZE_32K_T2GI:
        if (_dvbt2.carrier_mode == CARRIERS_NORMAL){
            switch (_dvbt2.pilot_pattern){
            case PP1:
                _dvbt2.c_data = 0;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            case PP2:
                _dvbt2.c_data = 24886;
                _dvbt2.n_fc = 22720;
                _dvbt2.c_fc = 20952;
                break;
            case PP3:
                _dvbt2.c_data = 0;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            case PP4:
                _dvbt2.c_data = 26022;
                _dvbt2.n_fc = 24992;
                _dvbt2.c_fc = 22649;
                break;
            case PP5:
                _dvbt2.c_data = 0;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            case PP6:
                _dvbt2.c_data = 26592;
                _dvbt2.n_fc = 26128;
                _dvbt2.c_fc = 23603;
                break;
            case PP7:
                _dvbt2.c_data = 26836;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            case PP8:
                _dvbt2.c_data = 26812;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            }
        }
        else{
            switch (_dvbt2.pilot_pattern){
            case PP1:
                _dvbt2.c_data = 0;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            case PP2:
                _dvbt2.c_data = 25412;
                _dvbt2.n_fc = 23200;
                _dvbt2.c_fc = 21395;
                break;
            case PP3:
                _dvbt2.c_data = 0;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            case PP4:
                _dvbt2.c_data = 26572;
                _dvbt2.n_fc = 25520;
                _dvbt2.c_fc = 23127;
                break;
            case PP5:
                _dvbt2.c_data = 0;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            case PP6:
                _dvbt2.c_data = 27152;
                _dvbt2.n_fc = 26680;
                _dvbt2.c_fc = 24102;
                break;
            case PP7:
                _dvbt2.c_data = 27404;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            case PP8:
                _dvbt2.c_data = 27376;
                _dvbt2.n_fc = 0;
                _dvbt2.c_fc = 0;
                break;
            }
        }
        if (_dvbt2.papr_mode == PAPR_TR || _dvbt2.papr_mode == PAPR_BOTH){
            if (_dvbt2.c_data != 0) _dvbt2.c_data -= 288;
            if (_dvbt2.n_fc != 0) _dvbt2.n_fc -= 288;
            if (_dvbt2.c_fc != 0) _dvbt2.c_fc -= 288;
        }
        break;
    }
    if (_dvbt2.miso == FALSE){
        if (_dvbt2.guard_interval_mode == GI_1_128 && _dvbt2.pilot_pattern == PP7){
            _dvbt2.n_fc = 0;
            _dvbt2.c_fc = 0;
        }
        if (_dvbt2.guard_interval_mode == GI_1_32 && _dvbt2.pilot_pattern == PP4){
            _dvbt2.n_fc = 0;
            _dvbt2.c_fc = 0;
        }
        if (_dvbt2.guard_interval_mode == GI_1_16 && _dvbt2.pilot_pattern == PP2){
            _dvbt2.n_fc = 0;
            _dvbt2.c_fc = 0;
        }
        if (_dvbt2.guard_interval_mode == GI_19_256 && _dvbt2.pilot_pattern == PP2){
            _dvbt2.n_fc = 0;
            _dvbt2.c_fc = 0;
        }
    }
    switch (_dvbt2.guard_interval_mode) {
    case GI_1_4:
        _dvbt2.guard_interval_size = _dvbt2.fft_size / 4;
        break;
    case GI_1_8:
        _dvbt2.guard_interval_size = _dvbt2.fft_size / 8;
        break;
    case GI_1_16:
        _dvbt2.guard_interval_size = _dvbt2.fft_size / 16;
        break;
    case GI_1_32:
        _dvbt2.guard_interval_size = _dvbt2.fft_size / 32;
        break;
    case GI_1_128:
        _dvbt2.guard_interval_size = _dvbt2.fft_size / 128;
        break;
    case GI_19_128:
        _dvbt2.guard_interval_size = (_dvbt2.fft_size / 128) * 19 ;
        break;
    case GI_19_256:
        _dvbt2.guard_interval_size = (_dvbt2.fft_size / 256) * 19 ;
        break;
    default:
        break;
    }

    if(_dvbt2.n_fc == 0) _dvbt2.l_fc = 0;
    else _dvbt2.l_fc = 1;
    _dvbt2.len_frame = _dvbt2.n_p2 + _dvbt2.n_data;
}
//-----------------------------------------------------------------------------------------------
