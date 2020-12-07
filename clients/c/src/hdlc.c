/**
 * @file:    hdlc.h
 *
 * Purpose:  Simplified Asynchronous HDLC utilities
 *
 * MIT License
 *
 * Copyright (c) 2020 Sierra Wireless Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *----------------------------------------------------------------------------
 *
 * NOTES:
 *
 * Partial HDLC implementation
 * - Includes framing, escaping, and 16-bit CRC-CCITT
 * - Does not include Address or control fields, or ACK/NACK
 *
 * Usage: see hdlc.h
 *
 */

#include "hdlc.h"
#include "legato.h"
#include <string.h>

//--------------------------------------------------------------------------------------------------
/**
 * Internal Definitions
 */
//--------------------------------------------------------------------------------------------------
/* Async HDLC achieves data transparency at the byte level by using two
 * special values. The first is a flag value which begins and ends every
 * packet: */
#define  HDLC_FRAME_OCTET      0x7E

/* The flag value might appear in the data.  If it does, it is sent as a
 * two-byte sequence consisting of a special escape value followed by the
 * flag value XORed with 0x20.  This gives a special meaning to the escape
 * character, so if it appears in the data it is itself escaped in the same
 * way. */
#define  HDLC_ESC_OCTET        0x7D
#define  HDLC_ESC_MASK         0x20

/* Indicies for the temporary CRC buffer.  The CRC is received LSB first, so the LSB is the
 * older of the two bytes:
 *
 *   <data_0>...<data_N><CRC_LSB><CRC_MSB>
 *   -------------------------------------> time
 */
#define  HDLC_FRAM_CRC_LSB     0
#define  HDLC_FRAM_CRC_MSB     1

//--------------------------------------------------------------------------------------------------
/**
 * States used when processing simplified HDLC framing protocol
 */
//--------------------------------------------------------------------------------------------------
enum hdlc_state
{
    HDLC_INIT = 0,             /* State intialized, no calls yet                    */

    UNPACK_SOF_SEARCH,         /* State machine is hunting for the start of a frame */
    UNPACK_SOF_FOUND,          /* Frame detected                                    */
    UNPACK_DATA,               /* Receiving data                                    */
    UNPACK_ESCAPED,            /* Escape detected, getting the next character       */

    PACK_START,                /* Sending the opening frame of a packet             */
    PACK_DATA,                 /* Sending out data characters, no escape            */
    PACK_ESCAPED               /* Escape character was sent                         */
};

// This is to be moved to crc.[ch]
#define CRC_CRC16_CCITT_INIT 0xFFFF
#define CRC_POLY_CCITT 0x1021
static bool             crc_tabccitt_init       = false;
static uint16_t         crc_tabccitt[256];
static void init_crcccitt_tab( void ) {

	uint16_t i;
	uint16_t j;
	uint16_t crc;
	uint16_t c;

	for (i=0; i<256; i++) {

		crc = 0;
		c   = i << 8;

		for (j=0; j<8; j++) {

			if ( (crc ^ c) & 0x8000 ) crc = ( crc << 1 ) ^ CRC_POLY_CCITT;
			else                      crc =   crc << 1;

			c = c << 1;
		}

		crc_tabccitt[i] = crc;
	}

	crc_tabccitt_init = true;

}  /* init_crcccitt_tab */

uint16_t _crc_ccitt_update( uint16_t crc, uint8_t c ) {

	if ( ! crc_tabccitt_init ) init_crcccitt_tab();

	return (crc << 8) ^ crc_tabccitt[ ((crc >> 8) ^ (uint16_t) c) & 0x00FF ];

}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize HDLC context
 */
