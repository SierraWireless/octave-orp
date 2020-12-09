/**
 * @file:    orpProtocol.c
 *
 * Purpose:  Octave Resource Protocol packet encoding and decoding functions
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
 * Refer to the Octave Resource Protocol description here:
 * https://docs.octave.dev/docs/octave-resource-protocol
 *
 */

#include "orpProtocol.h"
#include "legato.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>


#ifndef MIN
#    define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

/* Request/response bytes are chosen to be human-readable (ASCII)
 *
 * Strings must be null terminated.  These also act as the separator,
 * since any other character could appear as data
 *
 * Time represented as ASCII hex chars in ms:
 *
 * For sending via ASCII terminal, <CR> used to indicate "send", so best not to use
 * <CR> as a separator
 *
 * Fixed length fields:
 *
 *   pack_type[1] :
 *   data_type[1] :
 *   pad[2]       : future - to support multi-packet transactions
 *
 * Variable length fields:
 *
 *   path:  P<path chars><separator>     E.g:  P/app/remote/sensor/value<separator>
 *   time:  T<decimal chars><separator>  E.g:  T1541112861.982<separator>
 *   unit:  U<unit chars><separator>     E.g:  UmV<separator>
 *   data:  D<data chars>                E.g:  D{ \"value\" : 123, \"timestamp\" : 1541112861 }
 *
 *   Note: Data may contain the terminator so it must therefore be last
 */
// <source id> <dest id> <trans number> <type> <contents>

#define  ORP_PKT_RQST_INPUT_CREATE   'I'   // type[1] d_type[1] pad[2] path[] units[]
#define  ORP_PKT_RESP_INPUT_CREATE   'i'   // type[1] status[1] pad[2]

#define  ORP_PKT_RQST_OUTPUT_CREATE  'O'   // type[1] d_type[1] pad[2] path[] units[]
#define  ORP_PKT_RESP_OUTPUT_CREATE  'o'   // type[1] status[1] pad[2]

#define  ORP_PKT_RQST_DELETE         'D'   // type[1] pad[1]    pad[2] path[]
#define  ORP_PKT_RESP_DELETE         'd'   // type[1] status[1] pad[2]

#define  ORP_PKT_RQST_HANDLER_ADD    'H'   // type[1] pad[1]    pad[2] path[]
#define  ORP_PKT_RESP_HANDLER_ADD    'h'   // type[1] status[1] pad[2]

#define  ORP_PKT_RQST_HANDLER_REMOVE 'K'   // type[1] pad[1]    pad[2] path[]
#define  ORP_PKT_RESP_HANDLER_REMOVE 'k'   // type[1] status[1] pad[2]

#define  ORP_PKT_RQST_PUSH           'P'   // type[1] d_type[1] pad[2] time[] path[] data[]
#define  ORP_PKT_RESP_PUSH           'p'   // type[1] status[1] pad[2]

#define  ORP_PKT_RQST_GET            'G'   // type[1] pad[1]    pad[2] path[]
#define  ORP_PKT_RESP_GET            'g'   // type[1] status[1] pad[2] time[] data[]

#define  ORP_PKT_RQST_EXAMPLE_SET    'E'   // type[1] d_type[1] pad[2] path[] data[]
#define  ORP_PKT_RESP_EXAMPLE_SET    'e'   // type[1] status[1] pad[2]

#define  ORP_PKT_RQST_SENSOR_CREATE  'S'   // type[1] d_type[1] pad[2] path[] units[]
#define  ORP_PKT_RESP_SENSOR_CREATE  's'   // type[1] status[1] pad[2]

#define  ORP_PKT_RQST_SENSOR_REMOVE  'R'   // type[1] pad[1]    pad[2] path[]
#define  ORP_PKT_RESP_SENSOR_REMOVE  'r'   // type[1] status[1] pad[2]

#define  ORP_PKT_NTFY_HANDLER_CALL   'c'   // type[1] d_type[1] pad[2] time[] path[] data[]
#define  ORP_PKT_RESP_HANDLER_CALL   'C'   // type[1] status[1] pad[2]

#define  ORP_PKT_NTFY_SENSOR_CALL    'b'   // type[1] pad[1]    pad[2] path[]
#define  ORP_PKT_RESP_SENSOR_CALL    'B'   // type[1] status[1] pad[2]

