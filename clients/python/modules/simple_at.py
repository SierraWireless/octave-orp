#!/usr/bin/python
# coding: utf8

# License posted here: https://github.com/wuttem/simple-hdlc/blob/master/LICENSE
# and copied below:
#
# MIT License
#
# Copyright (c) 2016 wuttem

# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

__version__ = '0.3'

import sys
import logging
import time
from threading import Thread

logger = logging.getLogger(__name__)
logger.setLevel(logging.DEBUG)
ch = logging.StreamHandler()
ch.setLevel(logging.INFO)
logger.addHandler(ch)


def bin_to_hex(b):
    if sys.version_info[0] == 2:
        return b.encode("hex")
    return b.hex()


class OrpAtCmdHandler(object):
    def __init__(self, serial, reset=True):
        self.serial = serial
        self.frame_callback = None
        # self.error_callback = None
        self.running = False
        logger.debug("OrpAtCmdHandler init: %s bytes in buffer", self.serial.in_waiting)
        if reset:
            self.serial.reset_input_buffer()

    def sendFrame(self, data):
        bs = self._encode(data)
        logger.info("Sending bytearray: %s", bs)
        logger.debug("Sending Frame: %s", bin_to_hex(bs))
        res = self.serial.write(bs)
        logger.debug("Send %s bytes", res)

    def _onFrame(self, frame):
        logger.debug("Received bytearray: %s", frame)
        logger.debug("Received Frame: %s", bin_to_hex(frame))
        if self.frame_callback is not None:
            self.frame_callback(frame)

    # def _onError(self, frame):
    #     logger.warning("Frame Error: %s", bin_to_hex(frame))
    #     if self.error_callback is not None:
    #         self.error_callback(frame)

    @classmethod
    def _encode(cls, bs):
        return b"AT+ORP=\"" + bs + b"\"\r\n"

    @classmethod
    def _decode(cls, bs):
        if bs.startswith(b"AT+ORP=\""):
            decoded_bs =bs[8:-1]
            logger.debug(f"Decoded echo bytearray: {decoded_bs}")
            return None
        if bs.startswith(b"+ORP: "):
            logger.info("Received reply bytearray: %s", bs)
            decoded_bs =bs[6:]
            logger.debug(f"Decoded reply bytearray: {decoded_bs}")
            return decoded_bs
        logger.debug(f"Failed to decode received bytearray: {bs}")
        return None

    def _receiveLoop(self):
        while self.running:
            i = self.serial.in_waiting
            if i < 1:
                time.sleep(0.001)
                continue
            bs = self.serial.readline()
            logger.debug("Received bytearray: %s", bs)
            decoded_data = self._decode(bs)
            if decoded_data is not None:
                self._onFrame(decoded_data)

    def startReader(self, onFrame, onError=None):
        if self.running:
            raise RuntimeError("reader already running")
        self.reader = Thread(target=self._receiveLoop)
        self.reader.setDaemon(True)
        self.frame_callback = onFrame
        self.error_callback = onError
        self.running = True
        self.reader.start()

    def stopReader(self):
        self.running = False
        self.reader.join()
        self.reader = None
