/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * Development of the code in this file was sponsored by Microbric Pty Ltd
 *
 * The MIT License (MIT)
 *
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

// This file re-binds CSDK names to proper uppercases
#include "sdk_init.h"
#include "api_inc_socket.h"

#define LWIP_ACCEPT               CSDK_FUNC(lwip_accept)
#define LWIP_BIND                 CSDK_FUNC(lwip_bind)
#define LWIP_SHUTDOWN             CSDK_FUNC(lwip_shutdown)
#define LWIP_GETPEERNAME          CSDK_FUNC(lwip_getpeername)
#define LWIP_GETSOCKNAME          CSDK_FUNC(lwip_getsockname)
#define LWIP_GETSOCKOPT           CSDK_FUNC(lwip_getsockopt)
#define LWIP_SETSOCKOPT           CSDK_FUNC(lwip_setsockopt)
#define LWIP_CLOSE                CSDK_FUNC(lwip_close)
#define LWIP_CONNECT              CSDK_FUNC(lwip_connect)
#define LWIP_LISTEN               CSDK_FUNC(lwip_listen)
#define LWIP_RECV                 CSDK_FUNC(lwip_recv)
#define LWIP_READ                 CSDK_FUNC(lwip_read)
#define LWIP_RECVFROM             CSDK_FUNC(lwip_recvfrom)
#define LWIP_RECVMSG              CSDK_FUNC(lwip_recvmsg)
#define LWIP_SEND                 CSDK_FUNC(lwip_send)
#define LWIP_SENDMSG              CSDK_FUNC(lwip_sendmsg)
#define LWIP_SENDTO               CSDK_FUNC(lwip_sendto)
#define LWIP_SOCKET               CSDK_FUNC(lwip_socket)
#define LWIP_WRITE                CSDK_FUNC(lwip_write)
#define LWIP_WRITEV               CSDK_FUNC(lwip_writev)
#define LWIP_SELECT               CSDK_FUNC(lwip_select)
#define LWIP_IOCTL                CSDK_FUNC(lwip_ioctl)
#define LWIP_FCNTL                CSDK_FUNC(lwip_fcntl)
#define LWIP_IP6ADDR_NTOA_R       CSDK_FUNC(ip6addr_ntoa_r)
#define LWIP_IP4ADDR_NTOA_R       CSDK_FUNC(ip4addr_ntoa_r)
#define LWIP_IP6ADDR_NTOA         CSDK_FUNC(ip6addr_ntoa_r)
#define LWIP_IP4ADDR_NTOA         CSDK_FUNC(ip4addr_ntoa_r)
#define LWIP_IP6ADDR_ATON         CSDK_FUNC(ip6addr_aton)
#define LWIP_IP4ADDR_ATON         CSDK_FUNC(ip4addr_aton)
#define LWIP_STRERR               CSDK_FUNC(lwip_strerr)
#define LWIP_ERR_TO_ERRNO         CSDK_FUNC(err_to_errno)

#define DNS_GetHostByName2        CSDK_FUNC(DNS_GetHostByName2)

// Presumably, the transformation is symmetric
#define LWIP_HTONS(x) PP_HTONS(x)
#define LWIP_HTONL(x) PP_HTONL(x)
#define LWIP_NTOHS(x) PP_NTOHS(x)
#define LWIP_NTOHL(x) PP_NTOHL(x)

