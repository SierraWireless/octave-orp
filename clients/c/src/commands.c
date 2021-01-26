/**
 * @file:    commands.c
 *
 * Purpose:  Command-line utility to exercise Octave Resource Protocol example
 * client code
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

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include "orpClient.h"
#include "fileTransfer.h"
#include "orpFile.h"

//--------------------------------------------------------------------------------------------------
/**
 * Help message
 */
//--------------------------------------------------------------------------------------------------
const char helpStr[] =
"Syntax:\n\
\thelp\n\
\tquit\n\
\tcreate input|output|sensor  trig|bool|num|str|json <path> [<units>]\n\
\tdelete resource|handler|sensor <path>\n\
\tadd handler <path>\n\
\tpush trig|bool|num|str|json <path> <timestamp> [<data>] (note: if <timestamp> = 0, current timestamp is used)\n\
\tget <path>\n\
\texample json <path> [<data>]\n\
\treply handler|sensor|control|data <status>\n\
\tsync syn|synack|ack [-v] [-s] [-r] [-m]\n\
\tfile control info|ready|pending|suspend|resume|abort [<private data>]\n\
\tfile control start <remote file> [-a <remote file size>] [-f <local file>]\n\
\tfile data [<data>]\n\
";


//--------------------------------------------------------------------------------------------------
/**
 * Command look up and argument parsing
 */
//--------------------------------------------------------------------------------------------------
enum commandTypeE {
    ORPCLI_CMD_HELP,
    ORPCLI_CMD_QUIT,
    ORPCLI_CMD_CREATE,
    ORPCLI_CMD_DELETE,
    ORPCLI_CMD_ADD,
    ORPCLI_CMD_PUSH,
    ORPCLI_CMD_GET,
    ORPCLI_CMD_EXAMPLE,
    ORPCLI_CMD_FILE,
    ORPCLI_CMD_REPLY,
    ORPCLI_CMD_SYNC,
    ORPCLI_CMD_UNKNOWN
};

/* Extract and look up the first word in a command string
 * Case insensitive, matching continues up to the minimum of incomming command length or command
 * name, whichever is shorter
 */
static enum commandTypeE commandExtract(char **line)
{
    char *saveptr = NULL;
    char *cmdStr  = strtok_r(*line, " ", &saveptr);
    size_t cmdLen = strlen(cmdStr);

    *line += cmdLen + 1;

    if (!strncasecmp(cmdStr, "create", cmdLen))
        return ORPCLI_CMD_CREATE;
    if (!strncasecmp(cmdStr, "delete", cmdLen))
        return ORPCLI_CMD_DELETE;
    if (!strncasecmp(cmdStr, "add", cmdLen))
        return ORPCLI_CMD_ADD;
    if (!strncasecmp(cmdStr, "push", cmdLen))
        return ORPCLI_CMD_PUSH;
    if (!strncasecmp(cmdStr, "get", cmdLen))
        return ORPCLI_CMD_GET;
    if (!strncasecmp(cmdStr, "example", cmdLen))
        return ORPCLI_CMD_EXAMPLE;
    if (!strncasecmp(cmdStr, "file", cmdLen))
        return ORPCLI_CMD_FILE;
    if (!strncasecmp(cmdStr, "reply", cmdLen))
        return ORPCLI_CMD_REPLY;
    if (!strncasecmp(cmdStr, "sync", cmdLen))
        return ORPCLI_CMD_SYNC;
    if (!strncasecmp(cmdStr, "help", cmdLen))
        return ORPCLI_CMD_HELP;
    if (!strncasecmp(cmdStr, "quit", cmdLen))
        return ORPCLI_CMD_QUIT;

    printf("Unrecognized command: %s\n", cmdStr);
    return ORPCLI_CMD_UNKNOWN;
}

// Parse a string into an argument array.  Parsing stops at argcMax
static int string2Args(char *str, char *argv[], int argcMax)
{
    char *saveptr = NULL;
    int argc = 0;

    if (str && strlen(str))
    {
        // Stop parsing when the count (argc + 1) reaches the max requested
        for (argv[argc] = strtok_r(str, " ", &saveptr);
             argv[argc] && (++argc < argcMax);
             argv[argc] = strtok_r(NULL, " ", &saveptr));
    }
    return argc;
}

// Check argument count
static bool checkArgCount(int argc, int min, int max)
{
    if ((min < max) && (min <= argc && argc <= max))
    {
        return true;
    }
    else if ((min == max) && (min == argc))
    {
        return true;
    }
    else
    {
        printf("Invalid number of arguments %u\n", argc);
    }
    return false;
}

// Check that a path is non-null and non-zero length
static bool checkPath(char *path)
{
    if (!path || !strlen(path))
    {
        printf("Invalid path argument\n");
        return false;
    }
    return true;
}

