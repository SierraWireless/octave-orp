/**
 * @file:    main.c
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
 * This command-line utility may be used to test and demonstrate the way in
 * which a client (asset) uses the Octave Resource Protocol.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <poll.h>
#include "orpClient.h"


// Individual commands kept in command.c
extern bool commandDispatch(char *request);

// String lengths for some arguments
#define DEV_STR_LEN_MAX   128
#define BAUD_STR_LEN_MAX   32

//--------------------------------------------------------------------------------------------------
/**
 * Usage
 */
//--------------------------------------------------------------------------------------------------
const char usageStr[] =
"Usage:\n\
\tOctave Resource Protocol Client Utility\n\
\tusage: orp [-h] -d DEV [-b BAUD]\n\
\tWhere:\n\
\t  DEV is the serial port (e.g. /dev/ttyUSB0)\n\
\t  BAUD is the baudrate (example 115200, default value is 9600)\n\
";

void usage(void)
{
   printf(usageStr);
}


static char devStr[DEV_STR_LEN_MAX]   = {'\0'};
static char baudStr[BAUD_STR_LEN_MAX] = {'\0'};
static struct pollfd fds[2];

//--------------------------------------------------------------------------------------------------
/**
 * Handle stdin and serial port data from a single thread
 */
//--------------------------------------------------------------------------------------------------
void processIO(void)
{
    /* Unfortunately, the USB-to-Serial converter will fail to send the first packet after a
     * period of inactivity (USB suspend).  Rather than changing the USB behavior, we just
     * keep sending a preamble character to keep the bus from suspending.  Anything less than
     * 5 seconds seems to work
     */
    int timeout_msecs = 3000;

    fds[0].fd = 0; //stdin;
    fds[0].events = POLLIN | POLLHUP;
    fds[1].events = POLLIN | POLLHUP;

    printf("\norp > ");
    fflush(stdout);
    for (bool done = false; !done; )
    {
        if (poll(fds, 2, timeout_msecs) > 0)
        {
            if (fds[0].revents & POLLIN)
            {
                char *line = NULL;
                size_t len = 0;

                len = getline(&line, &len, stdin);
                if (line && len > 0)
                {
                    if ('\n' == line[len - 1])
                        line[len - 1] = '\0';
                    bool keepGoing = commandDispatch(line);
                    free(line);
                    if (!keepGoing)
                    {
                        printf("Exiting\n");
                        break;
                    }
                    printf("\norp > ");
                }
            }
            if (fds[1].revents & POLLIN)
            {
                orp_ClientReceive();
            }
            if (fds[1].revents & POLLHUP)
            {
                printf("Received POLLHUP from %s. Exiting\n", devStr);
                break;
            }
            fflush(stdout);
        }
        // send preamble byte to keep USB awake
        (void)write(fds[1].fd, "~", 1);
    }
}

speed_t baudGet(char *baudStr)
{
    speed_t baud = B0;

    if (0 == strcmp("115200", baudStr))
        baud = B115200;
    else if (0 == strcmp("9600", baudStr))
        baud = B9600;
    else if (0 == strcmp("38400", baudStr))
        baud = B38400;
    else if (0 == strcmp("57600", baudStr))
        baud = B57600;
    else if (0 == strcmp("460800", baudStr))
        baud = B460800;
    else if (0 == strcmp("921600", baudStr))
        baud = B921600;
    return baud;
}

int configureSerial(char *devStr, char *baudStr)
{
    int fd = -1;
    speed_t baud;
    struct termios settings;

    baud = baudGet(baudStr);
    if (B0 == baud)
    {
        printf("Invalid baud rate %s\n", baudStr);
    }
    else
    {
        fd = open(devStr, O_RDWR | O_SYNC /* O_NONBLOCK */);
        if (fd < 0)
        {
            printf("Failed to open %s.  Error %s\n", devStr, strerror(errno));
        }
        else
        {
            tcgetattr(fd, &settings);
            cfmakeraw(&settings);
            cfsetospeed(&settings, baud);

            // 8 bits, No parity, 1 stop bit
            settings.c_cflag &= ~CSIZE;
            settings.c_cflag |= CS8;
            settings.c_cflag &= ~PARENB;
            settings.c_cflag &= ~CSTOPB;

            tcsetattr(fd, TCSANOW, &settings);
            tcflush(fd, TCOFLUSH);
        }
    }
    return fd;
}

int main(int argc, char **argv)
{
    int c;
    int i;
    bool status = false;

    opterr = 0;

    while ((c = getopt(argc, argv, "b:d:h:v")) != -1)
    {
        switch (c)
        {
            case 'b':  // baud rate
                strncpy(baudStr, optarg, sizeof(baudStr));
                break;

            case 'd':  // serial device
                strncpy(devStr, optarg, sizeof(devStr));
                break;

            case '?':
                if (optopt == 'b' || optopt == 'd')
                {
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                    status = true;
                }
                else
                {
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
                usage();
                goto done;

            case 'h':  // help
                status = true;
                usage();
                goto done;

            default:
                usage();
                goto done;
        }
    }

    printf("ORP Serial Client - \"h\" for help, \"q\" to exit\n");
    printf("using device: %s, Baud: %s\n", devStr, baudStr);

    fds[1].fd = configureSerial(devStr, baudStr);
    if (fds[1].fd < 0)
    {
        goto done;
    }
    if (!orp_ClientInit(fds[1].fd))
    {
        goto done;
    }

    processIO();

done:
    if (fds[1].fd > 0)
    {
        close(fds[1].fd);
    }

    exit(status ? EXIT_SUCCESS : EXIT_FAILURE);
}
