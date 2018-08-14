/*
 * Copyright (C) 1999-2001 Free Software Foundation, Inc.
 * This file is part of the GNU LIBICONV Library.
 *
 * The GNU LIBICONV Library is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * The GNU LIBICONV Library is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the GNU LIBICONV Library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation, Inc., 51 Franklin Street,
 * Fifth Floor, Boston, MA 02110-1301, USA.
 */

/*
 * MacUkraine
 */

static const unsigned short mac_ukraine_2uni[128] = {
    /* 0x80 */
    0x0410, 0x0411, 0x0412, 0x0413, 0x0414, 0x0415, 0x0416, 0x0417,
    0x0418, 0x0419, 0x041a, 0x041b, 0x041c, 0x041d, 0x041e, 0x041f,
    /* 0x90 */
    0x0420, 0x0421, 0x0422, 0x0423, 0x0424, 0x0425, 0x0426, 0x0427,
    0x0428, 0x0429, 0x042a, 0x042b, 0x042c, 0x042d, 0x042e, 0x042f,
    /* 0xa0 */
    0x2020, 0x00b0, 0x0490, 0x00a3, 0x00a7, 0x2022, 0x00b6, 0x0406,
    0x00ae, 0x00a9, 0x2122, 0x0402, 0x0452, 0x2260, 0x0403, 0x0453,
    /* 0xb0 */
    0x221e, 0x00b1, 0x2264, 0x2265, 0x0456, 0x00b5, 0x0491, 0x0408,
    0x0404, 0x0454, 0x0407, 0x0457, 0x0409, 0x0459, 0x040a, 0x045a,
    /* 0xc0 */
    0x0458, 0x0405, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab,
    0x00bb, 0x2026, 0x00a0, 0x040b, 0x045b, 0x040c, 0x045c, 0x0455,
    /* 0xd0 */
    0x2013, 0x2014, 0x201c, 0x201d, 0x2018, 0x2019, 0x00f7, 0x201e,
    0x040e, 0x045e, 0x040f, 0x045f, 0x2116, 0x0401, 0x0451, 0x044f,
    /* 0xe0 */
    0x0430, 0x0431, 0x0432, 0x0433, 0x0434, 0x0435, 0x0436, 0x0437,
    0x0438, 0x0439, 0x043a, 0x043b, 0x043c, 0x043d, 0x043e, 0x043f,
    /* 0xf0 */
    0x0440, 0x0441, 0x0442, 0x0443, 0x0444, 0x0445, 0x0446, 0x0447,
    0x0448, 0x0449, 0x044a, 0x044b, 0x044c, 0x044d, 0x044e, 0x00a4,
};

static int
mac_ukraine_mbtowc(conv_t conv, ucs4_t *pwc, const unsigned char *s, int n)
{
    unsigned char c = *s;
    if (c >= 0x80) {
        *pwc = (ucs4_t) mac_ukraine_2uni[c - 0x80];
    } else {
        *pwc = (ucs4_t) c;
    }
    return 1;
}

static const unsigned char mac_ukraine_page00[32] = {
    0xca, 0x00, 0x00, 0xa3, 0xff, 0x00, 0x00, 0xa4, /* 0xa0-0xa7 */
    0x00, 0xa9, 0x00, 0xc7, 0xc2, 0x00, 0xa8, 0x00, /* 0xa8-0xaf */
    0xa1, 0xb1, 0x00, 0x00, 0x00, 0xb5, 0xa6, 0x00, /* 0xb0-0xb7 */
    0x00, 0x00, 0x00, 0xc8, 0x00, 0x00, 0x00, 0x00, /* 0xb8-0xbf */
};
static const unsigned char mac_ukraine_page04[152] = {
    0x00, 0xdd, 0xab, 0xae, 0xb8, 0xc1, 0xa7, 0xba, /* 0x00-0x07 */
    0xb7, 0xbc, 0xbe, 0xcb, 0xcd, 0x00, 0xd8, 0xda, /* 0x08-0x0f */
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, /* 0x10-0x17 */
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, /* 0x18-0x1f */
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, /* 0x20-0x27 */
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, /* 0x28-0x2f */
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, /* 0x30-0x37 */
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, /* 0x38-0x3f */
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, /* 0x40-0x47 */
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xdf, /* 0x48-0x4f */
    0x00, 0xde, 0xac, 0xaf, 0xb9, 0xcf, 0xb4, 0xbb, /* 0x50-0x57 */
    0xc0, 0xbd, 0xbf, 0xcc, 0xce, 0x00, 0xd9, 0xdb, /* 0x58-0x5f */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x60-0x67 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x68-0x6f */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x70-0x77 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x78-0x7f */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x80-0x87 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x88-0x8f */
    0xa2, 0xb6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x90-0x97 */
};
static const unsigned char mac_ukraine_page20[24] = {
    0x00, 0x00, 0x00, 0xd0, 0xd1, 0x00, 0x00, 0x00, /* 0x10-0x17 */
    0xd4, 0xd5, 0x00, 0x00, 0xd2, 0xd3, 0xd7, 0x00, /* 0x18-0x1f */
    0xa0, 0x00, 0xa5, 0x00, 0x00, 0x00, 0xc9, 0x00, /* 0x20-0x27 */
};
static const unsigned char mac_ukraine_page21[24] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xdc, 0x00, /* 0x10-0x17 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x18-0x1f */
    0x00, 0x00, 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x20-0x27 */
};
static const unsigned char mac_ukraine_page22[104] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc6, 0x00, /* 0x00-0x07 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x08-0x0f */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x10-0x17 */
    0x00, 0x00, 0xc3, 0x00, 0x00, 0x00, 0xb0, 0x00, /* 0x18-0x1f */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x20-0x27 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x28-0x2f */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x30-0x37 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x38-0x3f */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x40-0x47 */
    0xc5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x48-0x4f */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x50-0x57 */
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* 0x58-0x5f */
    0xad, 0x00, 0x00, 0x00, 0xb2, 0xb3, 0x00, 0x00, /* 0x60-0x67 */
};

static int
mac_ukraine_wctomb(conv_t conv, unsigned char *r, ucs4_t wc, int n)
{
    unsigned char c = 0;
    if (wc < 0x0080) {
        *r = wc;
        return 1;
    } else if (wc >= 0x00a0 && wc < 0x00c0) {
        c = mac_ukraine_page00[wc - 0x00a0];
    } else if (wc == 0x00f7) {
        c = 0xd6;
    } else if (wc == 0x0192) {
        c = 0xc4;
    } else if (wc >= 0x0400 && wc < 0x0498) {
        c = mac_ukraine_page04[wc - 0x0400];
    } else if (wc >= 0x2010 && wc < 0x2028) {
        c = mac_ukraine_page20[wc - 0x2010];
    } else if (wc >= 0x2110 && wc < 0x2128) {
        c = mac_ukraine_page21[wc - 0x2110];
    } else if (wc >= 0x2200 && wc < 0x2268) {
        c = mac_ukraine_page22[wc - 0x2200];
    }
    if (c != 0) {
        *r = c;
        return 1;
    }
    return RET_ILUNI;
}