// Version 2
#define  ORP_PKT_SYNC_SYN            'Y'   // type[1] version[1] sequence[2] time[] sent[] received[]
#define  ORP_PKT_SYNC_SYNACK         'y'   // type[1] unused[1]  sequence[2] sent[] received[]
#define  ORP_PKT_SYNC_ACK            'z'   // type[1] unused[1]  sequence[2] sent[] received[]

#define  ORP_PKT_RESP_UNKNOWN_RQST   '?'   // type[1] status[1] pad[2]


// Variable length field separator
#define  ORP_VARLENGTH_SEPARATOR ','   //


// Variable length field identifiers
#define  ORP_FIELD_ID_PATH       'P'   //
#define  ORP_FIELD_ID_TIME       'T'   //
#define  ORP_FIELD_ID_UNITS      'U'   //
#define  ORP_FIELD_ID_DATA       'D'   //
#define  ORP_FIELD_ID_RECV_COUNT 'R'   //
#define  ORP_FIELD_ID_SENT_COUNT 'S'   //


// Data types
#define  ORP_DATA_TYPE_TRIGGER   'T'   // trigger - no data
#define  ORP_DATA_TYPE_BOOLEAN   'B'   // Boolean - 1 byte:  't' | 'f'
#define  ORP_DATA_TYPE_NUMERIC   'N'   // numeric - null-terminated ASCII string, representing double
#define  ORP_DATA_TYPE_STRING    'S'   // string  - null-terminated ASCII string
#define  ORP_DATA_TYPE_JSON      'J'   // JSON    - null-terminated ASCII string, representing JSON
#define  ORP_DATA_TYPE_UNDEF     ' '   // not specified


// Minimum packet length
#define  ORP_PACKET_LEN_MIN       4    //


// Field offsets
#define  ORP_OFFSET_PACKET_TYPE   0    //
#define  ORP_OFFSET_DATA_TYPE     1    //
#define  ORP_OFFSET_SEQ_NUM       2    //
#define  ORP_OFFSET_VARLENGTH     4    //

#define  ORP_OFFSET_STATUS        1    //
#define  ORP_OFFSET_VERSION       1    //


// Field masks
#define ORP_MASK_PACK_TYPE        0x0001
#define ORP_MASK_DATA_TYPE        0x0002
#define ORP_MASK_SEG_NUM          0x0004
#define ORP_MASK_SEG_COUNT        0x0008
#define ORP_MASK_TIME             0x0010
#define ORP_MASK_PATH             0x0020
#define ORP_MASK_DATA             0x0040
#define ORP_MASK_RECV_COUNT       0x0080
#define ORP_MASK_SENT_COUNT       0x0100

#define ORP_MASK_STATUS           0x0200
#define ORP_MASK_VERSION          0x0400


// Status codes:  Use @-Z (0x40-0x5A) and subtract 0x40
enum orp_resp_status
{
    ORP_RESP_STATUS_OK = 0x40,         // LE_OK = 0
    ORP_RESP_STATUS_NOT_FOUND,         // LE_NOT_FOUND = -1
    ORP_RESP_STATUS_NOT_POSSIBLE,      // LE_NOT_POSSIBLE = -2  // DEPRECATED !
    ORP_RESP_STATUS_OUT_OF_RANGE,      // LE_OUT_OF_RANGE = -3
    ORP_RESP_STATUS_NO_MEMORY,         // LE_NO_MEMORY = -4
    ORP_RESP_STATUS_NOT_PERMITTED,     // LE_NOT_PERMITTED = -5
    ORP_RESP_STATUS_FAULT,             // LE_FAULT = -6
    ORP_RESP_STATUS_COMM_ERROR,        // LE_COMM_ERROR = -7
    ORP_RESP_STATUS_TIMEOUT,           // LE_TIMEOUT = -8
    ORP_RESP_STATUS_OVERFLOW,          // LE_OVERFLOW = -9
    ORP_RESP_STATUS_UNDERFLOW,         // LE_UNDERFLOW = -10
    ORP_RESP_STATUS_WOULD_BLOCK,       // LE_WOULD_BLOCK = -11
    ORP_RESP_STATUS_DEADLOCK,          // LE_DEADLOCK = -12
    ORP_RESP_STATUS_FORMAT_ERROR,      // LE_FORMAT_ERROR = -13
    ORP_RESP_STATUS_DUPLICATE,         // LE_DUPLICATE = -14
    ORP_RESP_STATUS_BAD_PARAMETER,     // LE_BAD_PARAMETER = -15
    ORP_RESP_STATUS_CLOSED,            // LE_CLOSED = -16
    ORP_RESP_STATUS_BUSY,              // LE_BUSY = -17
    ORP_RESP_STATUS_UNSUPPORTED,       // LE_UNSUPPORTED = -18
    ORP_RESP_STATUS_IO_ERROR,          // LE_IO_ERROR = -19
    ORP_RESP_STATUS_NOT_IMPLEMENTED,   // LE_NOT_IMPLEMENTED = -20
    ORP_RESP_STATUS_UNAVAILABLE,       // LE_UNAVAILABLE = -21
    ORP_RESP_STATUS_TERMINATED,        // LE_TERMINATED = -22
};


