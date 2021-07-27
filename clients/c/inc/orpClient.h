/**
 * @file:    orpClient.h
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

#ifndef ORP_CLIENT_H_INCLUDE_GUARD
#define ORP_CLIENT_H_INCLUDE_GUARD

#include <stdbool.h>
#include "orpProtocol.h"


//--------------------------------------------------------------------------------------------------
/**
 * Initialze internal data for the client
 *
 * @param:  fileDescriptor: An open file descriptor for reading and writing framed ORP packets
 */
//--------------------------------------------------------------------------------------------------
bool orp_ClientInit
(
    int fileDescriptor
);


//--------------------------------------------------------------------------------------------------
/**
 * Receive and process incoming data on the registered file descriptor
 *
 * @note:  The client does not monitor the file descriptor for incoming data.  It is the
 *         responsibility of the layer above to call this function when data is available
 */
//--------------------------------------------------------------------------------------------------
void orp_ClientReceive
(
    void
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a resource in the Data Hub
 */
//--------------------------------------------------------------------------------------------------
int orp_CreateResource
(
    bool isInput,
    const char *path,
    enum orp_IoDataType dataType,
    const char *units
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete a resource
 */
//--------------------------------------------------------------------------------------------------
int orp_DeleteResource
(
    const char *path
);


//--------------------------------------------------------------------------------------------------
/**
 * Register for notifications on a resource
 */
//--------------------------------------------------------------------------------------------------
int orp_AddPushHandler
(
    const char *path
);


//--------------------------------------------------------------------------------------------------
/**
 * Deregister for notifications on a resource
 */
//--------------------------------------------------------------------------------------------------
int orp_RemovePushHandler
(
    const char *path
);


//--------------------------------------------------------------------------------------------------
/**
 * Push a string-encoded data sample
 */
//--------------------------------------------------------------------------------------------------
int orp_Push
(
    const char *path,
    enum orp_IoDataType dataType,
    double timestamp,
    const char *value
);


//--------------------------------------------------------------------------------------------------
/**
 * Request a string-encoded data sample
 */
//--------------------------------------------------------------------------------------------------
int orp_Get
(
    const char *path
);


//--------------------------------------------------------------------------------------------------
/**
 * Set the example value for a JSON-type Input resource
 */
//--------------------------------------------------------------------------------------------------
int orp_SetJsonExample
(
    const char *path,
    const char *example
);


//--------------------------------------------------------------------------------------------------
/**
 * Create a sensor construct in the Data Hub
 */
//--------------------------------------------------------------------------------------------------
int orp_CreateSensor
(
    const char *path,
    enum orp_IoDataType dataType,
    const char *units
);


//--------------------------------------------------------------------------------------------------
/**
 * Delete a sensor
 */
//--------------------------------------------------------------------------------------------------
int orp_DestroySensor
(
    const char *path
);


//--------------------------------------------------------------------------------------------------
/**
 * Respond to a notification or unsolicited packet
 */
//--------------------------------------------------------------------------------------------------
int orp_Respond
(
    enum orp_PacketType type,
    int status
);


//--------------------------------------------------------------------------------------------------
/**
 * Send a sync packet
 */
//--------------------------------------------------------------------------------------------------
int orp_SyncSend
(
    enum orp_PacketType type,
    int version,
    int sentCount,
    int recvCount,
    int mtu
);


//--------------------------------------------------------------------------------------------------
/**
 * Send a file transfer notification (a control message)
 */
//--------------------------------------------------------------------------------------------------
int orp_FileTransferNotify
(
    unsigned int status,
    const char *controlData
);


//--------------------------------------------------------------------------------------------------
/**
 * Send file transfer data
 */
//--------------------------------------------------------------------------------------------------
int orp_FileTransferData
(
    unsigned int status,
    const char *fileData
);

#endif // ORP_CLIENT_H_INCLUDE_GUARD