// Convert string argument to data type enumeration
static enum orp_IoDataType dataTypeRead(char *dtypeStr)
{
    if (!dtypeStr)
    {
        return ORP_IO_DATA_TYPE_UNDEF;
    }
    switch (tolower(dtypeStr[0]))
    {
        case 't': return ORP_IO_DATA_TYPE_TRIGGER;
        case 'b': return ORP_IO_DATA_TYPE_BOOLEAN;
        case 'n': return ORP_IO_DATA_TYPE_NUMERIC;
        case 's': return ORP_IO_DATA_TYPE_STRING;
        case 'j': return ORP_IO_DATA_TYPE_JSON;
        default:
            printf("Invalid data type: %s\n", dtypeStr);
            return ORP_IO_DATA_TYPE_UNDEF;
    }
}


//--------------------------------------------------------------------------------------------------
/**
 * Individual command handling
 */
//--------------------------------------------------------------------------------------------------
/* Create a resource:
 * > create input|output|sensor  trig|bool|num|str|json <path> [<units>]
 */
static void commandCreate(char *args)
{
    char *argv[6];
    int argc = 0;

    argc = string2Args(args, argv, 6);
    if (!checkArgCount(argc, 3, 4))
    {
        return;
    }
    enum orp_IoDataType dataType = dataTypeRead(argv[1]);
    if (ORP_IO_DATA_TYPE_UNDEF == dataType)
    {
        return;
    }
    char *path = argv[2];
    if (!checkPath(path))
    {
        return;
    }
    // Units (optional)
    char *units = (argv[3] && strlen(argv[3])) ? argv[3] : "";
    // input | output | sensor
    switch (tolower(argv[0][0]))
    {
        case 'i': (void)orp_CreateResource(true, path, dataType, units); break;
        case 'o': (void)orp_CreateResource(false, path, dataType, units); break;
        case 's': (void)orp_CreateSensor(path, dataType, units); break;
        default: printf("Invalid resource type %s\n", argv[0]); break;
    }
}

/* Delete resource || sensor || handler:
 * > delete resource|handler|sensor <path>'
 */
static void commandDelete(char *args)
{
    char *argv[6];
    int argc = 0;

    argc = string2Args(args, argv, 6);
    if (!checkArgCount(argc, 2, 2))
    {
        return;
    }
    char *path = argv[1];
    if (!checkPath(path))
    {
        return;
    }
    // resource | handler | sensor
    switch (tolower(argv[0][0]))
    {
        case 'r': (void)orp_DeleteResource(path); break;
        case 'h': (void)orp_RemovePushHandler(path); break;
        case 's': (void)orp_DestroySensor(path); break;
        default: printf("Unrecognized type: %s\n", argv[0]); break;
    }
}

/* Add a push handler on a resource
 * > add handler <path>
 */
static void commandAdd(char *args)
{
    char *argv[6];
    int argc = 0;

    argc = string2Args(args, argv, 6);
    if (!checkArgCount(argc, 2, 2))
    {
        return;
    }
    char *path = argv[1];
    if (!checkPath(path))
    {
        return;
    }
    if (tolower(argv[0][0]) != 'h')
    {
        printf("Unrecognized type: %s\n", argv[0]);
        return;
    }
    (void)orp_AddPushHandler(path);
}

/* Push value to a resource
 * > push trig|bool|num|str|json <path> <timestamp> [<data>] (note: if <timestamp> = 0, current timestamp will be used)
 */
static void commandPush(char *args)
{
    char *argv[6];
    int argc = 0;
    char *argsEnd = args + strlen(args);

    /* !!! Only parse the first 3 args.  The fourth is data, which may contain
     * spaces.  We deal with the data arg separately, below
     */
    argc = string2Args(args, argv, 3);
    if (!checkArgCount(argc, 3, 3))
    {
        return;
    }
    // Point to the data string, if present
    char *data = NULL;
    argv[argc] = argv[argc - 1] + strlen(argv[argc - 1]) + 1;
    if (argv[argc] < argsEnd)
    {
        data = argv[argc];
    }

    enum orp_IoDataType dataType = dataTypeRead(argv[0]);
    if (ORP_IO_DATA_TYPE_UNDEF == dataType)
    {
        return;
    }
    char *path = argv[1];
    if (!checkPath(path))
    {
        return;
    }
    double timestamp = 0.0;
    if (1 != sscanf(argv[2], "%lf", &timestamp))
    {
        printf("Invalid timestamp %s\n", argv[2]);
        return;
    }
    (void)orp_Push(path, dataType, timestamp, data);
}

/* Get the value from a resource
 * > get <path>
 */
