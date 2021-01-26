/**
 * @file:    orpFile.c
 *
 * Purpose:  File transfer utility for the Octave Resource Protocol
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

#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "orpFile.h"
#include "legato.h"


//--------------------------------------------------------------------------------------------------
/**
 * Maximum data to be read
 */
//--------------------------------------------------------------------------------------------------
#define FILE_DATA_MAX_LEN   (100 * 1024)

//--------------------------------------------------------------------------------------------------
/**
 * Static for auto mode
 */
//--------------------------------------------------------------------------------------------------
static int AutoMode = false;

//--------------------------------------------------------------------------------------------------
/**
 * Static for file name
 */
//--------------------------------------------------------------------------------------------------
static char FileName[FILE_NAME_MAX_LEN] = {0};

//--------------------------------------------------------------------------------------------------
/**
 * Static buffer for incoming file data
 */
//--------------------------------------------------------------------------------------------------
static uint8_t IncomingFileData[FILE_DATA_MAX_LEN];

//--------------------------------------------------------------------------------------------------
/**
 * Static for incoming file data length
 */
//--------------------------------------------------------------------------------------------------
static size_t IncomingFileDataLen = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Total bytes received for the current file
 */
//--------------------------------------------------------------------------------------------------
static size_t ReceivedFileBytes = 0;

//--------------------------------------------------------------------------------------------------
/**
 * Total bytes expected for the current file
 */
//--------------------------------------------------------------------------------------------------
static ssize_t ExpectedFileBytes = -1;

//--------------------------------------------------------------------------------------------------
/**
 * Function to set the file name (from the 'file control start/auto <filename>' command)
 */
//--------------------------------------------------------------------------------------------------
static void FileTransferSetName
(
    char* namePtr               ///< [IN] File name
)
{
    int fd;

    if (!namePtr)
    {
        LE_ERROR("Invalid file name");
        return;
    }

    snprintf(FileName, FILE_NAME_MAX_LEN, "%s", namePtr);

    // Check if a file already exists
    fd = open(FileName, O_RDONLY);
    if (fd != -1)
    {
        close(fd);
        // The file already exists, delete it
        unlink(namePtr);
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to write data to the file
 * Each time this function is called, the file is opened/created, updated and closed
 */
//--------------------------------------------------------------------------------------------------
ssize_t FileDataWrite
(
    void*   dataPtr,            ///< [IN] Data pointer
    size_t  dataLen             ///< [IN] Data length
)
{
    if (strlen(FileName) && dataLen && dataPtr)
    {
        // Open the file, create it if it does not exist
        int fd = open(FileName, O_WRONLY | O_APPEND | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
        if (fd == -1)
        {
            perror("Cannot open output file\n");
            return -1;
        }

        ssize_t len = 0;
        while (len < dataLen)
        {
            ssize_t temp = write(fd, dataPtr + len, dataLen - len);
            if (temp == -1)
            {
                printf("Failed to write data: Error %s\n", strerror(errno));
            }
            else
            {
                len += temp;
            }
        }

        if (-1 == close(fd))
        {
            printf("Failed to close file: Error %s\n", strerror(errno));
        }
        return len;
    }

    return -1;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to keep data in RAM before storing it
 * This is used if auto mode is not set
 */
//--------------------------------------------------------------------------------------------------
void FileDataKeep
(
    void*   dataPtr,            ///< [IN] Data pointer
    size_t  dataLen             ///< [IN] Data length
)
{
    if (!dataPtr)
    {
        return;
    }

    memset(IncomingFileData, 0, FILE_DATA_MAX_LEN);
    memcpy(IncomingFileData, dataPtr, dataLen);
    IncomingFileDataLen = dataLen;
}

//--------------------------------------------------------------------------------------------------
/**
 * Function to set the auto mode
 */
//--------------------------------------------------------------------------------------------------
void orp_FileTransferSetAuto
(
    bool isAuto                 ///< [IN] Is auto mode set ?
)
{
    AutoMode = isAuto;
}


//--------------------------------------------------------------------------------------------------
/**
 * Function to check if the auto mode is activated
 */
//--------------------------------------------------------------------------------------------------
bool orp_FileTransferGetAuto
(
    void
)
{
    return AutoMode;
}


//--------------------------------------------------------------------------------------------------
/**
 * Setup data storage for inbound file transfer
 */
//--------------------------------------------------------------------------------------------------
void orp_FileDataSetup
(
    char   *namePtr,
    size_t  fileSize,
    bool    isAuto
)
{
    FileTransferSetName(namePtr);
    AutoMode = isAuto;
    ReceivedFileBytes = 0;
    ExpectedFileBytes = fileSize;

    IncomingFileDataLen = 0;
    memset(IncomingFileData, 0, FILE_DATA_MAX_LEN);
}

//--------------------------------------------------------------------------------------------------
/**
 * Save or cache inbound file data
 */
//--------------------------------------------------------------------------------------------------
void orp_FileDataCache
(
    void   *dataPtr,
    size_t  dataLen
)
{
    ssize_t writtenLen = -1;


    if (AutoMode)
    {
        writtenLen = FileDataWrite(dataPtr, dataLen);
    }
    else
    {
        FileDataKeep(dataPtr, dataLen);
        writtenLen = dataLen;
    }

    if (writtenLen != -1)
    {
        ReceivedFileBytes += dataLen;
    }
    else
    {
        printf("Failed to write data\n");
    }

    // Once all bytes are received, disable auto mode
    if ((ExpectedFileBytes > 0) && (ReceivedFileBytes >= ExpectedFileBytes))
    {
        AutoMode = false;
    }
}

//--------------------------------------------------------------------------------------------------
/**
 * Flush saved data from RAM to the file
 * To be called when the user acks a file data packet.  Does nothing if auto mode is active
 */
//--------------------------------------------------------------------------------------------------
void orp_FileDataFlush
(
    void
)
{
    if (!AutoMode && IncomingFileDataLen)
    {
        FileDataWrite(IncomingFileData, IncomingFileDataLen);
        IncomingFileDataLen = 0;
        memset(IncomingFileData, 0, FILE_DATA_MAX_LEN);
    }
}
