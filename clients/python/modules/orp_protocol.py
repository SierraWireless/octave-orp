#============================================================================
#
# Filename:  orp_protocol.py
#
# Purpose:   Python test functions to encode and decode packets for the
#            Octave Resource Protocol
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
import os.path
import sys
import ast
import shlex

#
# Packet type field - byte 0
#
ORP_PKT_RQST_INPUT_CREATE   = 'I'   # type[1] d_type[1] pad[2] path[] units[]
ORP_PKT_RESP_INPUT_CREATE   = 'i'   # type[1] status[1] pad[2]

ORP_PKT_RQST_OUTPUT_CREATE  = 'O'   # type[1] d_type[1] pad[2] path[] units[]
ORP_PKT_RESP_OUTPUT_CREATE  = 'o'   # type[1] status[1] pad[2]

ORP_PKT_RQST_DELETE         = 'D'   # type[1] pad[1]    pad[2] path[]
ORP_PKT_RESP_DELETE         = 'd'   # type[1] status[1] pad[2]

ORP_PKT_RQST_HANDLER_ADD    = 'H'   # type[1] pad[1]    pad[2] path[]
ORP_PKT_RESP_HANDLER_ADD    = 'h'   # type[1] status[1] pad[2]

ORP_PKT_RQST_HANDLER_REMOVE = 'K'   # type[1] pad[1]    pad[2] path[]
ORP_PKT_RESP_HANDLER_REMOVE = 'k'   # type[1] status[1] pad[2]

ORP_PKT_RQST_PUSH           = 'P'   # type[1] d_type[1] pad[2] time[] path[] data[]
ORP_PKT_RESP_PUSH           = 'p'   # type[1] status[1] pad[2]

ORP_PKT_RQST_GET            = 'G'   # type[1] pad[1]    pad[2] path[]
ORP_PKT_RESP_GET            = 'g'   # type[1] status[1] pad[2] time[] data[]

ORP_PKT_RQST_EXAMPLE_SET    = 'E'   # type[1] d_type[1] pad[2] path[] data[]
ORP_PKT_RESP_EXAMPLE_SET    = 'e'   # type[1] status[1] pad[2]

ORP_PKT_RQST_SENSOR_CREATE  = 'S'   # type[1] d_type[1] pad[2] path[] units[]
ORP_PKT_RESP_SENSOR_CREATE  = 's'   # type[1] status[1] pad[2]

ORP_PKT_RQST_SENSOR_REMOVE  = 'R'   # type[1] pad[1]    pad[2] path[]
ORP_PKT_RESP_SENSOR_REMOVE  = 'r'   # type[1] status[1] pad[2]

ORP_PKT_NTFY_HANDLER_CALL   = 'c'   # type[1] d_type[1] pad[2] time[] path[] data[]
ORP_PKT_RESP_HANDLER_CALL   = 'C'   # type[1] status[1] pad[2]

ORP_PKT_NTFY_SENSOR_CALL    = 'b'   # type[1] pad[1]    pad[2] path[]
ORP_PKT_RESP_SENSOR_CALL    = 'B'   # type[1] status[1] pad[2]

# Version 2
ORP_PKT_SYNC_SYN            = 'Y'   # type[1] version[1] unused[2] time[] sent[] received[]
ORP_PKT_SYNC_SYNACK         = 'z'   # type[1] version[1] unused[2] sent[] received[]
ORP_PKT_SYNC_ACK            = 'y'   # type[1] version[1] unused[2]

ORP_PKT_RESP_UNKNOWN_RQST   = '?'   # type[1] status[1] pad[2]


