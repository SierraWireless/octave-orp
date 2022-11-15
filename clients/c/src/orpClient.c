/**
 * @file:    orpClient.c
 *
 * Purpose:  Example client functions for the Octave Resource Protocol
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
 */

#include <unistd.h>
#include <string.h>
#include "orpClient.h"
#include "orpUtils.h"
#include "hdlc.h"
#include "at.h"
#include "legato.h"
#include "orpFile.h"


/* Buffers:
 *
 * The hdlc functions require non-overlapping input and output buffers so we must keep
 * at least one for each of: outbound encoded, outbound framed, inbound framed, and
 * inbound decoded.
 *
 * Sizing:
 *
 * Unframed packets are sized by the data length, plus a fixed component.  The fixed part
 * is defined in orpProtocol.h: ORP_PROTOCOL_LEN_NO_DATA_MAX
 * The data length may be set according to the client requirements but this length must
 * not exceed the maximum data size supported by the Datahub: IO_MAX_STRING_VALUE_LEN
 *
 * HDLC framed packets require space for the framing and crc bytes: HDLC_OVERHEAD_BYTES_COUNT,
 * plus the packet contents with some number of escape bytes.  The upper limit on the
 * escaped contents is: 2 * <max data length>.  If buffer size is a concern, a smaller
 * increase over the max data length is usually enough for real-world data - e.g. 10%
 *
 * This example is written to handle only one outbound and one inbound message at a time,
 * using static buffers
 */

// Max data length.  Setting equal to the Datahub max here
#define ORP_PACKET_DATA_SIZE_MAX    IO_MAX_STRING_VALUE_LEN

// Max size of an unframed request/response packet; including protocol fields:
#define ORP_PACKET_SIZE_MAX         (  ORP_PROTOCOL_LEN_NO_DATA_MAX \
                                     + ORP_PACKET_DATA_SIZE_MAX)

/* Max frame size.  Using a factor of 2 here in order to support stress-testing with all
 * needing to be escaped.  Normally, this isn't necessary
 */
#define ORP_HDLC_FRAME_SIZE_MAX     ((ORP_PACKET_SIZE_MAX * 2) + HDLC_OVERHEAD_BYTES_COUNT)

static uint8_t rxFrameBuf[ORP_HDLC_FRAME_SIZE_MAX];
static uint8_t rxPacketBuf[ORP_PACKET_SIZE_MAX];

static uint8_t txFrameBuf[ORP_HDLC_FRAME_SIZE_MAX];
static uint8_t txPacketBuf[ORP_PACKET_SIZE_MAX];

// ORP encoder/decoder structure, initialized via orp_ProtocolClientInit()
static struct orp_ProtocolCodec codec;

// File descriptor on which to send and receive frames, passed in by caller
static int fd = -1;

// HDLC context
static hdlc_context_t rxHdlcContext;

//--------------------------------------------------------------------------------------------------
/**
 * Initialize local variables and state
 */
//--------------------------------------------------------------------------------------------------
bool orp_ClientInit
(
    int fileDescriptor
)
{
    if (fileDescriptor < 0)
    {
        printf("Invalid file descriptor\n");
        return false;
    }
    if (!orp_ProtocolClientInit(ORP_PROTOCOL_V1, &codec))
    {
        printf("Failed to initialize protocol\n");
        return false;
    }
    printf("Protocol codec initialized\n");
    fd = fileDescriptor;

    if (mode == MODE_HDLC)
    {
        hdlc_Init(&rxHdlcContext);
    }

    return true;
}


//--------------------------------------------------------------------------------------------------
/**
 * Encode a message structure into a packet buffer
 */
//--------------------------------------------------------------------------------------------------
static bool orp_Encode
(
    uint8_t            *packetBuffer,
    size_t             *packetBufferLen,
    struct orp_Message *message
)
{
    return codec.encode(packetBuffer, packetBufferLen, message);
}


//--------------------------------------------------------------------------------------------------
/**
 * Decode a packet buffer into a message structure
 */
