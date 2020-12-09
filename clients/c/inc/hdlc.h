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
 * Usage: (see headers below)
 *
 * - hdlc_Pack / hdlc_Unpack routines may be called multiple times on a stream
 *   of bytes
 * - hdlc_Init must be called before packing / unpacking each new frame
 * - hdlc_UnpackDone must be called to check for unpacking complete
 * - hdlc_PackFinalize must be called to complete packing
 */

#ifndef HDLC_H_INCLUDE_GUARD
#define HDLC_H_INCLUDE_GUARD

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>


//--------------------------------------------------------------------------------------------------
/**
 * Defines, Constants
 */
//--------------------------------------------------------------------------------------------------
// Leading 0x7E + 16-bit CRC (possibly escaped to 4 bytes) + trailing 0x7E
#define HDLC_OVERHEAD_BYTES_COUNT 6


//--------------------------------------------------------------------------------------------------
/**
 * HDLC Context structure
 *
 * @note This context is used to allow multiple calls to the packing and unpacking functions
 * in order to support frames or data spanning multiple buffers.  The structure must be
 * (re) initialized before the start of each packing or unpacking operation
 */
//--------------------------------------------------------------------------------------------------
typedef struct
{
    int state;
    int count;
    uint16_t crc;
    uint8_t  crcbuf[sizeof(uint16_t)];
}
hdlc_context_t;


//--------------------------------------------------------------------------------------------------
/**
 * HDLC Error codes
 *
 * @note HDLC routines can return these negative values on error
 */
//--------------------------------------------------------------------------------------------------
typedef enum
{
    HDLC_ERROR_UNSPECIFIED = -1,       // Do not change this value
    HDLC_ERROR_CRC         = -2,
    HDLC_ERROR_FRAME       = -3
}
hdlc_error_t;


//--------------------------------------------------------------------------------------------------
/**
 * Initialize HDLC context
 *
 * @note Must be called to reinitialize the HDLC context each time a new frame is to be packed
 * or unpacked
 *
 * @return none
 */
//--------------------------------------------------------------------------------------------------
void hdlc_Init
(
    hdlc_context_t *hdlc               // pointer to HDLC context structure
);


//--------------------------------------------------------------------------------------------------
/**
 * Unpack an HDLC frame
 *
 * @note
 * - hdlc_Init must be called first, for each new frame to be processed
 * - Call hdlc_state after each call to hdlc_Unpack to check if unpacking is complete
 * - May be called multiple times until a complete frame is decoded
 * - If started in the middle of a frame, will search for <7E> or <7E><7E>
 *
 * @return  >= 0 : length data unpacked on this call
 * @return  <  0 : failure
 * @return  srclen : IN - count of source bytes to unpack.  OUT - count of source bytes unpacked
 *
 * *** The trailing frame byte is NOT included in the count of source bytes unpacked ***
 */
//--------------------------------------------------------------------------------------------------
ssize_t hdlc_Unpack
(
    hdlc_context_t *hdlc,              // pointer to HDLC context structure
    uint8_t        *dest,              // pointer to first empty byte of destination buffer
    size_t          destlen,           // number of data bytes to unframe into destination
    uint8_t        *src,               // pointer to first unprocessed byte of source (frame) buffer
    size_t         *srclen             // number of bytes in, or processed-from, the source buffer
);


//--------------------------------------------------------------------------------------------------
/**
 * Check whether HDLC unpacking is complete
 *
 * @return true: complete, false: not complete
 */
//--------------------------------------------------------------------------------------------------
bool hdlc_UnpackDone
(
    hdlc_context_t *hdlc
);


//--------------------------------------------------------------------------------------------------
/**
 * Pack an HDLC frame
 *
 * @note
 * - hdlc_Init must be called first, for each new frame to be encoded
 * - May be called multiple times until a complete frame is encoded
 * - hdlc_PackFinalize must be called to finalize the encoding, after all data is processed
 *
 * @return  >= 0 : number of bytes added to frame on this call
 * @return  <  0 : failure
 * @return  srclen : IN- bytes of data to pack.  OUT - bytes actually packed
 */
//--------------------------------------------------------------------------------------------------
ssize_t hdlc_Pack
(
    hdlc_context_t *hdlc,              // pointer to HDLC context structure
    uint8_t        *dest,              // pointer to first empty byte of destination buffer
    size_t          destlen,           // number of data bytes to frame into destination
    uint8_t        *src,               // pointer to first unprocessed byte of source buffer
    size_t         *srclen             // number of bytes-in / processed-from the source buffer
);


//--------------------------------------------------------------------------------------------------
/**
 * Complete HDLC packing - call to complete packing, after all data has been processed
 *
 * @return  >= 0 : number of bytes added to frame by the finalize operation: CRC, any required
 *                 escape characters, and the final frame delimiter 0x7E
 * @return  <  0 : failure
 */
//--------------------------------------------------------------------------------------------------
ssize_t hdlc_PackFinalize
(
    hdlc_context_t *hdlc,
    uint8_t        *dest,
    size_t          destlen
);


#endif // HDLC_H_INCLUDE_GUARD

