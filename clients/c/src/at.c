/**
 * @file:    at.c
 *
 * Purpose:  AT commands utilities
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
 * AT commands implementation
 *
 */

#include "at.h"
#include "legato.h"
#include <string.h>

static char* at_prefix="AT+ORP=\"";
static char* at_suffix="\"\n";

//--------------------------------------------------------------------------------------------------
/**
 * Pack an AT frame
 *
 * Prepare the final AT command string from an ORP command
 *
 */
//--------------------------------------------------------------------------------------------------
ssize_t at_Pack
(
    uint8_t        *dest,              // pointer to first empty byte of destination buffer
    size_t          destlen,           // number of data bytes to frame into destination
    uint8_t        *src,               // pointer to first unprocessed byte of source buffer
    size_t         *srclen             // number of bytes-in / processed-from the source buffer
)
//--------------------------------------------------------------------------------------------------
{
    int src_idx = 0;
    int dst_idx = -1;

    // Sanity check for dest size
    if(destlen < *srclen + strlen(at_prefix) + strlen(at_suffix))
    {
        LE_ERROR("Dest buffer too small");
        return dst_idx;
    }

    // AT command prefix
    strncpy(dest, at_prefix, destlen);
    dst_idx = strlen(at_prefix);
    
    // ORP packet type
    dest[dst_idx++] = src[src_idx++];

    // ORP data type (may be NULL)
    if (0 != src[src_idx])
    {
        dest[dst_idx++] = src[src_idx];
    }
    else
    {
       dest[dst_idx++] = '0';
    }

    src_idx++;
    
    // ORP Seq number fixed to "00" in AT mode
    dest[dst_idx++] = '0'; src_idx++;
    dest[dst_idx++] = '0'; src_idx++;

    // ORP command
    while ((src_idx < *srclen) && (dst_idx < destlen))
    {
        dest[dst_idx++] = src[src_idx++];
    }

    // AT command suffix
    strncpy(dest + dst_idx, at_suffix, destlen - dst_idx);
    dst_idx += strlen(at_suffix);
    
    return dst_idx;
}