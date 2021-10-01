#
# @file:    Makefile
#
# Purpose:  Makefile for example client functions for the Octave Resource Protocol
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

INC_DIR := inc
BUILD_DIR := build
BIN_DIR := bin

CLI_TOOL := orp

CFLAGS = -I$(INC_DIR)

SRCS := main.c commands.c orpProtocol.c hdlc.c at.c orpClient.c orpUtils.c orpFile.c
OBJS := $(addprefix $(BUILD_DIR)/,$(patsubst %.c,%.o,$(SRCS)))


.PHONY:
command_line_tool: clean $(BIN_DIR)/$(CLI_TOOL)

# Directory creation
.PRECIOUS: $(BUILD_DIR)/. $(BIN_DIR)/.

$(BUILD_DIR)/.:
	mkdir -p $@

$(BIN_DIR)/.:
	mkdir -p $@

.SECONDEXPANSION:

# Build
$(BUILD_DIR)/%.o: src/%.c | $$(@D)/.
	$(CC) -c $< -o $@ $(CFLAGS)

$(BIN_DIR)/$(CLI_TOOL): $(OBJS) | $$(@D)/.
	$(CC) $^ -o $@ $(CFLAGS)

# Clean
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