//--------------------------------------------------------------------------------------------------
void hdlc_Init
(
    hdlc_context_t *hdlc
)
//--------------------------------------------------------------------------------------------------
{
    LE_ASSERT(hdlc);
    memset(hdlc, 0, sizeof(hdlc_context_t));
    hdlc->state = HDLC_INIT;
    hdlc->crc = CRC_CRC16_CCITT_INIT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Unpack an HDLC frame
 */
//--------------------------------------------------------------------------------------------------
ssize_t hdlc_Unpack
(
    hdlc_context_t *hdlc,
    uint8_t        *dest,
    size_t          destlen,
    uint8_t        *src,
    size_t         *srclen
)
//--------------------------------------------------------------------------------------------------
{
    int      dst_idx = 0;               // recieved length of the packet
    int      src_idx = 0;               // recieved length of the packet

    if (hdlc)
    {
        while ((src_idx < (int)*srclen) && (dst_idx < (int)destlen))
        {
            uint8_t data = src[src_idx];

            switch (hdlc->state)
            {
                case HDLC_INIT:
                    hdlc->state = UNPACK_SOF_SEARCH;
                    hdlc->crc = CRC_CRC16_CCITT_INIT;
                    hdlc->count = 0;
                    /* fall through */

                case UNPACK_SOF_SEARCH:
                    if (data == HDLC_FRAME_OCTET)
                    {
                        hdlc->state = UNPACK_SOF_FOUND;
                    }
                    /* discard anything else until SOF found */
                    break;

                case UNPACK_SOF_FOUND:
                    switch (data)
                    {
                        /* contiguous frame flag - no change */
                        case HDLC_FRAME_OCTET:
                            break;

                        case HDLC_ESC_OCTET:
                            hdlc->state = UNPACK_ESCAPED;
                            break;

                        default:
                            hdlc->state = UNPACK_DATA;
                            break;
                    }
                    break;

                case UNPACK_DATA:
                    switch (data)
                    {
                        /* Frame Boundary: Unpacking complete. Compare the calculated CRC with
                         * the CRC in the frame
                         */
                        case HDLC_FRAME_OCTET:
                        {
                            uint16_t sndrcrc = ((uint16_t)(hdlc->crcbuf[HDLC_FRAM_CRC_MSB]) << 8) + hdlc->crcbuf[HDLC_FRAM_CRC_LSB];
                            if (hdlc->crc != sndrcrc)
                            {
                                LE_INFO("CRC Mismatch: calculated %04X, received %04X", hdlc->crc, sndrcrc);
                                dst_idx = HDLC_ERROR_CRC;
                            }
                            hdlc->state = HDLC_INIT;
                            break;
                        }

                        case HDLC_ESC_OCTET:
                            hdlc->state = UNPACK_ESCAPED;
                            break;

                        // Regular data
                        default:
                            break;
                    }
                    break;

                case UNPACK_ESCAPED:
                    switch (data)
                    {
                        /* Errors:  Indicate error and reset state
                         * - Duplicate escape
                         * - Frame boundary while escaped
                         */
                        case HDLC_ESC_OCTET:
                        case HDLC_FRAME_OCTET:
                            LE_ERROR("Framing error");
                            dst_idx = HDLC_ERROR_FRAME;
                            hdlc->state = HDLC_INIT;
                            break;

                        default:
                            data ^= HDLC_ESC_MASK;
                            hdlc->state = UNPACK_DATA;
                            break;
                    }
                    break;

                default:
                    LE_FATAL("Invalid state %d", hdlc->state);
                    break;
            }

            /* If unpacking, copy data to output buffer and update CRC */
            if (UNPACK_DATA == hdlc->state)
            {
                /* running crc calculation on the unpacked data
                 * if the current length of data unpacked is less than 2 bytes, it may actually be
                 * the crc itself.  Therefore, buffer the two most recent bytes until the end of
                 * frame occurs
                 */
                if (hdlc->count > 1)
                {
                    dest[dst_idx] = hdlc->crcbuf[HDLC_FRAM_CRC_MSB];
                    hdlc->crc = _crc_ccitt_update(hdlc->crc, dest[dst_idx]);
                    dst_idx++;
                }
                hdlc->crcbuf[HDLC_FRAM_CRC_MSB] = hdlc->crcbuf[HDLC_FRAM_CRC_LSB];
                hdlc->crcbuf[HDLC_FRAM_CRC_LSB] = data;
                hdlc->count++;
            }

            src_idx++;

            if (HDLC_INIT == hdlc->state)
            {
                /* return to the caller */
                break;
            }
        }
    }

    *srclen = src_idx;

     return dst_idx;
}


//--------------------------------------------------------------------------------------------------
/**
 * Check whether HDLC unpacking is complete
 */
//--------------------------------------------------------------------------------------------------
bool hdlc_UnpackDone
(
    hdlc_context_t *hdlc
)
//--------------------------------------------------------------------------------------------------
{
    if (hdlc && (HDLC_INIT == hdlc->state))
    {
        return true;
    }
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Pack an HDLC frame
 */
//--------------------------------------------------------------------------------------------------
ssize_t hdlc_Pack
(
    hdlc_context_t *hdlc,
    uint8_t        *dest,
    size_t          destlen,
    uint8_t        *src,
    size_t         *srclen
)
//--------------------------------------------------------------------------------------------------
{
    int src_idx = 0;
    int dst_idx = -1;


    if (hdlc)
    {
        /* Newly initialized context - move to first state for packing */
        if (HDLC_INIT == hdlc->state)
        {
            hdlc->state = PACK_START;
        }

        for (dst_idx = 0;
             (src_idx < *srclen) && (dst_idx < destlen);
             dst_idx++)
        {
            switch (hdlc->state)
            {
                case PACK_START:
                    dest[dst_idx] = HDLC_FRAME_OCTET;
                    hdlc->state = PACK_DATA;
                    break;

                case PACK_DATA:
                    hdlc->crc = _crc_ccitt_update(hdlc->crc, src[src_idx]);
                    if ((src[src_idx] == HDLC_FRAME_OCTET) || (src[src_idx] == HDLC_ESC_OCTET))
                    {
                        dest[dst_idx] = HDLC_ESC_OCTET;
                        hdlc->state = PACK_ESCAPED;
                    }
                    else
                    {
                        dest[dst_idx] = src[src_idx++];
                    }
                    break;

                case PACK_ESCAPED:
                    dest[dst_idx] = (src[src_idx++] ^ HDLC_ESC_MASK);
                    hdlc->state = PACK_DATA;
                    break;

                default:
                    LE_FATAL("Invalid state %d", hdlc->state);
                    break;
            }
        }
    }

    *srclen = src_idx;

    return dst_idx;
}


//--------------------------------------------------------------------------------------------------
/**
 * Complete HDLC packing
 */
//--------------------------------------------------------------------------------------------------
ssize_t hdlc_PackFinalize
(
    hdlc_context_t *hdlc,
    uint8_t        *dest,
    size_t          destlen
)
//--------------------------------------------------------------------------------------------------
{
    ssize_t count = -1;


    // calculate crc and use hdlc_Pack() to add it to the end
    if (hdlc)
    {
        size_t crcLen = sizeof(hdlc->crc);
        hdlc->crcbuf[0] = hdlc->crc >> 8;
        hdlc->crcbuf[1] = hdlc->crc & 0x00FF;

        count = hdlc_Pack(hdlc, dest, destlen, hdlc->crcbuf, &crcLen);

        destlen -= count;

        if (destlen)
        {
            dest[count++] = HDLC_FRAME_OCTET;
        }
        else
        {
            LE_ERROR("Insufficient space");
            count = -1;
        }
    }
    return count;
}