static void commandGet(char *args)
{
    char *argv[6];
    int argc = 0;

    argc = string2Args(args, argv, 6);
    if (!checkArgCount(argc, 1, 1))
    {
        return;
    }
    char *path = argv[0];
    if (!checkPath(path))
    {
        return;
    }
    (void)orp_Get(path);
}

/* Set JSON example
 * > example json <path> [<data>]
 */
static void commandExample(char *args)
{
    char *argv[6];
    int argc = 0;

    argc = string2Args(args, argv, 6);
    if (!checkArgCount(argc, 2, 3))
    {
        return;
    }
    enum orp_IoDataType dataType = dataTypeRead(argv[0]);
    if (ORP_IO_DATA_TYPE_JSON != dataType)
    {
        return;
    }
    char *path = argv[1];
    if (!checkPath(path))
    {
        return;
    }
    char *data = argv[2];
    (void)orp_SetJsonExample(path, data);
}

/* Respond to a notification or unsolicited packet
 * > reply handler|sensor|control|data <status>
 */
static void commandRespond(char *args)
{
    char *argv[6];
    int argc = 0;

    argc = string2Args(args, argv, 6);
    if (!checkArgCount(argc, 1, 2))
    {
        return;
    }
    unsigned int status = 0; // default to OK | Version 1
    if (   (2 == argc)
        && (1 != sscanf(argv[1], "%d", &status)))
    {
        printf("Invalid status %s\n", argv[1]);
        return;
    }
    // handler | sensor | sync
    enum orp_PacketType responseType;
    if (!strncasecmp(argv[0], "handler", strlen(argv[0])))
    {
        responseType = ORP_RESP_HANDLER_CALL;
    }
    else if (!strncasecmp(argv[0], "sensor", strlen(argv[0])))
    {
        responseType = ORP_RESP_SENSOR_CALL;
    }
    else if (!strncasecmp(argv[0], "data", strlen(argv[0])))
    {
        responseType = ORP_RESP_FILE_DATA;
    }
    else if (!strncasecmp(argv[0], "control", strlen(argv[0])))
    {
        responseType = ORP_RESP_FILE_CONTROL;
    }
    else
    {
        printf("Unknown response type %s\n", argv[0]);
        return;
    }
    (void)orp_Respond(responseType, status);
}

/* Send file control or data packet
 * Control:
 * > file control info|ready|pending|suspend|resume|abort [<private data>]
 * > file control start <remote file> [-a <remote file size>] [-f <local file>]
 *
 * Data:
 * > file data <data>
 */
static void commandFileTransfer(char *args)
{
    char *argv[8];
    int argc;
    char *argsEnd = args + strlen(args);
    char *data = NULL;

    /* !!! Only parse the first 2 args.  The 3rd is data, which may contain
     * spaces.  We deal with the data arg separately, below
     */
    argc = string2Args(args, argv, 2);
    if (!checkArgCount(argc, 2, 2))
    {
        return;
    }
    // Point to the data string, if present
    argv[argc] = argv[argc - 1] + strlen(argv[argc - 1]) + 1;
    if (argv[argc] < argsEnd)
    {
        data = argv[argc];
    }

    // Control (notification) packets require an event value for byte[1]
    if ('c' == tolower(argv[0][0]))
    {
        // info | ready | pending | start | suspend | resume | abort
        unsigned int event;
        const char *eventStr = argv[1];
        if (!strncasecmp(eventStr, "info", strlen(eventStr)))
        {
            event = FILETRANSFER_EVENT_INFO;
        }
        else if (!strncasecmp(eventStr, "ready", strlen(eventStr)))
        {
            event = FILETRANSFER_EVENT_READY;
        }
        else if (!strncasecmp(eventStr, "pending", strlen(eventStr)))
        {
            event = FILETRANSFER_EVENT_PENDING;
        }
        else if (!strncasecmp(eventStr, "start", strlen(eventStr)))
        {
            event = FILETRANSFER_EVENT_START;
        }
        else if (!strncasecmp(eventStr, "suspend", strlen(eventStr)))
        {
            event = FILETRANSFER_EVENT_SUSPEND;
        }
        else if (!strncasecmp(eventStr, "resume", strlen(eventStr)))
        {
            event = FILETRANSFER_EVENT_RESUME;
        }
        else if (!strncasecmp(eventStr, "abort", strlen(eventStr)))
        {
            event = FILETRANSFER_EVENT_ABORT;
        }
        else
        {
            printf("Unknown file control event %s\n", eventStr);
            return;
        }

        if (FILETRANSFER_EVENT_START == event)
        {
            // This is a start request, so the format is known.  Extract the remaining args.
            // Start of the remaining string
            args = argv[argc - 1] + strlen(argv[argc - 1]) + 1;
            argc += string2Args(args, argv + argc, 6);
            if (!checkArgCount(argc, 2, 8))
            {
                return;
            }

            // Handle optional arguments for start command
            char filename[FILE_NAME_MAX_LEN] = { '\0' };
            int fileSize = -1;  // -1 Not specified - default
            bool autoAck = false;
            optind = 1;
            opterr = 0;
            int c;
            while ((c = getopt(argc, argv, "f:a:")) != -1)
            {
                switch (c)
                {
                    case 'f': 
                        snprintf(filename, sizeof(filename), "%s", optarg); 
                        break;

                    case 'a':
                        fileSize = (int)strtoul(optarg, NULL, 0);
                        autoAck = true;
                        break;

                    case '?':
                        if (strchr("a", optopt) || strchr("f", optopt))
                        {
                            printf("Option %c requires a value\n", optopt);
                        }
                        else
                        {
                            printf("Unhandled option %c\n", isalnum(optopt) ? optopt : ' ');
                        }
                        return;

                    default: 
                        break;
                }
            }
            // if no filename supplied, use name from remote source
            if (!strlen(filename))
            {
                snprintf(filename, FILE_NAME_MAX_LEN, "%s", data);
            }
            orp_FileDataSetup(filename, fileSize, autoAck);
        }
        (void)orp_FileTransferNotify(event, data);
    }
    else if ('d' == tolower(argv[0][0]))
    {
        (void)orp_FileTransferData(0, data);
    }
    else
    {
        printf("Unrecognized type: %s\n", argv[0]);
    }
}