#
# Data type field - byte 1
#
ORP_DATA_TYPE_TRIGGER       = 'T'   # trigger - no data
ORP_DATA_TYPE_BOOLEAN       = 'B'   # Boolean - 1 byte:  't' | 'f'
ORP_DATA_TYPE_NUMERIC       = 'N'   # numeric - null-terminated ASCII string, representing double
ORP_DATA_TYPE_STRING        = 'S'   # string  - null-terminated ASCII string
ORP_DATA_TYPE_JSON          = 'J'   # JSON    - null-terminated ASCII string, representing JSON
ORP_DATA_TYPE_UNDEF         = ' '   # not specified


#
# Variable length field identifiers
#
ORP_FIELD_ID_PATH           = 'P'
ORP_FIELD_ID_TIME           = 'T'
ORP_FIELD_ID_UNITS          = 'U'
ORP_FIELD_ID_DATA           = 'D'

# Variable length field separator
ORP_VARLENGTH_SEPARATOR     = ','



#
# Packet type descriptions
#
ptypes = [
    [ ORP_PKT_RQST_INPUT_CREATE,   'request create input'     ],
    [ ORP_PKT_RESP_INPUT_CREATE,   'response create input'    ],

    [ ORP_PKT_RQST_OUTPUT_CREATE,  'request create output'    ],
    [ ORP_PKT_RESP_OUTPUT_CREATE,  'response create output'   ],

    [ ORP_PKT_RQST_DELETE,         'request delete resource'  ],
    [ ORP_PKT_RESP_DELETE,         'response delete resource' ],

    [ ORP_PKT_RQST_HANDLER_ADD,    'request add handler'      ],
    [ ORP_PKT_RESP_HANDLER_ADD,    'response add handler'     ],

    [ ORP_PKT_RQST_HANDLER_REMOVE, 'request remove handler'   ],
    [ ORP_PKT_RESP_HANDLER_REMOVE, 'response remove handler'  ],

    [ ORP_PKT_RQST_PUSH,           'request push'             ],
    [ ORP_PKT_RESP_PUSH,           'response push'            ],

    [ ORP_PKT_RQST_GET,            'request get'              ],
    [ ORP_PKT_RESP_GET,            'response get'             ],

    [ ORP_PKT_RQST_EXAMPLE_SET,    'request set example'      ],
    [ ORP_PKT_RESP_EXAMPLE_SET,    'response set example'     ],

    [ ORP_PKT_RQST_SENSOR_CREATE,  'request create sensor'    ],
    [ ORP_PKT_RESP_SENSOR_CREATE,  'response create sensor'   ],

    [ ORP_PKT_RQST_SENSOR_REMOVE,  'request remove sensor'    ],
    [ ORP_PKT_RESP_SENSOR_REMOVE,  'response remove sensor'   ],

    [ ORP_PKT_NTFY_HANDLER_CALL,   'handler call'             ],
    [ ORP_PKT_RESP_HANDLER_CALL,   'handler ack'              ],

    [ ORP_PKT_NTFY_SENSOR_CALL,    'sensor poll'              ],
    [ ORP_PKT_RESP_SENSOR_CALL,    'sensor poll ack'          ],

    # Version 2
    [ ORP_PKT_SYNC_SYN,            'sync packet'              ],
    [ ORP_PKT_SYNC_SYNACK,         'sync ack packet'          ],

    [ ORP_PKT_RESP_UNKNOWN_RQST,   'unknown packet type'      ],
]


#
# Status field
#
status_list = [
    'OK',
    'NOT FOUND',
    'NOT POSSIBLE',   # deprecated
    'OUT OF RANGE',
    'NO MEMORY',
    'NOT PERMITTED',
    'FAULT',
    'COMM ERROR',
    'TIMEOUT',
    'OVERFLOW',
    'UNDERFLOW',
    'WOULD BLOCK',
    'DEADLOCK',
    'FORMAT ERROR',
    'DUPLICATE',
    'BAD PARAMETER',
    'CLOSED',
    'BUSY',
    'UNSUPPORTED',
    'IO_ERROR',
    'NOT IMPLEMENTED',
    'UNAVAILABLE',
    'TERMINATED'
]


