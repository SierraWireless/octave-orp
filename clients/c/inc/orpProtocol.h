/**
 * @file:    orpProtocol.h
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

#ifndef ORP_PROTOCOL_H_INCLUDE_GUARD
#define ORP_PROTOCOL_H_INCLUDE_GUARD

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>


/* The following defines are taken from the Datahub io.api file for the WP77 familly.  These
 * should be replaced if building in a legato environment, or building to work with a different
 * Octave device.
 */
#if 1
//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of bytes (excluding null terminator) in an I/O resource's path within its
 * namespace in the Data Hub's resource tree.
 */
//--------------------------------------------------------------------------------------------------
#define IO_MAX_RESOURCE_PATH_LEN  79

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of bytes (excluding terminator) in the value of a string type data sample.
 */
//--------------------------------------------------------------------------------------------------
#define IO_MAX_STRING_VALUE_LEN  50000

//--------------------------------------------------------------------------------------------------
/**
 * Maximum number of bytes (excluding terminator) in the units string of a numeric I/O resource.
 */
//--------------------------------------------------------------------------------------------------
#define IO_MAX_UNITS_NAME_LEN  23
#endif // Datahub io.api definitions


//--------------------------------------------------------------------------------------------------
/**
 * Maximum field sizes in the protocol
 */
//--------------------------------------------------------------------------------------------------

/* Maximum combined size of protocol fields, not including the actual resource path,
 * timestamp, data, or units
 *
 * Examples:
 *     Create: <packet type[1]><data type[1]><segment[2]>P<path>[,U<units>]
 *             1 + 1 + 2 + P + ,U      = 7
 *
 *     Push:   <packet type[1]><data type[1]><segment[2]>P<path>[,T<timestamp>][,D<data>]
 *             1 + 1 + 2 + P + ,T + ,D = 9
 */
#define ORP_PROTOCOL_OVERHEAD_LEN_MAX   9

// The maximum sizes of <path>, <units>, and <timestamp>
#define ORP_PROTOCOL_PATH_LEN_MAX       IO_MAX_RESOURCE_PATH_LEN
#define ORP_PROTOCOL_UNITS_LEN_MAX      IO_MAX_UNITS_NAME_LEN
#define ORP_PROTOCOL_TIMESTAMP_LEN_MAX  17  // string representation: 0000000000.000000

// Maximum size of a protocol packet, before accounting for data
#define ORP_PROTOCOL_LEN_NO_DATA_MAX    (  ORP_PROTOCOL_OVERHEAD_LEN_MAX \
                                         + ORP_PROTOCOL_PATH_LEN_MAX \
                                         + ORP_PROTOCOL_UNITS_LEN_MAX \
                                         + ORP_PROTOCOL_TIMESTAMP_LEN_MAX )

// Minimum size which a frame must support
#define ORP_PROTOCOL_FRAME_LEN_MIN      128

//--------------------------------------------------------------------------------------------------
/**
 * Supported protocol versions
 */
//--------------------------------------------------------------------------------------------------
enum orp_ProtocolVersion
{
    ORP_PROTOCOL_V1 = 0,
    ORP_PROTOCOL_V2 = 1,
    ORP_PROTOCOL_COUNT,
};


//--------------------------------------------------------------------------------------------------
/**
 * Packet types
 */
//--------------------------------------------------------------------------------------------------
#define ORP_RESPONSE_MASK 0x80
enum orp_PacketType
{
    ORP_PACKET_TYPE_UNKNOWN = 0,

    ORP_RQST_INPUT_CREATE   = 1,
    ORP_RESP_INPUT_CREATE   = ORP_RQST_INPUT_CREATE  | ORP_RESPONSE_MASK,

    ORP_RQST_OUTPUT_CREATE  = 2,
    ORP_RESP_OUTPUT_CREATE  = ORP_RQST_OUTPUT_CREATE | ORP_RESPONSE_MASK,

    ORP_RQST_DELETE         = 3,
    ORP_RESP_DELETE         = ORP_RQST_DELETE        | ORP_RESPONSE_MASK,

    ORP_RQST_HANDLER_ADD    = 4,
    ORP_RESP_HANDLER_ADD    = ORP_RQST_HANDLER_ADD   | ORP_RESPONSE_MASK,

    ORP_RQST_HANDLER_REM    = 5,
    ORP_RESP_HANDLER_REM    = ORP_RQST_HANDLER_REM   | ORP_RESPONSE_MASK,

    ORP_RQST_PUSH           = 6,
    ORP_RESP_PUSH           = ORP_RQST_PUSH          | ORP_RESPONSE_MASK,

    ORP_RQST_GET            = 7,
    ORP_RESP_GET            = ORP_RQST_GET           | ORP_RESPONSE_MASK,

    ORP_RQST_EXAMPLE_SET    = 8,
    ORP_RESP_EXAMPLE_SET    = ORP_RQST_EXAMPLE_SET   | ORP_RESPONSE_MASK,

