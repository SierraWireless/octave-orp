/**
 * @file:    orpUtils.c
 *
 * Purpose:  Utility functions for the Octave Resource Protocol
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

#include <string.h>
#include <stdio.h>
#include "orpUtils.h"
#include "orpFile.h"


//--------------------------------------------------------------------------------------------------
/**
 * Print the fields of an ORP message structure (template)
 */
//--------------------------------------------------------------------------------------------------
void orp_MessagePrint
(
    struct orp_Message *message
)
{
    static struct
    {
        enum orp_PacketType type;
        const char *name;
    } packetNames[] = {
        { ORP_PACKET_TYPE_UNKNOWN, "Unknown packet type" },
        { ORP_RQST_INPUT_CREATE,   "Request, input create" },
        { ORP_RESP_INPUT_CREATE,   "Response, input create" },
        { ORP_RQST_OUTPUT_CREATE,  "Request, output create" },
        { ORP_RESP_OUTPUT_CREATE,  "Response, output create" },
        { ORP_RQST_DELETE,         "Request, delete" },
        { ORP_RESP_DELETE,         "Response, delete" },
        { ORP_RQST_HANDLER_ADD,    "Request, handler add" },
        { ORP_RESP_HANDLER_ADD,    "Response, handler add" },
        { ORP_RQST_HANDLER_REM,    "Request, handler remove" },
        { ORP_RESP_HANDLER_REM,    "Response, handler remove" },
        { ORP_RQST_PUSH,           "Request, push" },
        { ORP_RESP_PUSH,           "Response, push" },
        { ORP_RQST_GET,            "Request, get" },
        { ORP_RESP_GET,            "Response, get" },
        { ORP_RQST_EXAMPLE_SET,    "Request, set example" },
        { ORP_RESP_EXAMPLE_SET,    "Response, set example" },
        { ORP_RQST_SENSOR_CREATE,  "Request, sensor create" },
        { ORP_RESP_SENSOR_CREATE,  "Response, sensor create" },
        { ORP_RQST_SENSOR_REMOVE,  "Request, sensor remove" },
        { ORP_RESP_SENSOR_REMOVE,  "Response, sensor remove" },
        { ORP_NTFY_HANDLER_CALL,   "Notification, handler called" },
        { ORP_RESP_HANDLER_CALL,   "Response, handler called" },
        { ORP_NTFY_SENSOR_CALL,    "Notification, sensor call" },
        { ORP_RESP_SENSOR_CALL,    "Response, sensor call" },
        { ORP_SYNC_SYN,            "Synchronization, sync" },
        { ORP_SYNC_SYNACK,         "Synchronization, sync-ack" },
        { ORP_SYNC_ACK,            "Synchronization, ack" },

        { ORP_RQST_FILE_DATA,      "Request, File transfer data" },
        { ORP_RESP_FILE_DATA,      "Response, File transfer data" },
        { ORP_NTFY_FILE_CONTROL,   "Notification, File transfer control" },
        { ORP_RESP_FILE_CONTROL,   "Response, File transfer control" },

        { ORP_RESP_UNKNOWN_RQST,   "Response, unknown request" },
    };

    const char *statusStr[] = {
        "OK",
        "Item does not exist or could not be found",
        "Not possible to perform the requested action",
        "An index or other value is out of range",
        "Insufficient memory is available",
        "Current user does not have permission to perform requested action",
        "Unspecified internal error",
        "Communications error",
        "A time-out occurred",
        "An overflow occurred or would have occurred",
        "An underflow occurred or would have occurred",
        "Would have blocked if non-blocking behaviour was not requested",
        "Would have caused a deadlock",
        "Format error",
        "Duplicate entry found or operation already performed",
        "Parameter is invalid",
        "The resource is closed",
        "The resource is busy",
        "The underlying resource does not support this operation",
        "An IO operation failed",
        "Unimplemented functionality",
        "A transient or temporary loss of a service or resource",
        "The process, operation, data stream, session, etc. has stopped",
        "The operation is in progress",
        "The operation is suspended",
    };

    const char *packetName = "Unknown";
    for (int i = 0; i < (sizeof(packetNames) / sizeof(packetNames[0])); i++)
    {
        if (message->type == packetNames[i].type)
        {
            packetName = packetNames[i].name;
            break;
        }
    }
    printf("\tType     : %s\n", packetName);
    switch (message->type)
    {
        // Byte[1] is unused on these notification packets
        case ORP_NTFY_HANDLER_CALL:
        case ORP_NTFY_SENSOR_CALL:
            break;

        case ORP_NTFY_FILE_CONTROL:
            printf("\tEvent    : %d\n", (int)message->status);
            break;

        default:
            if (message->type & ORP_RESPONSE_MASK)
            {
                printf("\tStatus   : %d (%s)\n", (int)message->status, statusStr[message->status * -1]);
            }
            else
            {
                printf("\tData type: %d\n", message->dataType);
            }
            break;
    }

    printf("\tSequence : %u\n", message->sequenceNum);
    if (message->timestamp > 0.0)
    {
        printf("\tTimestamp: %lf\n", message->timestamp);
    }
    if (message->path && strlen(message->path))
    {
        printf("\tPath     : %s\n", message->path);
    }
    if (message->data && message->dataLen)
    {
        // In case of file transfer, do not print data which can be binary
        if (message->type != ORP_RQST_FILE_DATA)
        {
            printf("\tData     : %s\n", (char *)message->data);
        }
    }
}