//--------------------------------------------------------------------------------------------------
/**
 * Mapping of encoded to decoded packet types, plus a bitmask of required fields
 */
//--------------------------------------------------------------------------------------------------
static struct
{
    uint8_t             encoded;
    enum orp_PacketType decoded;
    unsigned int        required;
}
orp_PacketTypeTable[] =
{
    { ORP_PKT_RQST_INPUT_CREATE,   ORP_RQST_INPUT_CREATE,   ORP_MASK_DATA_TYPE | ORP_MASK_PATH },
    { ORP_PKT_RESP_INPUT_CREATE,   ORP_RESP_INPUT_CREATE,   ORP_MASK_STATUS                    },

    { ORP_PKT_RQST_OUTPUT_CREATE,  ORP_RQST_OUTPUT_CREATE,  ORP_MASK_DATA_TYPE | ORP_MASK_PATH },
    { ORP_PKT_RESP_OUTPUT_CREATE,  ORP_RESP_OUTPUT_CREATE,  ORP_MASK_STATUS                    },

    { ORP_PKT_RQST_DELETE,         ORP_RQST_DELETE,         ORP_MASK_PATH                      },
    { ORP_PKT_RESP_DELETE,         ORP_RESP_DELETE,         ORP_MASK_STATUS                    },

    { ORP_PKT_RQST_HANDLER_ADD,    ORP_RQST_HANDLER_ADD,    ORP_MASK_PATH                      },
    { ORP_PKT_RESP_HANDLER_ADD,    ORP_RESP_HANDLER_ADD,    ORP_MASK_STATUS                    },

    { ORP_PKT_RQST_HANDLER_REMOVE, ORP_RQST_HANDLER_REM,    ORP_MASK_PATH                      },
    { ORP_PKT_RESP_HANDLER_REMOVE, ORP_RESP_HANDLER_REM,    ORP_MASK_STATUS                    },

    { ORP_PKT_RQST_PUSH,           ORP_RQST_PUSH,           ORP_MASK_DATA_TYPE | ORP_MASK_PATH },
    { ORP_PKT_RESP_PUSH,           ORP_RESP_PUSH,           ORP_MASK_STATUS                    },

    { ORP_PKT_RQST_GET,            ORP_RQST_GET,            ORP_MASK_PATH                      },
    { ORP_PKT_RESP_GET,            ORP_RESP_GET,            ORP_MASK_STATUS                    },

    { ORP_PKT_RQST_EXAMPLE_SET,    ORP_RQST_EXAMPLE_SET,    ORP_MASK_DATA_TYPE | ORP_MASK_PATH },
    { ORP_PKT_RESP_EXAMPLE_SET,    ORP_RESP_EXAMPLE_SET,    ORP_MASK_STATUS                    },

    { ORP_PKT_RQST_SENSOR_CREATE,  ORP_RQST_SENSOR_CREATE,  ORP_MASK_DATA_TYPE | ORP_MASK_PATH },
    { ORP_PKT_RESP_SENSOR_CREATE,  ORP_RESP_SENSOR_CREATE,  ORP_MASK_STATUS                    },

    { ORP_PKT_RQST_SENSOR_REMOVE,  ORP_RQST_SENSOR_REMOVE,  ORP_MASK_PATH                      },
    { ORP_PKT_RESP_SENSOR_REMOVE,  ORP_RESP_SENSOR_REMOVE,  ORP_MASK_STATUS                    },

    { ORP_PKT_NTFY_HANDLER_CALL,   ORP_NTFY_HANDLER_CALL,   ORP_MASK_TIME | ORP_MASK_PATH      },
    { ORP_PKT_RESP_HANDLER_CALL,   ORP_RESP_HANDLER_CALL,   ORP_MASK_STATUS                    },

    { ORP_PKT_NTFY_SENSOR_CALL,    ORP_NTFY_SENSOR_CALL,    ORP_MASK_PATH                      },
    { ORP_PKT_RESP_SENSOR_CALL,    ORP_RESP_SENSOR_CALL,    ORP_MASK_STATUS                    },

    { ORP_PKT_SYNC_SYN,            ORP_SYNC_SYN,     /* no mandatory fields for incomming */   },
    { ORP_PKT_SYNC_SYNACK,         ORP_SYNC_SYNACK,  /* no mandatory fields for incomming */   },
    { ORP_PKT_SYNC_ACK,            ORP_SYNC_ACK,     /* no mandatory fields for incomming */   },

    { ORP_PKT_RESP_UNKNOWN_RQST,   ORP_RESP_UNKNOWN_RQST,   0                                  },

};