    ORP_RQST_SENSOR_CREATE  = 9,
    ORP_RESP_SENSOR_CREATE  = ORP_RQST_SENSOR_CREATE | ORP_RESPONSE_MASK,

    ORP_RQST_SENSOR_REMOVE  = 10,
    ORP_RESP_SENSOR_REMOVE  = ORP_RQST_SENSOR_REMOVE | ORP_RESPONSE_MASK,

    ORP_NTFY_HANDLER_CALL   = 11,
    ORP_RESP_HANDLER_CALL   = ORP_NTFY_HANDLER_CALL  | ORP_RESPONSE_MASK,

    ORP_NTFY_SENSOR_CALL    = 12,
    ORP_RESP_SENSOR_CALL    = ORP_NTFY_SENSOR_CALL   | ORP_RESPONSE_MASK,

    ORP_SYNC_SYN            = 13,
    ORP_SYNC_SYNACK         = 14,
    ORP_SYNC_ACK            = 15,

    ORP_RQST_FILE_DATA      = 16,
    ORP_RESP_FILE_DATA      = ORP_RQST_FILE_DATA     | ORP_RESPONSE_MASK,

    ORP_NTFY_FILE_CONTROL   = 17,
    ORP_RESP_FILE_CONTROL   = ORP_NTFY_FILE_CONTROL  | ORP_RESPONSE_MASK,

    ORP_RESP_UNKNOWN_RQST   = 128                    | ORP_RESPONSE_MASK,
};


//--------------------------------------------------------------------------------------------------
/**
 * Resource data types
 */
//--------------------------------------------------------------------------------------------------
enum orp_IoDataType
{
    ORP_IO_DATA_TYPE_TRIGGER = 0,  ///< trigger
    ORP_IO_DATA_TYPE_BOOLEAN = 1,  ///< Boolean
    ORP_IO_DATA_TYPE_NUMERIC = 2,  ///< numeric (floating point number)
    ORP_IO_DATA_TYPE_STRING  = 3,  ///< string
    ORP_IO_DATA_TYPE_JSON    = 4,  ///< JSON
};

#define  ORP_IO_DATA_TYPE_UNDEF  (-1)


//--------------------------------------------------------------------------------------------------
/**
 * Message structure
 */
//--------------------------------------------------------------------------------------------------
struct orp_Message
{
    enum orp_PacketType         type;          ///< Type of ORP packet
    // TODO - Consider using a union for mutually exclusive fields (minor savings)
    enum orp_IoDataType         dataType;      ///< Data type of resource
    int                         version;       ///< Protocol version (sync packets only)
    int                         status;        ///< Status of a response

    uint16_t                    sequenceNum;   ///< Number of this packet (16-bit rollover)
    double                      timestamp;     ///< Timestamp read/write
    const char                 *path;          ///< Resource path
    const char                 *unit;          ///< Resource units
    void                       *data;          ///< Data (binary permitted)
    size_t                      dataLen;       ///< Data length
    int                         sentCount;     ///< Sent packet count (sync packets only)
    int                         receivedCount; ///< Received packet count (sync packets only)
    int                         mtu;           ///< Maximum transfer unit (sync packets only)
};

#define  ORP_TIMESTAMP_INVALID   ((double)(-1))


//--------------------------------------------------------------------------------------------------
/**
 * Packet decode function:  Packet -> Message structure
 */
//--------------------------------------------------------------------------------------------------
typedef bool (*orp_ProtocolDecode_t)
(
    uint8_t *packetBuffer,             ///< IN : Unframed ORP packet
    size_t   packetLength,             ///< IN : Unframed ORP packet length
    struct orp_Message *request        ///< OUT: Message structure
);


//--------------------------------------------------------------------------------------------------
/**
 * Message encode function:  Message structure -> Packet
 */
//--------------------------------------------------------------------------------------------------
typedef bool (*orp_ProtocolEncode_t)
(
    uint8_t *packetBuffer,             ///< OUT: Unframed ORP packet
    size_t  *packetLength,             ///< IN/OUT: Packet max length / packet length
    struct orp_Message *response       ///< IN : Message structure
);


//--------------------------------------------------------------------------------------------------
/**
 * Structure to access protocol functions
 */
//--------------------------------------------------------------------------------------------------
struct orp_ProtocolCodec
{
    enum orp_ProtocolVersion version;
    orp_ProtocolDecode_t     decode;
    orp_ProtocolEncode_t     encode;
};


//--------------------------------------------------------------------------------------------------
/**
 * Initialize protocol interface
 */
//--------------------------------------------------------------------------------------------------
bool orp_ProtocolClientInit
(
    enum orp_ProtocolVersion  version,
    struct orp_ProtocolCodec *interface
);


//--------------------------------------------------------------------------------------------------
/**
 * Initialize an outbound message
 */
//--------------------------------------------------------------------------------------------------
void orp_MessageInit
(
    struct orp_Message  *message,
    enum orp_PacketType  type,
    unsigned int         status
);

#endif // ORP_PROTOCOL_H_INCLUDE_GUARD
