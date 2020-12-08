# Octave-ORP

A collection of utilities and information to support the Sierra Wireless Octave Resource Protocol.

The Octave Resource Protocol (ORP) is a simple ASCII-based protocol which enables a remote asset (device) to interact with Octave by exchanging messages with an Octave-enabled edge device.

More information on ORP is provided on the Octave website.  See: https://docs.octave.dev/docs/octave-resource-protocol

## clients

Client code and examples for communicating with an Octave device using the Octave Resource Protocol

### Python

#### orp_test.py

A simple python command-line utilty which can be used to mimic an ORP client, for testing and demonstration.

Depends on:
- clients/python/modules/orp_protocol.py
- clients/python/modules/simple_hdlc.py

Tested on:
- Ubuntu 18.04: Python 2.7, 3.6
- Ubuntu 20.04: Python 2.7, 3.8
- Windows 10: Python 2.7, 3.8

Setup:
1. Install Python 3.x
2. Install the following, using pip3:

   Windows:
   `> pip3 install six pythoncrc pyserial`

   Linux:
   `$ sudo -H pip3 install six pythoncrc pyserial`

Usage:

    python orp_test.py [-h] --dev DEV [--b BAUD] [--no-auto-ack]

Where:

    DEV           : serial port name or path (e.g. COM3 on Windows or /dev/ttyUSB0 on Linux)
    BAUD          : baud rate (default 9600)
    --no-auto-ack : option to disable automatic acknowledgement of sync and notification packets

For a list of supported commands, type "h" at the prompt

### C

#### orp

A simple binary command-line utilty which can be used to mimic an ORP client, for testing and demonstration.

Depends on:
- clients/c/src/*
- clients/c/inc/*

Tested on:
- Ubuntu 20.04

Build:
1. Install gcc
2. cd clients/c
3. make

Usage:

    ./bin/orp -d DEV -b BAUD

Where:

    DEV           : serial port name or path (e.g. /dev/ttyUSB0)
    BAUD          : baud rate (default 9600)

For a list of supported commands, type "h" at the prompt
