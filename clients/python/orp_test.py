#============================================================================
#
# Filename:  orp_test.py
#
# Purpose:   Python test script to send and receive HDLC framed commands
#            using the Octave Resource Protocol
#
# MIT License
#
# Copyright (c) 2020 Sierra Wireless Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
#----------------------------------------------------------------------------
#
# NOTES:
#
# 1. Install Python 3.x
#
# 2. Install the following, using pip3:
#
#    Windows:
#    > pip3 install six pythoncrc pyserial
#
#    Linux
#    > sudo -H pip3 install six pythoncrc pyserial
#

import sys
import os
from time import sleep
import argparse

from modules.simple_hdlc import HDLC
from modules.simple_hdlc import __version__ as hdlc_version
import serial
import modules.orp_protocol
from modules.orp_protocol import decode_response
from modules.orp_protocol import encode_request

#
# Function to automatically reply to sync
#
def sync_acknowledge(data):
    if sys.version_info[0] == 2:
        checkPacket = data[0]
    else:
        checkPacket = chr(data[0])

    if orp_protocol.ORP_PKT_SYNC_SYN == checkPacket or orp_protocol.ORP_PKT_SYNC_SYNACK == checkPacket:
        request = 'r y 0'
        packet = encode_request(request)
        h.sendFrame(packet.encode())
        print('\nService Restarted\nConnected\n>')

    if orp_protocol.ORP_PKT_NTFY_HANDLER_CALL == checkPacket or orp_protocol.ORP_PKT_RESP_HANDLER_CALL == checkPacket:
        request = 'r C 0'
        packet = encode_request(request)
        h.sendFrame(packet.encode())
        print('\nAcknowledged push notification\n>')

    if orp_protocol.ORP_PKT_NTFY_SENSOR_CALL == checkPacket or orp_protocol.ORP_PKT_RESP_SENSOR_CALL == checkPacket:
        request = 'r B 0'
        packet = encode_request(request)
        h.sendFrame(packet.encode())
        print('\nAcknowledged sensor notification\n>')

#
# Callback to receive and decode incoming frames
#
def frame_callback(data):
    decode_response(data)
    if auto_ack == True:
        sync_acknowledge(data)

# =========================================================================== #

#
# Require minimum version of simple_hdlc
#
if float(hdlc_version) < 0.3 :
    print('HDLC version: ' + hdlc_version + ' too old.  Minimum 0.3 required. Exiting')
    exit()

#
# Arguments
#
parser = argparse.ArgumentParser(description='Octave Resource Protocol Client Test Script',
                                 usage="orp_test.py [-h] --dev DEV [--b BAUD] [--no-auto-ack]\n"
		                               "Where:\n"
                                       "  DEV is the serial port (e.g. COM3 on Windows or /dev/ttyUSB0 on Linux)\n"
                                       "  BAUD is the baudrate (example 115200, default value is 9600)\n")
parser.add_argument('--dev', help='serial device port (example COM3 on Windows or /dev/ttyUSB0 on Linux)', required=True )
parser.add_argument('--b', help='baudrate of serial port. Example 115200, default value is 9600')
parser.add_argument('--no-auto-ack', help='Disable automatic responses', action='store_false', required=False, default=True)
args = parser.parse_args()

dev=args.dev
baud=9600
auto_ack = True
if args.b:
	baud=args.b
if args.no_auto_ack == False:
    auto_ack = False

#
# UART configuration
#
# Using the default: 8/N/1
s = serial.Serial(port=dev, baudrate=baud)

h = HDLC(s)
h.startReader(onFrame=frame_callback)

print('ORP Serial Client - "q" to exit')
print('using device: ' + dev + ', speed: ' + baud + ', 8N1')
if auto_ack == False:
    print('auto-acknowledgements disabled')
print('enter commands\n')

packet = ''
preamble = '~~'

while True:
    if sys.version_info[0] == 2:
        request = raw_input('> ')
    else:
        request = input('> ')
    if request == 'q':
        print('exiting')
        break

	# remove trailing whitespace
    request = request.rstrip()
    if request != "":
        packet = encode_request(request)

        if packet != None:
            s0 = ord(packet[2])
            s1 = ord(packet[3])
            prestr = 'Sending: ' + packet[0] + packet[1] + str(s0) + str(s1)
            print((prestr + packet[4:75] + '..') if len(packet) > 75  else (prestr + packet[4:]))

			# Wake up the WP UART with a preamble of 0x7E bytes
            s.write(preamble.encode())
            sleep(0.1)
            s.write(preamble.encode())

            h.sendFrame(packet.encode())
            sleep(0.5)

sleep(0.5)
s.close

