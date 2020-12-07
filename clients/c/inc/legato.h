/**
 * @file:    legato.h
 *
 * Purpose:  Minimal set of definitions required to compile the orp files
 *           independent of the Legato project
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
 * The definitions provided here are not necessarily the same as those in
 * Legato project.  See:  https://github.com/legatoproject/legato-af
 *
 */
#ifndef LEGATO_H_INCLUDE_GUARD
#define LEGATO_H_INCLUDE_GUARD

#include <stdio.h>
#include <stdlib.h>


#ifndef LE_DEBUG
#define LE_DEBUG(format, ...) {}
//#define LE_DEBUG(format, ...) printf("DBUG | " __FILE__ ", line: %u | " format "\r\n", __LINE__, ##__VA_ARGS__)
#endif

#ifndef LE_INFO
#define LE_INFO(format, ...)  printf("INFO | " __FILE__ ", line: %u | " format "\r\n", __LINE__, ##__VA_ARGS__)
#endif

#ifndef LE_WARN
#define LE_WARN(format, ...)  printf("WARN | " __FILE__ ", line: %u | " format "\r\n", __LINE__, ##__VA_ARGS__)
#endif

#ifndef LE_ERROR
#define LE_ERROR(format, ...) printf("ERRO | " __FILE__ ", line: %u | " format "\r\n", __LINE__, ##__VA_ARGS__)
#endif

#ifndef LE_CRIT
#define LE_CRIT(format, ...)  printf("CRIT | " __FILE__ ", line: %u | " format "\r\n", __LINE__, ##__VA_ARGS__)
#endif

#ifndef LE_FATAL
#define LE_FATAL(format, ...) do { \
                                  printf("FATAL | " __FILE__ ", line: %u | " format "\r\n", __LINE__, ##__VA_ARGS__); \
                                  abort(); \
                              } while(0)
#endif

#ifndef LE_ASSERT
#define LE_ASSERT(condition)  do { \
                                  if (!condition) LE_FATAL("Assert Failed: '%s'", #condition);\
                              } while(0)
#endif

typedef enum
{
    LE_OK = 0,                  ///< Successful.
    LE_NOT_FOUND = -1,          ///< Referenced item does not exist or could not be found.
    LE_NOT_POSSIBLE = -2,       ///< @deprecated It is not possible to perform the requested action.
    LE_OUT_OF_RANGE = -3,       ///< An index or other value is out of range.
    LE_NO_MEMORY = -4,          ///< Insufficient memory is available.
    LE_NOT_PERMITTED = -5,      ///< Current user does not have permission to perform requested action.
    LE_FAULT = -6,              ///< Unspecified internal error.
    LE_COMM_ERROR = -7,         ///< Communications error.
    LE_TIMEOUT = -8,            ///< A time-out occurred.
    LE_OVERFLOW = -9,           ///< An overflow occurred or would have occurred.
    LE_UNDERFLOW = -10,         ///< An underflow occurred or would have occurred.
    LE_WOULD_BLOCK = -11,       ///< Would have blocked if non-blocking behaviour was not requested.
    LE_DEADLOCK = -12,          ///< Would have caused a deadlock.
    LE_FORMAT_ERROR = -13,      ///< Format error.
    LE_DUPLICATE = -14,         ///< Duplicate entry found or operation already performed.
    LE_BAD_PARAMETER = -15,     ///< Parameter is invalid.
    LE_CLOSED = -16,            ///< The resource is closed.
    LE_BUSY = -17,              ///< The resource is busy.
    LE_UNSUPPORTED = -18,       ///< The underlying resource does not support this operation.
    LE_IO_ERROR = -19,          ///< An IO operation failed.
    LE_NOT_IMPLEMENTED = -20,   ///< Unimplemented functionality.
    LE_UNAVAILABLE = -21,       ///< A transient or temporary loss of a service or resource.
    LE_TERMINATED = -22,        ///< The process, operation, data stream, session, etc. has stopped.
    LE_IN_PROGRESS = -23,       ///< The operation is in progress.
    LE_SUSPENDED = -24,         ///< The operation is suspended.
}
le_result_t;

#endif // LEGATO_H_INCLUDE_GUARD