#define ORP_PACKET_TYPE_TABLE_SIZE (sizeof(orp_PacketTypeTable) / sizeof(orp_PacketTypeTable[0]))


//--------------------------------------------------------------------------------------------------
/**
 * Mapping encoded to decoded data types
 */
//--------------------------------------------------------------------------------------------------
static struct
{
    uint8_t             encoded;
    enum orp_IoDataType decoded;
}
orp_DataTypeTable[] =
{
    { ORP_DATA_TYPE_TRIGGER,  ORP_IO_DATA_TYPE_TRIGGER, },
    { ORP_DATA_TYPE_BOOLEAN,  ORP_IO_DATA_TYPE_BOOLEAN, },
    { ORP_DATA_TYPE_NUMERIC,  ORP_IO_DATA_TYPE_NUMERIC, },
    { ORP_DATA_TYPE_STRING,   ORP_IO_DATA_TYPE_STRING,  },
    { ORP_DATA_TYPE_JSON,     ORP_IO_DATA_TYPE_JSON,    },
    { ORP_DATA_TYPE_UNDEF,    ORP_IO_DATA_TYPE_UNDEF,   },
};

#define ORP_DATA_TYPE_TABLE_SIZE (sizeof(orp_DataTypeTable) / sizeof(orp_DataTypeTable[0]))


//--------------------------------------------------------------------------------------------------
/**
 * Internal utility to initialize a message structure before decoding into it
 */
//--------------------------------------------------------------------------------------------------
static void orp_MessageInInit
(
    struct orp_Message *msg
)
//--------------------------------------------------------------------------------------------------
{
    static const char * const emptyStr = "";

    memset(msg, 0, sizeof(struct orp_Message));
    msg->type     = ORP_PACKET_TYPE_UNKNOWN;
    msg->dataType = ORP_IO_DATA_TYPE_UNDEF;
    msg->path     = emptyStr;
    msg->unit     = emptyStr;
}


//--------------------------------------------------------------------------------------------------
/**
 * Public utility to initialize a message structure before encoding
 */
//--------------------------------------------------------------------------------------------------
void orp_MessageInit
(
    struct orp_Message  *msg,
    enum orp_PacketType  type,
    unsigned int         status
)
//--------------------------------------------------------------------------------------------------
{
    memset(msg, 0, sizeof(struct orp_Message));
    msg->type = type;
    msg->status = status;
    msg->timestamp = ORP_TIMESTAMP_INVALID;
}


//--------------------------------------------------------------------------------------------------
/**
 * Test whether or not a field is required for a given packet type
 */
