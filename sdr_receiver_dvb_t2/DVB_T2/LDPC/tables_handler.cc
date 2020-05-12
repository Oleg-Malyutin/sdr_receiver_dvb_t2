/*
LDPC tables handler

Copyright 2018 Ahmet Inan <xdsopl@gmail.com>
*/

#include <cstring>
#include <cstdint>
#include "ldpc.hh"

#include "dvb_t2_tables.hh"

LDPCInterface *create_ldpc(char *standard, char prefix, int number)
{
	if (!strcmp(standard, "T2")) {
		if (prefix == 'A') {
			switch (number) {
			case 1:
                return new LDPC<DVB_T2_TABLE_NORMAL_C1_2>();
			case 2:
                return new LDPC<DVB_T2_TABLE_NORMAL_C3_5>();
			case 3:
                return new LDPC<DVB_T2_TABLE_NORMAL_C2_3>();
			case 4:
                return new LDPC<DVB_T2_TABLE_NORMAL_C3_4>();
			case 5:
                return new LDPC<DVB_T2_TABLE_NORMAL_C4_5>();
			case 6:
                return new LDPC<DVB_T2_TABLE_NORMAL_C5_6>();
			}
		}
		if (prefix == 'B') {
			switch (number) {
			case 1:
                return new LDPC<DVB_T2_TABLE_SHORT_C1_4>();
			case 2:
                return new LDPC<DVB_T2_TABLE_SHORT_C1_2>();
			case 3:
                return new LDPC<DVB_T2_TABLE_SHORT_C3_5>();
			case 4:
                return new LDPC<DVB_T2_TABLE_SHORT_C2_3>();
			case 5:
                return new LDPC<DVB_T2_TABLE_SHORT_C3_4>();
			case 6:
                return new LDPC<DVB_T2_TABLE_SHORT_C4_5>();
			case 7:
                return new LDPC<DVB_T2_TABLE_SHORT_C5_6>();
			case 8:
				return new LDPC<DVB_T2_TABLE_B8>();
			case 9:
                return new LDPC<DVB_T2_TABLE_B9>();
			}
		}
	}
    return nullptr;
}

//constexpr int DVB_T2_TABLE_NORMAL_C1_2::DEG[];
//constexpr int DVB_T2_TABLE_NORMAL_C1_2::LEN[];
//constexpr int DVB_T2_TABLE_NORMAL_C1_2::POS[];

//constexpr int DVB_T2_TABLE_NORMAL_C3_5::DEG[];
//constexpr int DVB_T2_TABLE_NORMAL_C3_5::LEN[];
//constexpr int DVB_T2_TABLE_NORMAL_C3_5::POS[];

//constexpr int DVB_T2_TABLE_NORMAL_C2_3::DEG[];
//constexpr int DVB_T2_TABLE_NORMAL_C2_3::LEN[];
//constexpr int DVB_T2_TABLE_NORMAL_C2_3::POS[];

//constexpr int DVB_T2_TABLE_NORMAL_C3_4::DEG[];
//constexpr int DVB_T2_TABLE_NORMAL_C3_4::LEN[];
//constexpr int DVB_T2_TABLE_NORMAL_C3_4::POS[];

//constexpr int DVB_T2_TABLE_NORMAL_C4_5::DEG[];
//constexpr int DVB_T2_TABLE_NORMAL_C4_5::LEN[];
//constexpr int DVB_T2_TABLE_NORMAL_C4_5::POS[];

//constexpr int DVB_T2_TABLE_NORMAL_C5_6::DEG[];
//constexpr int DVB_T2_TABLE_NORMAL_C5_6::LEN[];
//constexpr int DVB_T2_TABLE_NORMAL_C5_6::POS[];

//constexpr int DVB_T2_TABLE_SHORT_C1_4::DEG[];
//constexpr int DVB_T2_TABLE_SHORT_C1_4::LEN[];
//constexpr int DVB_T2_TABLE_SHORT_C1_4::POS[];

//constexpr int DVB_T2_TABLE_SHORT_C1_2::DEG[];
//constexpr int DVB_T2_TABLE_SHORT_C1_2::LEN[];
//constexpr int DVB_T2_TABLE_SHORT_C1_2::POS[];

//constexpr int DVB_T2_TABLE_SHORT_C3_5::DEG[];
//constexpr int DVB_T2_TABLE_SHORT_C3_5::LEN[];
//constexpr int DVB_T2_TABLE_SHORT_C3_5::POS[];

//constexpr int DVB_T2_TABLE_SHORT_C2_3::DEG[];
//constexpr int DVB_T2_TABLE_SHORT_C2_3::LEN[];
//constexpr int DVB_T2_TABLE_SHORT_C2_3::POS[];

//constexpr int DVB_T2_TABLE_SHORT_C3_4::DEG[];
//constexpr int DVB_T2_TABLE_SHORT_C3_4::LEN[];
//constexpr int DVB_T2_TABLE_SHORT_C3_4::POS[];

//constexpr int DVB_T2_TABLE_SHORT_C4_5::DEG[];
//constexpr int DVB_T2_TABLE_SHORT_C4_5::LEN[];
//constexpr int DVB_T2_TABLE_SHORT_C4_5::POS[];

//constexpr int DVB_T2_TABLE_SHORT_C5_6::DEG[];
//constexpr int DVB_T2_TABLE_SHORT_C5_6::LEN[];
//constexpr int DVB_T2_TABLE_SHORT_C5_6::POS[];

//constexpr int DVB_T2_TABLE_B8::DEG[];
//constexpr int DVB_T2_TABLE_B8::LEN[];
//constexpr int DVB_T2_TABLE_B8::POS[];

//constexpr int DVB_T2_TABLE_B9::DEG[];
//constexpr int DVB_T2_TABLE_B9::LEN[];
//constexpr int DVB_T2_TABLE_B9::POS[];