#
# Data types
#
data_types = [
    [ 'trig',   ORP_DATA_TYPE_TRIGGER ],
    [ 'bool',   ORP_DATA_TYPE_BOOLEAN ],
    [ 'num',    ORP_DATA_TYPE_NUMERIC ],
    [ 'str',    ORP_DATA_TYPE_STRING  ],
    [ 'json',   ORP_DATA_TYPE_JSON    ]
]


#
# Syntax
#
syntax_list = [
    '  create input|output|sensor  trig|bool|num|str|json <path> [<units>]',
    '  delete resource|handler|sensor <path>',
    '  add handler <path>',
    '  push trig|bool|num|str|json <path> <timestamp> [<data>] (note: if <timestamp> = 0, current timestamp will be used)',
    '  get <path>',
    '  example json <path> [<data>]',
    '  reply B|C|y <status>',
    '  send <raw packet content>'
]


#
# Globals
#
sentCount = 0
recvCount = 0

#
# Usage
#
def print_usage():

    print('Usage:')
    for i in range(len(syntax_list)):
        print(syntax_list[i])
    print


#
# Encode data type
#
def encode_dtype(data_type):

    field = ''
    dtype = data_type.lower()

    if   dtype[0] == 't':
        field = field + ORP_DATA_TYPE_TRIGGER

    elif dtype[0] == 'b':
        field = field + ORP_DATA_TYPE_BOOLEAN

    elif dtype[0] == 'n':
        field = field + ORP_DATA_TYPE_NUMERIC

    elif dtype[0] == 's':
        field = field + ORP_DATA_TYPE_STRING

    elif dtype[0] == 'j':
        field = field + ORP_DATA_TYPE_JSON

    else:
        print('Invalid data type')
        return

    return field

#
# Encode sequence number
#
def encode_sequence(count):
    seq0 = count & 0x00ff
    seq1 = count & 0xff00
    seq1 //= 256
    return chr(seq1) + chr(seq0)


#
# Increment local sent count and encode sequence number
#
def increment_encode_sequence():
    global sentCount
    sentCount += 1
    return encode_sequence(sentCount)

sentCount
#
# Encode path
#
def encode_path(path):

    return ORP_FIELD_ID_PATH + path


#
# Encode units
#
def encode_units(units):

    return ORP_FIELD_ID_UNITS + units


#
# Encode data
#
def encode_data(data):

    return ORP_FIELD_ID_DATA + data


#
# Encode timestamp
#
def encode_timestamp(timestamp):

    return ORP_FIELD_ID_TIME + timestamp


#
# create input|output|sensor data-type path [units]
#
def encode_create(argc, args):

    packet = ''
    dtype = ''
    syntax = syntax_list[0]


    if argc < 3 :
        print('Invalid number of arguments')
        print(syntax_list[0])
        return

    if argc > 3 :
        what,data_type,path,units = args.split(' ')

    else:
        what,data_type,path = args.split(' ')
        units = None

    what = what.lower()

    if what[0] == 'i':
        packet = packet + ORP_PKT_RQST_INPUT_CREATE
    elif what[0] == 'o':
        packet = packet + ORP_PKT_RQST_OUTPUT_CREATE
    elif what[0] == 's':
        packet = packet + ORP_PKT_RQST_SENSOR_CREATE
    else:
        print('Invalid request')
        print(syntax_list[0])
        return

    dtype = encode_dtype(data_type)
    if dtype == '':
        return
    packet = packet + dtype
    packet = packet + increment_encode_sequence()
    packet = packet + encode_path(path)
    if units != None:
        packet = packet + ORP_VARLENGTH_SEPARATOR
        packet = packet + encode_units(units)
    return packet