//--------------------------------------------------------------------------------------------------
static bool orp_FieldRequired
(
    enum orp_PacketType ptype,
    unsigned int fieldMask
)
//--------------------------------------------------------------------------------------------------
{
    for (unsigned int i = 0; i < ORP_PACKET_TYPE_TABLE_SIZE; i++)
    {
        if (orp_PacketTypeTable[i].decoded == ptype)
        {
            return (orp_PacketTypeTable[i].required & fieldMask) ? true : false;
        }
    }
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode the packet type from a packet buffer
 */
//--------------------------------------------------------------------------------------------------
static bool orp_PacketTypeDecode
(
    uint8_t *buf,
    enum orp_PacketType *ptype
)
//--------------------------------------------------------------------------------------------------
{
    for (unsigned int i = 0; i < ORP_PACKET_TYPE_TABLE_SIZE; i++)
    {
        if (orp_PacketTypeTable[i].encoded == buf[ORP_OFFSET_PACKET_TYPE])
        {
            *ptype = orp_PacketTypeTable[i].decoded;
            return true;
        }
    }
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the packet type into a packet buffer
 */
//--------------------------------------------------------------------------------------------------
static bool orp_PacketTypeEncode
(
    uint8_t *buf,
    enum orp_PacketType ptype
)
//--------------------------------------------------------------------------------------------------
{
    for (unsigned int i = 0; i < ORP_PACKET_TYPE_TABLE_SIZE; i++)
    {
        if (orp_PacketTypeTable[i].decoded == ptype)
        {
            buf[ORP_OFFSET_PACKET_TYPE] = orp_PacketTypeTable[i].encoded;
            return true;
        }
    }
    LE_ERROR("Unrecognized packet type: %d", ptype);
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode the data type from a packet buffer
 */
//--------------------------------------------------------------------------------------------------
static bool orp_DataTypeDecode
(
    uint8_t             *buf,
    enum orp_IoDataType *dtype
)
//--------------------------------------------------------------------------------------------------
{
    *dtype = ORP_IO_DATA_TYPE_UNDEF;
    for (unsigned int i = 0; i < ORP_DATA_TYPE_TABLE_SIZE; i++)
    {
        if (orp_DataTypeTable[i].encoded == buf[ORP_OFFSET_DATA_TYPE])
        {
            *dtype = orp_DataTypeTable[i].decoded;
            return true;
        }
    }
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a data type into a packet buffer
 */
//--------------------------------------------------------------------------------------------------
static bool orp_DataTypeEncode
(
    uint8_t            *buf,
    enum orp_IoDataType dtype
)
//--------------------------------------------------------------------------------------------------
{
    for (unsigned int i = 0; i < ORP_DATA_TYPE_TABLE_SIZE; i++)
    {
        if (orp_DataTypeTable[i].decoded == dtype)
        {
            buf[ORP_OFFSET_DATA_TYPE] = orp_DataTypeTable[i].encoded;
            return true;
        }
    }
    LE_ERROR("Unrecognized data type: %d", dtype);
    return false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the timestamp into a packet buffer
 */
//--------------------------------------------------------------------------------------------------
static size_t orp_TimeEncode
(
    uint8_t *buf,
    size_t   bufLen,
    double   time
)
//--------------------------------------------------------------------------------------------------
{
/* NOTE: There is an error in the ORP service which causes it to reject timestamps containing a
 * decimal point.  This will be corrected in Octave firmware release 3.2.0.
 */
#if 1
#define TIME_FMT "%lu"
    unsigned long sec = time;
#else
#define TIME_FMT "%lf"
    double sec = time;
#endif
    return (time == ORP_TIMESTAMP_INVALID) ? 0 : snprintf((char *)buf, bufLen, "%c" TIME_FMT, ORP_FIELD_ID_TIME, sec);
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode the timestamp from a packet buffer
 */
//--------------------------------------------------------------------------------------------------
static bool orp_TimeDecode
(
    double     *timeVal,
    const char *timeStr
)
//--------------------------------------------------------------------------------------------------
{
    bool result = false;
    bool inDecimal = false;


    if (!timeStr)
    {
        return false;
    }

    int len = strnlen(timeStr, ORP_PROTOCOL_TIMESTAMP_LEN_MAX + 1);
    if (0 == len || len > ORP_PROTOCOL_TIMESTAMP_LEN_MAX)
    {
        LE_ERROR("Invalid length time string: %d", len);
    }
    else
    {
        result = true;
        for (unsigned int i = 0; i < strlen(timeStr); i++)
        {
            if (!isdigit(timeStr[i]))
            {
                // One decimal point is fine, any more is obviously wrong
                if (('.' == timeStr[i]) && (false == inDecimal))
                {
                    inDecimal = true;
                }
                else
                {
                    LE_ERROR("Invalid character in time string: %c", timeStr[i]);
                    result = false;
                    break;
                }
            }
        }
        if (result)
        {
            char *endPtr;
            errno = 0;
            *timeVal = strtod(timeStr, &endPtr);
            if (endPtr == timeStr)
            {
                LE_ERROR("Failed to decode time string: %s", timeStr);
                result = false;
            }
        }
    }

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the path into a protocol buffer
 */
//--------------------------------------------------------------------------------------------------
static size_t orp_PathEncode
(
    uint8_t    *buf,
    size_t      bufLen,
    const char *path
)
//--------------------------------------------------------------------------------------------------
{
    size_t len = 0;


    if (path)
    {
        len = strlen(path);

        // + 1 for ID byte, no null terminator
        if (len + 1 > bufLen)
        {
            LE_FATAL("Insufficient buffer size %lu", (unsigned long)bufLen);
        }

        memmove(buf + 1, path, len);
        *buf = ORP_FIELD_ID_PATH;
        len++;
    }

    return len;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode data into a packet
 */
//--------------------------------------------------------------------------------------------------
static size_t orp_DataEncode
(
    uint8_t *buf,
    size_t   bufLen,
    uint8_t *data,
    size_t   dataLen
)
//--------------------------------------------------------------------------------------------------
{
    if (data && dataLen)
    {
        // Allow encoding of less than dataLen in order to support multi-packet transactions
        dataLen = MIN(bufLen - 1, dataLen);

        // + 1 for ID byte, no null terminator
        memmove(buf + 1, data, dataLen);
        *buf = ORP_FIELD_ID_DATA;
        dataLen++;
    }

    return dataLen;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the status enum into an ascii packet
 */
//--------------------------------------------------------------------------------------------------
static bool orp_StatusEncode
(
    uint8_t     *buf,
    unsigned int status
)
//--------------------------------------------------------------------------------------------------
{
    buf[ORP_OFFSET_STATUS] = (uint8_t)((int)ORP_RESP_STATUS_OK - (int)status);
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the status enum into an ascii packet
 */
//--------------------------------------------------------------------------------------------------
static bool orp_StatusDecode
(
    uint8_t      *buf,
    unsigned int *status
)
//--------------------------------------------------------------------------------------------------
{
    *status = (unsigned int)((int)ORP_RESP_STATUS_OK - (int)buf[ORP_OFFSET_STATUS]);
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the protocol version into an ascii packet
 */
//--------------------------------------------------------------------------------------------------
static bool orp_VersionEncode
(
    uint8_t *buf,
    uint8_t  version
)
//--------------------------------------------------------------------------------------------------
{
    if (version <= 9)
    {
        buf[ORP_OFFSET_VERSION] = '0' + version;
    }
    else if (10 <= version && version <= 15)
    {
        buf[ORP_OFFSET_VERSION] = 'A' + (version - 10);
    }
    else
    {
        return false;
    }
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the second byte of a packet, according to the packet type
 *
 * Request:  data type
 * Response: status
 * Sync:     version number
 *
 * @note: Message type field must be valid
 */
//--------------------------------------------------------------------------------------------------
static bool orp_PacketByte1Encode
(
    uint8_t            *buf,
    struct orp_Message *msg
)
//--------------------------------------------------------------------------------------------------
{
    // SYN | SYNACK | ACK
    if (   (ORP_SYNC_SYN    == msg->type)
        || (ORP_SYNC_SYNACK == msg->type)
        || (ORP_SYNC_ACK    == msg->type))
    {
        if (!orp_VersionEncode(buf, ORP_PROTOCOL_V2))
        {
            return false;
        }
    }
    // Response
    else if (ORP_RESPONSE_MASK & msg->type)
    {
        if (!orp_StatusEncode(buf, msg->status))
        {
            return false;
        }
    }
    // Request
    else
    {
        if (!orp_DataTypeEncode(buf, msg->dataType))
        {
            return false;
        }
    }
    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode the second byte of a packet, according to the packet type
 *
 * Request:  data type
 * Response: status
 * Sync:     version number
 *
 * @note: Message type field must be valid
 */
//--------------------------------------------------------------------------------------------------
static bool orp_PacketByte1Decode
(
    uint8_t            *buf,
    struct orp_Message *msg
)
//--------------------------------------------------------------------------------------------------
{
    bool status = true;

    // SYN | SYNACK | ACK
    if (   (ORP_SYNC_SYN    == msg->type)
        || (ORP_SYNC_SYNACK == msg->type)
        || (ORP_SYNC_ACK    == msg->type))
    {
        LE_ERROR("SYN|SYNACK|ACK byte 1 not decoded");
//            break;
    }
    // Response
    else if (ORP_RESPONSE_MASK & msg->type)
    {
        status = orp_StatusDecode(buf, &msg->status);
    }
    // Request
    else
    {
        // Data Type is not mandatory for all packets
        if (orp_FieldRequired(msg->type, ORP_MASK_DATA_TYPE))
        {
            status = orp_DataTypeDecode(buf, &msg->dataType);
        }
    }

    if (!status)
    {
        LE_ERROR("Failed to decode packet byte 1: %c", buf[1]);
    }
    return status;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the sent count into a protocol buffer
 */
//--------------------------------------------------------------------------------------------------
static size_t orp_SentCountEncode
(
    uint8_t *buf,
    size_t   bufLen,
    int      count
)
//--------------------------------------------------------------------------------------------------
{
    return (count < 0) ? 0 : snprintf((char *)buf, bufLen, "%c%d", ORP_FIELD_ID_SENT_COUNT, count);
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode the received count into a protocol buffer
 */
//--------------------------------------------------------------------------------------------------
static size_t orp_ReceivedCountEncode
(
    uint8_t *buf,
    size_t   bufLen,
    int      count
)
//--------------------------------------------------------------------------------------------------
{
    return (count < 0) ? 0 : snprintf((char *)buf, bufLen, "%c%d", ORP_FIELD_ID_RECV_COUNT, count);
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a request from a buffer formatted according to version 1 of the protocol
 */
//--------------------------------------------------------------------------------------------------
static bool orp_ProtocolDecode_v1
(
    uint8_t            *pktBuf,
    size_t              pktLen,
    struct orp_Message *msg
)
//--------------------------------------------------------------------------------------------------
{
    enum { SEARCH, INFIELD, DONE, ERROR } state = ERROR;
    const char *timeStr = NULL;


    LE_ASSERT(pktBuf && msg);

    do
    {
        if (pktLen < ORP_PACKET_LEN_MIN)
        {
            LE_ERROR("Packet too short: %lu", (unsigned long)pktLen);
            break;
        }

        orp_MessageInInit(msg);

        // Fixed length fields
        if (!orp_PacketTypeDecode(pktBuf, &msg->type))
        {
            LE_ERROR("Failed to decode packet type: %d", msg->type);
            break;
        }
        if (!orp_PacketByte1Decode(pktBuf, msg))
        {
            break;
        }
        msg->sequenceNum = (pktBuf[ORP_OFFSET_SEQ_NUM] << 8) & 0xFF00;
        msg->sequenceNum += pktBuf[ORP_OFFSET_SEQ_NUM + 1] & 0x00FF;

        /* Locate and parse variable length fields
         * Variable length fields must begin with an identifier byte
         */
        state = SEARCH;
        for (unsigned int i = ORP_OFFSET_VARLENGTH;
             i < pktLen && state != DONE && state != ERROR; i++)
        {
            // If separator found, null-terminate current field and scan for next
            if (ORP_VARLENGTH_SEPARATOR == pktBuf[i])
            {
                pktBuf[i] = '\0';
                state = SEARCH;
                continue;
            }

            if (SEARCH == state)
            {
                char *endPtr;

                switch (pktBuf[i])
                {
                    case ORP_FIELD_ID_PATH:
                        msg->path = (const char *)&pktBuf[i + 1];
                        state = INFIELD;
                        break;

                    case ORP_FIELD_ID_TIME:
                        timeStr = (const char *)&pktBuf[i + 1];
                        state = INFIELD;
                        break;

                    case ORP_FIELD_ID_UNITS:
                        msg->unit = (const char *)&pktBuf[i + 1];
                        state = INFIELD;
                        break;

                    case ORP_FIELD_ID_DATA:
                        msg->data = &pktBuf[i + 1];
                        msg->dataLen = pktLen - i - 1;
                        pktBuf[pktLen] = '\0';
                        // Data must be last field - Stop scanning immediately
                        state = DONE;
                        break;

                    case ORP_FIELD_ID_RECV_COUNT:
                        state = INFIELD;
                        errno = 0;
                        msg->receivedCount = strtoul((const char *)&pktBuf[i + 1], &endPtr, 0);
                        if (0 != errno)
                        {
                            LE_ERROR("Failed to decode received count");
                            state = ERROR;
                        }
                        break;

                    case ORP_FIELD_ID_SENT_COUNT:
                        state = INFIELD;
                        errno = 0;
                        msg->sentCount = strtoul((const char *)&pktBuf[i + 1], &endPtr, 0);
                        if (0 != errno)
                        {
                            LE_ERROR("Failed to decode sent count");
                            state = ERROR;
                        }
                        break;

                    default:
                        LE_ERROR("Unknown field identifier pktBuf[%d] = %02X", i, pktBuf[i]);
                        state = ERROR;
                        break;
                }
            }
        }

        /* Ensure that the last byte beyond the reported packet length is null.  This is to
         * null-terminate the last field, as there is no separator at the end.  Doing this will
         * not affect (binary) data as long as the packet size is not incremented
         */
        pktBuf[pktLen] = '\0';

        // Convert the string timestamp to double, if present.  Done here to ensure string is null terminated
        if (timeStr && strlen(timeStr))
        {
            orp_TimeDecode(&msg->timestamp, timeStr);
        }

        if (ERROR == state)
        {
            LE_ERROR("Failed to decode: %d %d %04X %s",
                     msg->type, msg->dataType, msg->sequenceNum,
                     pktBuf + ORP_OFFSET_VARLENGTH);
        }
        else
        {
            LE_DEBUG("Decoded: %u %d %04X path: %s time: %lf unit: %s dataLen: %lu",
                     msg->type, msg->dataType, msg->sequenceNum,
                     msg->path ? msg->path : "",
                     msg->timestamp,
                     msg->unit ? msg->unit : "",
                     (unsigned long)msg->dataLen);
        }

    } while (0);

    return ERROR == state ? false : true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode an outgoing response according to version 1 of the protocol and load it into a buffer
 */
//--------------------------------------------------------------------------------------------------
static bool orp_ProtocolEncode_v1
(
    uint8_t            *packet,
    size_t             *packetLen,
    struct orp_Message *msg
)
//--------------------------------------------------------------------------------------------------
{
    size_t len;
    bool result = false;


    LE_ASSERT(packet && packetLen && msg);

    do
    {
        len = *packetLen;
        if (len < ORP_PACKET_LEN_MIN)
        {
            LE_ERROR("Buffer too short: %zu", len);
            break;
        }

        memset(packet, 0, len);

        // Encode fixed-length fields
        if (!orp_PacketTypeEncode(packet, msg->type))
        {
            break;
        }
        if (!orp_PacketByte1Encode(packet, msg))
        {
            break;
        }
        packet[ORP_OFFSET_SEQ_NUM]     = (msg->sequenceNum & 0x00FF);
        packet[ORP_OFFSET_SEQ_NUM + 1] = (msg->sequenceNum & 0xFF00) >> 8;

        /* Encode variable length fields, starting at ORP_OFFSET_VARLENGTH.
         * Insert separators only as needed
         */
        size_t index = ORP_OFFSET_VARLENGTH;
        size_t fieldLen = 0;

        fieldLen = orp_TimeEncode(packet + index, len - index, msg->timestamp);
        index += fieldLen;

        // Append path if provided.  Note: zero length is permitted
        if (msg->path)
        {
            if (fieldLen)
            {
                packet[index++] = ',';
            }
            fieldLen = orp_PathEncode(packet + index, len - index, msg->path);
            index += fieldLen;
        }

        // Append data if provided.  Zero length will be omitted
        if (msg->dataLen)
        {
            if (fieldLen)
            {
                packet[index++] = ',';
            }
            fieldLen = orp_DataEncode(packet + index, len - index, msg->data, msg->dataLen);
            index += fieldLen;
            msg->dataLen -= fieldLen;
        }

        // Version 2: Append send and received counts if this is a sync packet
        if (ORP_SYNC_SYN == msg->type)
        {
            if (msg->sentCount >= 0)
            {
                if (fieldLen)
                {
                    packet[index++] = ',';
                }
                fieldLen = orp_SentCountEncode(packet + index, len - index, msg->sentCount);
                index += fieldLen;
            }
            if (msg->receivedCount >= 0)
            {
                if (fieldLen)
                {
                    packet[index++] = ',';
                }
                fieldLen = orp_ReceivedCountEncode(packet + index, len - index, msg->receivedCount);
                index += fieldLen;
            }
        }

        *packetLen = index;
        result = true;

    } while (0);

    return result;
}


//--------------------------------------------------------------------------------------------------
/**
 * Initialize protocol interface
 */
//--------------------------------------------------------------------------------------------------
bool orp_ProtocolClientInit
(
    enum orp_ProtocolVersion  version,
    struct orp_ProtocolCodec *codecs
)
//--------------------------------------------------------------------------------------------------
{
    bool status = false;


    LE_ASSERT(codecs);

    switch (version)
    {
        case ORP_PROTOCOL_V1:
        case ORP_PROTOCOL_V2:
            codecs->decode = orp_ProtocolDecode_v1;
            codecs->encode = orp_ProtocolEncode_v1;
            status = true;
            break;

        default:
            break;
    }

    return status;
}