/* Send one of the SYNC type packets
 * > sync syn|synack [-v <version>] [-s <sent count>] [-r <received count>] [-m <mtu>]
 * > sync ack
 */
static void commandSync(char *args)
{
    char *argv[8];
    int argc = 0;

    int version   =  0;
    int sentCount = -1;  // -1 will not be encoded
    int recvCount = -1;
    int mtu       = -1;

    argc = string2Args(args, argv, 8);
    if (!checkArgCount(argc, 1, 8))
    {
        return;
    }

    // syn | synack | ack
    enum orp_PacketType syncType;
    if (!strncasecmp(argv[0], "syn", strlen(argv[0])))
    {
        syncType = ORP_SYNC_SYN;
    }
    else if (!strncasecmp(argv[0], "synack", strlen(argv[0])))
    {
        syncType = ORP_SYNC_SYNACK;
    }
    else if (!strncasecmp(argv[0], "ack", strlen(argv[0])))
    {
        syncType = ORP_SYNC_ACK;
    }
    else
    {
        printf("Unknown sync type %s\n", argv[0]);
        return;
    }

    optind = 1;
    opterr = 0;
    int c;
    while ((c = getopt(argc, argv, "v:s:r:m:")) != -1)
    {
        switch (c)
        {
            case 'v': version = (int)strtoul(optarg, NULL, 0);   break;
            case 's': sentCount = (int)strtoul(optarg, NULL, 0); break;
            case 'r': recvCount = (int)strtoul(optarg, NULL, 0); break;
            case 'm': mtu = (int)strtoul(optarg, NULL, 0);       break;

            case '?':
            {
                if (strchr("vsrm", optopt))
                {
                    printf("Option %c requires value\n", optopt);
                }
                else
                {
                    printf("Unhandled option %c\n", isalnum(optopt) ? optopt : ' ');
                }
                return;
            }

            default:
                break;
        }
    }

    (void)orp_SyncSend(syncType, version, sentCount, recvCount, mtu);
}

/* > help
 */
static void commandHelp(char *args)
{
    printf(helpStr);
}

bool commandDispatch(char *request)
{
    if (strlen(request) >= 1)
    {
        // The command (first argument) determines how parsing is done on the rest of the line
        switch (commandExtract(&request))
        {
            case ORPCLI_CMD_CREATE:  commandCreate(request); break;
            case ORPCLI_CMD_DELETE:  commandDelete(request); break;
            case ORPCLI_CMD_ADD:     commandAdd(request); break;
            case ORPCLI_CMD_PUSH:    commandPush(request); break;
            case ORPCLI_CMD_GET:     commandGet(request); break;
            case ORPCLI_CMD_EXAMPLE: commandExample(request); break;
            case ORPCLI_CMD_FILE:    commandFileTransfer(request); break;
            case ORPCLI_CMD_REPLY:   commandRespond(request); break;
            case ORPCLI_CMD_SYNC:    commandSync(request); break;
            case ORPCLI_CMD_HELP:    commandHelp(request); break;
            case ORPCLI_CMD_QUIT:    return false;
            default:
                break;
        }
    }
    return true;
}