//--------------------------------------------------------------------------------------------------
static bool orp_Decode
(
    uint8_t            *packetBuffer,
    size_t              packetBufferLen,
    struct orp_Message *message
)
{
    return codec.decode(packetBuffer, packetBufferLen, message);
}

//--------------------------------------------------------------------------------------------------
/**
 * Frame a packet as AT
 *
 */
//--------------------------------------------------------------------------------------------------
static ssize_t orp_AtEnframe
(
    uint8_t *frameBuf,
    size_t   frameBufSize,
    uint8_t *packet,
    size_t   packetLen
)
{
    int i;
    size_t frameLen;
    size_t count = packetLen;

    memset(frameBuf, 0, frameBufSize);

    // AT mode
    frameLen = at_Pack(frameBuf, frameBufSize, packet, &count);

    return frameLen;
}
//--------------------------------------------------------------------------------------------------
/**
 * Frame a packet as HDLC
 *
 * @note:  The hdlc_Pack routine allows data to be streamed through it, to allow the use of
 * buffers which are smaller than the frame size
 */
//--------------------------------------------------------------------------------------------------
static ssize_t orp_HdlcEnframe
(
    uint8_t *frameBuf,
    size_t   frameBufSize,
    uint8_t *packet,
    size_t   packetLen
)
{
    hdlc_context_t context;
    ssize_t frameLen;
    size_t count = packetLen;

    LE_ASSERT(frameBuf && packet);

    if (frameBufSize < packetLen + HDLC_OVERHEAD_BYTES_COUNT)
    {
        printf("Frame buffer too small (%zu bytes)\n", frameBufSize);
        goto err;
    }

    // Initialize the framing context
    hdlc_Init(&context);
    memset(frameBuf, 0, frameBufSize);

    // Frame the packet
    frameLen = hdlc_Pack(&context, frameBuf, frameBufSize, packet, &count);

    // Check that all of the packet was loaded into the frame buffer
    if (count < packetLen)
    {
        printf("Failed to frame packet (loaded: %zu / %zu)\n", count, packetLen);
        goto err;
    }
    // Check that there is enough room remaining in the buffer to finalize the frame
    // (leading 0x7E already prepended)
    if (frameBufSize - frameLen < (HDLC_OVERHEAD_BYTES_COUNT - 1))
    {
        printf("Frame buffer too small to finalize (used: %zu / %zu)\n", frameLen, frameBufSize);
        goto err;
    }
    count = hdlc_PackFinalize(&context, frameBuf + frameLen, frameBufSize - frameLen);
    if (count < 0)
    {
        printf("Frame buffer too small to finalize (%zu bytes)\n", frameBufSize);
        goto err;
    }
    frameLen += count;

    return frameLen;

err:
    return -1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Simple transmit routine
 */
//--------------------------------------------------------------------------------------------------
static bool orp_Transmit
(
    uint8_t *data,
    size_t  dataLen
)
{
    int rc = -1;

    if (dataLen)
    {
        rc = write(fd, data, dataLen);
    }

    return rc > 0 ? true : false;
}


//--------------------------------------------------------------------------------------------------
/**
 * Convert a message structure to a framed ORP packet and send
 */
//--------------------------------------------------------------------------------------------------
static le_result_t orp_ClientMessageSend
(
    struct orp_Message *message
)
{
    uint8_t *packetBuffer = txPacketBuf;
    size_t   packetBufferLen = sizeof(txPacketBuf);
    uint8_t *frameBuffer = txFrameBuf;
    size_t   frameBufferSize = sizeof(txFrameBuf);

    // Encode the packet
    if (!orp_Encode(packetBuffer, &packetBufferLen, message))
    {
        printf("Failed to encode request\n");
        goto err;
    }

    ssize_t frameLen;
    if (mode == MODE_HDLC)
    {
        // Frame packet
        frameLen = orp_HdlcEnframe(frameBuffer, frameBufferSize, packetBuffer, packetBufferLen);
        if (frameLen < 0)
        {
            printf("Failed to frame packet %zd\n", frameLen);
            goto err;
        }
        printf("Sending:");
        printf(" '%c%c%c%01u%01u%s', (%zu bytes)\n",
            frameBuffer[0], frameBuffer[1], frameBuffer[2], frameBuffer[3], frameBuffer[4], &frameBuffer[5], frameLen);

    }
    else
    {
        frameLen = orp_AtEnframe(frameBuffer, frameBufferSize, packetBuffer, packetBufferLen);
        if (frameLen < 0)
        {
            printf("Failed to frame packet %zd\n", frameLen);
            goto err;
        }
        printf("Sending:");
        printf(" '%s', (%zu bytes)\n", frameBuffer, frameLen);
    }
    orp_MessagePrint(message);

    if (!orp_Transmit(frameBuffer, frameLen))
    {
        printf("Failed to send request\n");
        goto err;
    }

    return LE_OK;

err:
    return LE_FAULT;
}


//--------------------------------------------------------------------------------------------------
/**
 * Handle an incomimg message
 */
//--------------------------------------------------------------------------------------------------
static void orp_Dispatch
(
    struct orp_Message *message
)
{
    // nothing implemented yet
}

//--------------------------------------------------------------------------------------------------
/**
 * Decode AT packets
 *
 * No deframe action for AT packets, so simply print the AT result
 *
 */
//--------------------------------------------------------------------------------------------------
static size_t orp_AtDeframe
(
    uint8_t *frameBuf,
    size_t   frameLen
)
{
    for(int i=0; i<frameLen; i++)
    {
        printf("%c", frameBuf[i]);
    }

    return frameLen;
}


//--------------------------------------------------------------------------------------------------
/**
 * Deframe and decode HDLC packets
 */
//--------------------------------------------------------------------------------------------------
static size_t orp_HdlcDeframe
(
    uint8_t *frameBuf,
    size_t   frameLen
)
{
    static size_t rxPacketLen = 0;
    size_t consumed = 0;
    bool ack = false;


    do
    {
        /* hdlc_Unpack returns a negative value on error.  This will happen if bytes are missed
         * or are corrupt.  When this occurs, decrement the count of received hdlc bytes as usual
         * Reset the deframed buffer as the frame is not useable
         */
        size_t count = frameLen;
        ssize_t hdlcResult = hdlc_Unpack(&rxHdlcContext,
                                         rxPacketBuf + rxPacketLen,
                                         sizeof(rxPacketBuf) - rxPacketLen,
                                         frameBuf, &count);
        frameLen -= count;
        frameBuf += count;
        consumed += count;
        if (hdlcResult < 0)
        {
            printf("Failed to unpack data %zd\n", hdlcResult);
            goto err;
        }

        // Increment the deframed packet length and check for overflow
        rxPacketLen += hdlcResult;
        if (rxPacketLen > sizeof(rxPacketBuf))
        {
            printf("Packet length exceeded %zd\n", rxPacketLen);
            goto err;
        }

        // If a complete frame was NOT unpacked, there is nothing left to do
        if (!hdlc_UnpackDone(&rxHdlcContext))
        {
            break;
        }

        // Decode and process the received packet
        struct orp_Message message;
        bool result = orp_Decode(rxPacketBuf, rxPacketLen, &message);
        if (!result)
        {
            goto err;
        }

        printf("\nReceived:");
        if (message.type != ORP_RQST_FILE_DATA)
        {
            printf(" '%c%c%01X%01X%s', (%zu bytes)",
                   rxPacketBuf[0], rxPacketBuf[1], rxPacketBuf[2], rxPacketBuf[3], &rxPacketBuf[4], rxPacketLen);
        }
        else
        {
            if (message.data && message.dataLen)
            {
                // Auto-ack file transfer data, if using auto mode
                if (orp_FileTransferGetAuto())
                {
                    ack = true;
                }
                orp_FileDataCache(message.data, message.dataLen);
            }

            // In case of file transfer, do not print data (rxPacketBuf[4]) which can be binary
            printf(" '%c%c%01X%01X', (%zu bytes)",
                   rxPacketBuf[0], rxPacketBuf[1], rxPacketBuf[2], rxPacketBuf[3], rxPacketLen);
        }
        printf("\n");
        orp_MessagePrint(&message);

        orp_Dispatch(&message);

        printf("\norp > ");

        // Reset hdlc context and packet length for the next frame
        hdlc_Init(&rxHdlcContext);
        rxPacketLen = 0;

    } while (frameLen > 0);

    if (ack)
    {
        (void)orp_Respond(ORP_RESP_FILE_DATA, 0);
    }

    return consumed;

err:
    hdlc_Init(&rxHdlcContext);
    rxPacketLen = 0;
    return consumed;
}


//--------------------------------------------------------------------------------------------------
/**
 * Reads bytes from the file descriptor, deframes them, and decodes packets. This routine can be
 * called on any number of received bytes - i.e. it is not necessary to wait for a frame boundary.
 * When a full frame has been received, the resulting packet is decoded.
 */
//--------------------------------------------------------------------------------------------------
void orp_ClientReceive
(
    void
)
{
    static size_t rxFrameLen = 0;
    ssize_t count;

    count = read(fd, rxFrameBuf + rxFrameLen, sizeof(rxFrameBuf) - rxFrameLen);
    if (count < 0)
    {
        printf("Failed to receive\n");
    }
    else
    {
        rxFrameLen += count;
        if (MODE_HDLC == mode)
        {
            count = orp_HdlcDeframe(rxFrameBuf, rxFrameLen);
        }
        else
        {
            count = orp_AtDeframe(rxFrameBuf, rxFrameLen);
        }
        rxFrameLen -= count;
        // Shift remaining bytes in the Frame Buffer to the beginning, for processing next time
        if (rxFrameLen > 0)
        {
            memmove(rxFrameBuf, rxFrameBuf + count, rxFrameLen);
        }
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Create a resource in the Data Hub
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_CreateResource
(
    bool isInput,
    const char *path,
    enum orp_IoDataType dataType,
    const char *units
)
{
    struct orp_Message message;
    enum orp_PacketType rqstType = isInput ? ORP_RQST_INPUT_CREATE : ORP_RQST_OUTPUT_CREATE;

    // Copy arguments to the outbound message template.  Buffers will be copied when encoding.
    orp_MessageInit(&message, rqstType, 0);
    message.path = path;           // Resource path
    message.dataType = dataType;   // Resource data type
    message.unit = units;          // Resource units (optional)
    return orp_ClientMessageSend(&message);
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a resource
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_DeleteResource
(
    const char *path
)
{
    struct orp_Message message;

    orp_MessageInit(&message, ORP_RQST_DELETE, 0);
    message.path = path;
    return orp_ClientMessageSend(&message);
}


//--------------------------------------------------------------------------------------------------
/**
 * Register for notifications on a resource
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_AddPushHandler
(
    const char *path
)
{
    struct orp_Message message;

    orp_MessageInit(&message, ORP_RQST_HANDLER_ADD, 0);
    message.path = path;
    return orp_ClientMessageSend(&message);
}


//--------------------------------------------------------------------------------------------------
/**
 * Deregister for notifications on a resource
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_RemovePushHandler
(
    const char *path
)
{
    struct orp_Message message;

    orp_MessageInit(&message, ORP_RQST_HANDLER_REM, 0);
    message.path = path;
    return orp_ClientMessageSend(&message);
}


//--------------------------------------------------------------------------------------------------
/**
 * Push a string-encoded data sample
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_Push
(
    const char *path,
    enum orp_IoDataType dataType,
    double timestampSec,
    const char *value
)
{
    struct orp_Message message;

    orp_MessageInit(&message, ORP_RQST_PUSH, 0);
    message.dataType = dataType;
    message.path = path;
    message.timestamp = timestampSec;
    if (value)
    {
        message.data = (void *)value;
        message.dataLen = strlen(value);
    }
    return orp_ClientMessageSend(&message);
}


//--------------------------------------------------------------------------------------------------
/**
 * Request a string-encoded data sample
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_Get
(
    const char *path
)
{
    struct orp_Message message;

    orp_MessageInit(&message, ORP_RQST_GET, 0);
    message.path = path;
    return orp_ClientMessageSend(&message);
}


//--------------------------------------------------------------------------------------------------
/**
 * Set the example value for a JSON-type Input resource
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_SetJsonExample
(
    const char *path,
    const char *example
)
{
    struct orp_Message message;

    orp_MessageInit(&message, ORP_RQST_EXAMPLE_SET, 0);
    message.path = path;
    message.dataType = ORP_IO_DATA_TYPE_JSON;
    message.data = (void *)example;
    return orp_ClientMessageSend(&message);
}

//--------------------------------------------------------------------------------------------------
/**
 * Create a sensor in the Data Hub
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_CreateSensor
(
    const char *path,
    enum orp_IoDataType dataType,
    const char *units
)
{
    struct orp_Message message;

    orp_MessageInit(&message, ORP_RQST_SENSOR_CREATE, 0);
    message.path = path;
    message.dataType = dataType;
    message.unit = units;
    return orp_ClientMessageSend(&message);
}


//--------------------------------------------------------------------------------------------------
/**
 * Delete a sensor
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_DestroySensor
(
    const char *path
)
{
    struct orp_Message message;

    orp_MessageInit(&message, ORP_RQST_SENSOR_REMOVE, 0);
    message.path = path;
    return orp_ClientMessageSend(&message);
}


//--------------------------------------------------------------------------------------------------
/**
 * Respond to a notification or unsolicited packet
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_Respond
(
    enum orp_PacketType type,
    int status
)
{
    struct orp_Message message;

    switch (type)
    {
        case ORP_RESP_HANDLER_CALL:
            break;

        case ORP_RESP_SENSOR_CALL:
            break;

        case ORP_RESP_FILE_DATA:
            if (LE_OK == status)
            {
                // Data is being accepted, flush to file if required
                orp_FileDataFlush();
            }
            break;

        case ORP_RESP_FILE_CONTROL:
            break;

        default:
            return LE_BAD_PARAMETER;
    }
    orp_MessageInit(&message, type, status);
    return orp_ClientMessageSend(&message);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a sync packet
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_SyncSend
(
    enum orp_PacketType type,
    int version,
    int sentCount,
    int recvCount,
    int mtu
)
{
    struct orp_Message message;

    switch (type)
    {
        case ORP_SYNC_SYN:    break;
        case ORP_SYNC_SYNACK: break;
        case ORP_SYNC_ACK:    break;
        default: return LE_BAD_PARAMETER;
    }

    orp_MessageInit(&message, type, LE_OK);
    message.version = version;
    message.sentCount = sentCount;
    message.receivedCount = recvCount;
    message.mtu = mtu;

    return orp_ClientMessageSend(&message);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send a file transfer notification (a control message)
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_FileTransferNotify
(
    unsigned int status,
    const char *controlData
)
{
    struct orp_Message message;

    orp_MessageInit(&message, ORP_NTFY_FILE_CONTROL, status);
    if (controlData)
    {
        message.data = (void *)controlData;
        message.dataLen = strlen(controlData);
    }
    return orp_ClientMessageSend(&message);
}

//--------------------------------------------------------------------------------------------------
/**
 * Send file transfer data
 */
//--------------------------------------------------------------------------------------------------
le_result_t orp_FileTransferData
(
    unsigned int status,
    const char *fileData
)
{
    struct orp_Message message;

    orp_MessageInit(&message, ORP_RQST_FILE_DATA, status);
    if (fileData)
    {
        message.data = (void *)fileData;
        message.dataLen = strlen(fileData);
    }
    return orp_ClientMessageSend(&message);
}
