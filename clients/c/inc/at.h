/**
 * @file:    at.h
 *
 * Purpose:  Simplified Asynchronous HDLC utilities
 *
 * MIT License
 *
 * Copyright (c) 2021 Sierra Wireless Inc.
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
 
 */

#ifndef AT_H_INCLUDE_GUARD
#define AT_H_INCLUDE_GUARD

#include <sys/types.h>
#include <stdbool.h>
#include <stdint.h>



//--------------------------------------------------------------------------------------------------
/**
 * Unpack an AT frame
 *
 */
//--------------------------------------------------------------------------------------------------
ssize_t at_Unpack
(
    uint8_t        *dest,              // pointer to first empty byte of destination buffer
    size_t          destlen,           // number of data bytes to unframe into destination
    uint8_t        *src,               // pointer to first unprocessed byte of source (frame) buffer
    size_t         *srclen             // number of bytes in, or processed-from, the source buffer
);


//--------------------------------------------------------------------------------------------------
/**
 * Pack an AT frame
 *
 */
//--------------------------------------------------------------------------------------------------
ssize_t at_Pack
(
    uint8_t        *dest,              // pointer to first empty byte of destination buffer
    size_t          destlen,           // number of data bytes to frame into destination
    uint8_t        *src,               // pointer to first unprocessed byte of source buffer
    size_t         *srclen             // number of bytes-in / processed-from the source buffer
);


//--------------------------------------------------------------------------------------------------
/**
 * Complete AT packing
 * 
 */
//--------------------------------------------------------------------------------------------------
ssize_t at_PackFinalize
(
    uint8_t        *dest,
    size_t          destlen
);


#endif // AT_H_INCLUDE_GUARD