#
# delete resource|handler|sensor path
#
def encode_delete(argc, args):

    packet = ''


    if argc < 2 :
        print('Invalid number of arguments')
        print(syntax_list[1])
        return

    what,path = args.split(' ')
    what = what.lower()

    # packet type
    if what[0] == 'r':
        packet = packet + ORP_PKT_RQST_DELETE
    elif what[0] == 'h':
        packet = packet + ORP_PKT_RQST_HANDLER_REMOVE
    elif what[0] == 's':
        packet = packet + ORP_PKT_RQST_SENSOR_REMOVE
    else:
        print('Invalid request')
        print(syntax_list[1])
        return

    packet = packet + '.'
    packet = packet + increment_encode_sequence()
    packet = packet + encode_path(path)

    return packet


#
# add handler path
#
def encode_add(argc, args):

    packet = ''


    if argc < 2 :
        print('Invalid number of arguments')
        print(syntax_list[2])
        return

    what,path = args.split(' ')
    what = what.lower()

    if what[0] != 'h':
        print('Invalid request ' + what)
        print(syntax_list[2])
        return

    packet = packet + ORP_PKT_RQST_HANDLER_ADD
    # data type - ignored
    packet = packet + '.'
    packet = packet + increment_encode_sequence()
    packet = packet + encode_path(path)

    return packet


#
# push data-type path <timestamp> [<data>]
#
def encode_push(argc, args):

    packet = ''
    dtype = ''

    if argc < 3 :
        print('Invalid number of arguments')
        print(syntax_list[3])
        return

    print('argc: ' + str(argc))
    # all types which require data
    if argc > 3 :
        data_type, path, timestamp, data = args.split(' ', 3)

    # trigger type - no data
    else:
        data_type, path, timestamp = args.split(' ')
        data = ''

    packet = packet + ORP_PKT_RQST_PUSH

    dtype = encode_dtype(data_type)
    if dtype == '':
        return
    packet = str(packet) + str(dtype)

    packet = packet + increment_encode_sequence()
    packet = packet + encode_path(path)

    if timestamp != '0':
        packet = packet + ORP_VARLENGTH_SEPARATOR
        packet = packet + encode_timestamp(timestamp)

    if data != '':
        # This is a bit of a hack:  If data field is actually a path to a
        # file, read the data from the file instead.  Format must be
        #     file://<host>/<path>
        # @Note:  Currently only local host ==  null is supported:
        #     file://<path>
        if data.startswith('file://'):
            path = data.split('file://')[1]
            if os.path.isfile(path):
                try:
                    f = open(path, "r")
                    data = f.read()
                    print("Sending data from file: " + path)
                except FileNotFoundError:
                    print("File not accessible: " + path)
                finally:
                    f.close()
            else:
                print("File does not exist: " + path)

        # append separator for variable length field
        packet = packet + ORP_VARLENGTH_SEPARATOR
        packet = packet + encode_data(data)

    return packet


#
# get path
#
def encode_get(argc, args):

    packet = ''
    dtype = ''

    if argc < 1 :
        print('Invalid number of arguments')
        print(syntax_list[4])
        return

    packet = packet + ORP_PKT_RQST_GET
    # data type ignored
    packet = packet + '.'
    packet = packet + increment_encode_sequence()
    packet = packet + encode_path(args)

    return packet



#
# example data-type path [data]
#
def encode_example(argc, args):

    packet = ''
    dtype = ''

    if argc < 2 :
        print('Invalid number of arguments')
        print(syntax_list[5])
        return

    print('argc: ' + str(argc))
    if argc > 2 :
        data_type,path,data = args.split(' ', 2)

    else:
        data_type,path = args.split(' ')
        data = ''

    packet = packet + ORP_PKT_RQST_EXAMPLE_SET

    dtype = encode_dtype(data_type)
    if dtype == '':
        return
    packet = packet + dtype

    packet = packet + increment_encode_sequence()
    packet = packet + encode_path(path)
    if data != '':
        packet = packet + ORP_VARLENGTH_SEPARATOR
        packet = packet + encode_data(data)

    return packet


