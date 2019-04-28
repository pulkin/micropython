/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013-2016 Damien P. George
 * Copyright (c) 2019 pulkin
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/builtin.h"

const char gprs_a9_help_text[] =
"Welcome to MicroPython on the GPRS A9/A9G!\n"
"\n"
"For generic online docs please visit http://docs.micropython.org/\n"
"\n"
"For access to the hardware use the 'machine' module:\n"
"\n"
"import machine\n"
"led = machine.Pin(27, machine.Pin.OUT, 0)\n"
"led.value(1)\n"
"pin25 = machine.Pin(25, machine.Pin.IN, machine.Pin.PULL_UP)\n"
"print(pin25.value())\n"
"i2c = machine.I2C(id=2,freq=400000)\n"
"i2c.scan()\n"
"i2c.writeto(addr, b'1234')\n"
"i2c.readfrom(addr, 4)\n"
"\n"
"Basic GPRS configuration:\n"
"\n"
"import cellular\n"
"cellular.attach()\n"
"cellular.activate(\"access_point\",\"user\",\"password\")\n"
"\n"
"Sockets:\n"
"import usocket\n"
"s = usocket.socket()\n"
"s.connect((\"httpstat.us\", 80))\n"
"s.send(\"GET / HTTP/1.1\\r\\nHost: httpstat.us\\r\\nConnection: close\\r\\n\\r\\n\")\n"
"print(s.recv(16))\n"
"\n"
"Control commands:\n"
"  CTRL-A        -- on a blank line, enter raw REPL mode\n"
"  CTRL-B        -- on a blank line, enter normal REPL mode\n"
"  CTRL-C        -- interrupt a running program\n"
"  CTRL-D        -- on a blank line, do a soft reset of the board\n"
"  CTRL-E        -- on a blank line, enter paste mode\n"
"\n"
"For further help on a specific object, type help(obj)\n"
"For a list of available modules, type help('modules')\n"
;
