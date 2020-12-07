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
#include "orpClient.h"


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
\tpush trig|bool|num|str|json <path> <timestamp> [<data>] (note: if <timestamp> = 0, current timestamp will be used)\n\
\tget <path>\n\
\texample json <path> [<data>]\n\
\treply handler|sensor|sync <status>\n\
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
    ORPCLI_CMD_REPLY,
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
    if (!strncasecmp(cmdStr, "reply", cmdLen))
        return ORPCLI_CMD_REPLY;
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
 * > reply resource| <status>
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
    else if (!strncasecmp(argv[0], "syn", strlen(argv[0])))
    {
        responseType = ORP_SYNC_ACK;
    }
    else if (!strncasecmp(argv[0], "synack", strlen(argv[0])))
    {
        responseType = ORP_SYNC_SYNACK;
    }
    else
    {
        printf("Unknown response type %s\n", argv[0]);
        return;
    }
    (void)orp_Respond(responseType, status);
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
            case ORPCLI_CMD_REPLY:   commandRespond(request); break;
            case ORPCLI_CMD_HELP:    commandHelp(request); break;
            case ORPCLI_CMD_QUIT:    return false;
            default:
                break;
        }
    }
    return true;
}