#
# acknowledge an asynchronous packet
#
def encode_acknowledge(argc, args):

    packet = ''
    dtype = ''

    if argc < 2 :
        print('Invalid number of arguments')
        print(syntax_list[6])
        return

    what,status = args.split(' ')

    # packet type
    if what[0] == 'y':
        packet = packet + ORP_PKT_SYNC_ACK
    elif what[0] == 'C':
        packet = packet + ORP_PKT_RESP_HANDLER_CALL
    elif what[0] == 'B':
        packet = packet + ORP_PKT_RESP_SENSOR_CALL
    else:
        print('Invalid request')
        print(syntax_list[6])
        return

    packet = packet + status
    packet = packet + increment_encode_sequence()

    return packet


#
# Parse command and build a request packet
#
def encode_request(request):

    if request.find(' ') < 0 :
        print_usage()
        return

    argc = len(request.split(' ')) - 1

    # all commands take at least one argument
    if argc < 1 :
        print_usage()
        return

    request_type,args = request.split(' ', 1)
    request_type = request_type.lower()

    if request_type[0] == 'c':
        p = encode_create(argc, args)

    elif request_type[0] == 'd':
        p = encode_delete(argc, args)

    elif request_type[0] == 'a':
        p = encode_add(argc, args)

    elif request_type[0] == 'p':
        p = encode_push(argc, args)

    elif request_type[0] == 'g':
        p = encode_get(argc, args)

    elif request_type[0] == 'e':
        p = encode_example(argc, args)

    elif request_type[0] == 'r':
        p = encode_acknowledge(argc, args)

    # raw mode
    elif request_type[0] == 's':
        p = ast.literal_eval(shlex.quote(args))

    else:
        print_usage()
        return

    return p


#
# Decode and print contents of an incoming packet
#
def decode_response(response):

    resp = {}

    if sys.version_info[0] == 2:
        # Positional fields:
        ptype      = response[0]
        status_ver = ord(response[1])
        seq_num    = int(response[2:4])
        # Labeled, variable length fields
        var_length = response[4:]
        print('Received     : ' + response)

    else:
        # Positional fields:
        ptype      = chr(response[0])
        status_ver = response[1]
        seq_num    = int(response[2:4])
        # Labeled, variable length fields
        var_length = (response[4:len(response)]).decode("utf-8")
        print('Received     : ' + chr(response[0]) + chr(response[1]) + chr(response[2]) + chr(response[3]) + var_length)

    for i in range(len(ptypes)):
        test = ptypes[i]
        if test[0] == ptype:
            print('Message type : ' + test[1])
            resp['responseType'] = test[0]

    # If this is a SYNC packet, byte 1 contains the version number.  Otherwise, it
    # may contain status
    if ORP_PKT_SYNC_SYN == ptype or ORP_PKT_SYNC_SYNACK == ptype:
        version = chr(status_ver + 1)
        resp['version'] = version
        print('Version      : ' + version)
    else:
        # Status is represented in ASCII, starting with '@' (0x40) for OK.
        # Subtract 0x40 to index into the table, above
        #
        status = status_list[status_ver - 64]
        resp['status'] = status
        print('Status       : ' + status)

    resp['sequence'] = seq_num
    print('Sequence     : ' + str(seq_num))

    if len(var_length):
        var_fields = var_length.split(',')
        for i in range(len(var_fields)):
            field = var_fields[i]
            if field[0] == 'P':
                resp['path'] = field[1:]
                print('Path         : ' + field[1:])
            if field[0] == 'T':
                resp['timestamp'] = field[1:]
                print('Timestamp    : ' + field[1:])
            if field[0] == 'D':
                resp['data'] = field[1:]
                print('Data         : ' + field[1:])
                break
            if field[0] == 'S':
                resp['sent'] = field[1:]
                print('Sent         : ' + field[1:])
            if field[0] == 'R':
                resp['sent'] = field[1:]
                print('Received     : ' + field[1:])

    return resp
