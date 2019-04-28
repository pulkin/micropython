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

#include "modusocket.h"
#include "modcellular.h"
#include "errno.h"

#include "py/nlr.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/binary.h"
#include "py/objarray.h"
#include "py/objexcept.h"
#include "py/mperrno.h"
#include "py/stream.h"
#include "lib/netutils/netutils.h"

#include "api_network.h"

#include "stdio.h"

#define SOCKET_POLL_US (100000)

uint8_t num_sockets_open = 0;

NORETURN static void exception_from_errno(int _errno) {
    // Here we need to convert from lwip errno values to MicroPython's standard ones
    if (_errno == EINPROGRESS) {
        _errno = MP_EINPROGRESS;
    }
    mp_raise_OSError(_errno);
}

static inline void check_for_exceptions(void) {
    mp_handle_pending();
}

// -------
// Classes
// -------

typedef struct _socket_obj_t {
    mp_obj_base_t base;
    int fd;
    uint8_t domain;
    uint8_t type;
    uint8_t proto;
    bool peer_closed;
    unsigned int retries;
} socket_obj_t;

void _socket_settimeout(socket_obj_t *sock, uint64_t timeout_ms) {
    // Rather than waiting for the entire timeout specified, we wait sock->retries times
    // for SOCKET_POLL_US each, checking for a MicroPython interrupt between timeouts.
    // with SOCKET_POLL_MS == 100ms, sock->retries allows for timeouts up to 13 years.
    // if timeout_ms == UINT64_MAX, wait forever.
    sock->retries = (timeout_ms == UINT64_MAX) ? UINT_MAX : timeout_ms * 1000 / SOCKET_POLL_US;

    struct timeval timeout = {
        .tv_sec = 0,
        .tv_usec = timeout_ms ? SOCKET_POLL_US : 0
    };
    LWIP_SETSOCKOPT(sock->fd, SOL_SOCKET, SO_SNDTIMEO, (const void *)&timeout, sizeof(timeout));
    LWIP_SETSOCKOPT(sock->fd, SOL_SOCKET, SO_RCVTIMEO, (const void *)&timeout, sizeof(timeout));
    LWIP_FCNTL(sock->fd, F_SETFL, timeout_ms ? 0 : O_NONBLOCK);
}

static int _socket_getaddrinfo2(const mp_obj_t host, const mp_obj_t port, struct sockaddr_in *resp) {

    const char *host_str = mp_obj_str_get_str(host);
    const int port_int = mp_obj_get_int(port);

    if (host_str[0] == '\0') {
        // a host of "" is equivalent to the default/all-local IP address
        host_str = "0.0.0.0";
    }

    char address[16];
    if (DNS_GetHostByName2((uint8_t*)host_str, (uint8_t*)address) != 0) {
        return -1;
    }

    memset(resp, 0, sizeof(*resp));
    resp->sin_family = AF_INET;
    resp->sin_port = LWIP_HTONS(port_int);

    MP_THREAD_GIL_EXIT();
    int res = LWIP_IP4ADDR_ATON(address, (ip4_addr_t*)&resp->sin_addr);
    MP_THREAD_GIL_ENTER();

    return res;
}

int _socket_getaddrinfo(const mp_obj_t addrtuple, struct sockaddr_in *resp) {
    mp_uint_t len = 0;
    mp_obj_t *elem;
    mp_obj_get_array(addrtuple, &len, &elem);
    if (len != 2) return -1;
    return _socket_getaddrinfo2(elem[0], elem[1], resp);
}

mp_obj_t socket_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {

    enum { ARG_af, ARG_type, ARG_proto };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_af, MP_ARG_INT, {.u_int = AF_INET} },
        { MP_QSTR_type, MP_ARG_INT, {.u_int = SOCK_STREAM} },
        { MP_QSTR_proto, MP_ARG_INT, {.u_int = IPPROTO_TCP} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    socket_obj_t *self = m_new_obj_with_finaliser(socket_obj_t);
    self->base.type = type;
    self->peer_closed = false;

    switch (args[ARG_af].u_int) {
        case AF_INET:
            self->domain = AF_INET;
            break;
        case AF_INET6:
            self->domain = AF_INET6;
            break;
        default:
            mp_raise_ValueError("Unknown 'af' argument value");
            break;
    }

    switch (args[ARG_type].u_int) {
        case SOCK_STREAM:
            self->type = SOCK_STREAM;
            break;
        case SOCK_DGRAM:
            self->type = SOCK_DGRAM;
            break;
        default:
            mp_raise_ValueError("Unknown 'type' argument");
            break;
    }

    switch (args[ARG_proto].u_int) {
        case IPPROTO_TCP:
            self->proto = IPPROTO_TCP;
            break;
        case IPPROTO_UDP:
            self->proto = IPPROTO_UDP;
            break;
        default:
            mp_raise_ValueError("Unknown protocol");
            return mp_const_none;
    }

    self->fd = LWIP_SOCKET(self->domain, self->type, self->proto);
    if (self->fd < 0) {
        mp_raise_NotImplementedError("Failed to create the socket but the error is unknown");
    }
    num_sockets_open ++;
    _socket_settimeout(self, UINT64_MAX);

    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t socket_bind(mp_obj_t self_in, mp_obj_t address) {
    // ========================================
    // Binds the socket.
    // Args:
    //     address (tuple): address to bind to;
    // ========================================
    mp_raise_NotImplementedError("Server capabilities are not implemented yet");
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_bind_obj, &socket_bind);

STATIC mp_obj_t socket_listen(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Sets the socket to listen for incoming connections.
    // Args:
    //     backlog (int): the number of unaccepted connections
    //     that the system will allow before refusing new connections.
    // ========================================
    mp_raise_NotImplementedError("Server capabilities are not implemented yet");
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socket_listen_obj, 1, 2, &socket_listen);

STATIC mp_obj_t socket_accept(mp_obj_t self_in) {
    // ========================================
    // Accepts the connection.
    // ========================================
    mp_raise_NotImplementedError("Server capabilities are not implemented yet");
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_1(socket_accept_obj, &socket_accept);

STATIC mp_obj_t socket_connect(mp_obj_t self_in, mp_obj_t ipv4) {
    // ========================================
    // Connects.
    // Args:
    //     ipv4 (tuple): a tuple of (address, port);
    // ========================================
    socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    struct sockaddr_in res;
    _socket_getaddrinfo(ipv4, &res);

    MP_THREAD_GIL_EXIT();
    int r = LWIP_CONNECT(self->fd, (struct sockaddr*)&res, sizeof(struct sockaddr_in));
    MP_THREAD_GIL_ENTER();

    if (r < 0) {
        exception_from_errno(LWIP_ERRNO());
    }

    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_connect_obj, &socket_connect);

int _socket_send(socket_obj_t *sock, const char *data, size_t datalen) {
    int sentlen = 0;
    for (int i=0; i<=sock->retries && sentlen < datalen; i++) {

        MP_THREAD_GIL_EXIT();
        int r = LWIP_WRITE(sock->fd, data + sentlen, datalen - sentlen);
        MP_THREAD_GIL_ENTER();

        int errno = LWIP_ERRNO();
        if (r < 0 && errno != EWOULDBLOCK) exception_from_errno(errno);
        if (r > 0) sentlen += r;
        check_for_exceptions();
    }
    if (sentlen == 0) mp_raise_OSError(MP_ETIMEDOUT);
    return sentlen;
}

STATIC mp_obj_t socket_send(mp_obj_t self_in, mp_obj_t bytes) {
    // ========================================
    // Sends bytes.
    // Args:
    //     bytes (str, bytearray): bytes to send;
    // ========================================
    socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(bytes, &bufinfo, MP_BUFFER_READ);
    int r = _socket_send(self, bufinfo.buf, bufinfo.len);

    return mp_obj_new_int(r);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_send_obj, &socket_send);

STATIC mp_obj_t socket_sendall(mp_obj_t self_in, mp_obj_t bytes) {
    // ========================================
    // Sends all bytes chunk by chunk.
    // Args:
    //     bytes (str, bytearray): bytes to send;
    // ========================================
    socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(bytes, &bufinfo, MP_BUFFER_READ);
    int r = _socket_send(self, bufinfo.buf, bufinfo.len);
    if (r < bufinfo.len) mp_raise_OSError(MP_ETIMEDOUT);
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_sendall_obj, &socket_sendall);

// XXX this can end up waiting a very long time if the content is dribbled in one character
// at a time, as the timeout resets each time a recvfrom succeeds ... this is probably not
// good behaviour.
STATIC mp_uint_t _socket_read_data(mp_obj_t self_in, void *buf, size_t size, struct sockaddr *from, socklen_t *from_len, int *errcode) {
    socket_obj_t *sock = MP_OBJ_TO_PTR(self_in);

    // If the peer closed the connection then the lwIP socket API will only return "0" once
    // from lwip_recvfrom_r and then block on subsequent calls.  To emulate POSIX behaviour,
    // which continues to return "0" for each call on a closed socket, we set a flag when
    // the peer closed the socket.
    if (sock->peer_closed) {
        return 0;
    }

    // XXX Would be nicer to use RTC to handle timeouts
    for (int i = 0; i <= sock->retries; ++i) {

        MP_THREAD_GIL_EXIT();
        int r = LWIP_RECVFROM(sock->fd, buf, size, 0, from, from_len);
        MP_THREAD_GIL_ENTER();

        if (r == 0) sock->peer_closed = true;
        if (r >= 0) return r;
        int errno = LWIP_ERRNO();
        if (errno != EWOULDBLOCK) {
            *errcode = errno;
            return MP_STREAM_ERROR;
        }
        check_for_exceptions();
    }

    *errcode = sock->retries == 0 ? MP_EWOULDBLOCK : MP_ETIMEDOUT;
    return MP_STREAM_ERROR;
}

mp_obj_t _socket_recvfrom(mp_obj_t self_in, mp_obj_t len_in, struct sockaddr *from, socklen_t *from_len) {
    size_t len = mp_obj_get_int(len_in);
    vstr_t vstr;
    vstr_init_len(&vstr, len);

    int errcode;
    mp_uint_t r = _socket_read_data(self_in, vstr.buf, len, from, from_len, &errcode);
    if (r == MP_STREAM_ERROR) {
        exception_from_errno(errcode);
    }

    vstr.len = r;
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}

STATIC mp_obj_t socket_recv(mp_obj_t self_in, mp_obj_t bufsize) {
    // ========================================
    // Receives bytes.
    // Args:
    //     bufsize (int): output array size;
    // ========================================
    return _socket_recvfrom(self_in, bufsize, NULL, NULL);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_recv_obj, &socket_recv);

STATIC mp_obj_t socket_sendto(mp_obj_t self_in, mp_obj_t bytes, mp_obj_t address) {
    // ========================================
    // Connects and sends bytes.
    // Args:
    //     bytes (str, byterray): bytes to send;
    //     address (tuple): destination address;
    // ========================================
    socket_obj_t *self = MP_OBJ_TO_PTR(self_in);

    // get the buffer to send
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(bytes, &bufinfo, MP_BUFFER_READ);

    // create the destination address
    struct sockaddr_in to;
    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    to.sin_port = LWIP_HTONS(netutils_parse_inet_addr(address, (uint8_t*)&to.sin_addr, NETUTILS_BIG));

    // send the data
    for (int i=0; i<=self->retries; i++) {
        MP_THREAD_GIL_EXIT();
        int ret = LWIP_SENDTO(self->fd, bufinfo.buf, bufinfo.len, 0, (struct sockaddr*)&to, sizeof(to));
        MP_THREAD_GIL_ENTER();
        if (ret > 0) return mp_obj_new_int_from_uint(ret);
        int errno = LWIP_ERRNO();
        if (ret == -1 && errno != EWOULDBLOCK) {
            exception_from_errno(errno);
        }
        check_for_exceptions();
    }
    mp_raise_OSError(MP_ETIMEDOUT); 
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_3(socket_sendto_obj, &socket_sendto);

STATIC mp_obj_t socket_recvfrom(mp_obj_t self_in, mp_obj_t bufsize) {
    // ========================================
    // Receives bytes.
    // Args:
    //     bufsize (int): output array size;
    // ========================================
    struct sockaddr from;
    socklen_t fromlen = sizeof(from);

    mp_obj_t tuple[2];
    tuple[0] = _socket_recvfrom(self_in, bufsize, &from, &fromlen);

    uint8_t *ip = (uint8_t*)&((struct sockaddr_in*)&from)->sin_addr;
    mp_uint_t port = LWIP_NTOHS(((struct sockaddr_in*)&from)->sin_port);
    tuple[1] = netutils_format_inet_addr(ip, port, NETUTILS_BIG);

    return mp_obj_new_tuple(2, tuple);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_recvfrom_obj, &socket_recvfrom);

STATIC mp_obj_t socket_setsockopt(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Sets socket options.
    // Args:
    //     level (int): option level;
    //     optname (int): option to set;
    //     value (int): value to set;
    // ========================================
    mp_raise_NotImplementedError("Not implemented yet");
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socket_setsockopt_obj, 4, 4, &socket_setsockopt);

STATIC mp_obj_t socket_settimeout(mp_obj_t self_in, mp_obj_t value) {
    // ========================================
    // Sets the timeout for socket operations.
    // Args:
    //     timeout (int): timeout in seconds;
    // ========================================
    mp_raise_NotImplementedError("Not implemented yet");
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_settimeout_obj, &socket_settimeout);

STATIC mp_obj_t socket_setblocking(mp_obj_t self_in, mp_obj_t flag) {
    // ========================================
    // Sets blocking or non-blocking socket mode.
    // Args:
    //     flag (bool): blocking or non-blocking mode;
    // ========================================
    mp_raise_NotImplementedError("Not implemented yet");
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(socket_setblocking_obj, &socket_setblocking);

STATIC mp_obj_t socket_makefile(size_t n_args, const mp_obj_t *args) {
    // ========================================
    // Returns a file object associated with the socket.
    // Args:
    //     mode (str): file mode;
    //     buffering (int): buffer size (not supported);
    // ========================================
    (void)n_args;
    return args[0];
}

STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(socket_makefile_obj, 1, 3, socket_makefile);

STATIC mp_uint_t socket_stream_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode) {
    // ========================================
    // Stream: read.
    // ========================================
    return _socket_read_data(self_in, buf, size, NULL, NULL, errcode); 
}

STATIC mp_uint_t socket_stream_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode) {
    // ========================================
    // Stream: write.
    // ========================================
    socket_obj_t *sock = self_in;
    for (int i=0; i<=sock->retries; i++) {
        MP_THREAD_GIL_EXIT();
        int r = LWIP_WRITE(sock->fd, buf, size);
        MP_THREAD_GIL_ENTER();
        if (r > 0) return r;
        int errno = LWIP_ERRNO();
        if (r < 0 && errno != EWOULDBLOCK) { *errcode = errno; return MP_STREAM_ERROR; }
        check_for_exceptions();
    }
    *errcode = sock->retries == 0 ? MP_EWOULDBLOCK : MP_ETIMEDOUT;
    return MP_STREAM_ERROR;
}

STATIC mp_uint_t socket_stream_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode) {
    // ========================================
    // Stream: IO control.
    // ========================================
    socket_obj_t * socket = self_in;
    if (request == MP_STREAM_POLL) {
        fd_set rfds; FD_ZERO(&rfds);
        fd_set wfds; FD_ZERO(&wfds);
        fd_set efds; FD_ZERO(&efds);
        struct timeval timeout = { .tv_sec = 0, .tv_usec = 0 };
        if (arg & MP_STREAM_POLL_RD) FD_SET(socket->fd, &rfds);
        if (arg & MP_STREAM_POLL_WR) FD_SET(socket->fd, &wfds);
        if (arg & MP_STREAM_POLL_HUP) FD_SET(socket->fd, &efds);

        int r = LWIP_SELECT((socket->fd)+1, &rfds, &wfds, &efds, &timeout);
        if (r < 0) {
            *errcode = MP_EIO;
            return MP_STREAM_ERROR;
        }

        mp_uint_t ret = 0;
        if (FD_ISSET(socket->fd, &rfds)) ret |= MP_STREAM_POLL_RD;
        if (FD_ISSET(socket->fd, &wfds)) ret |= MP_STREAM_POLL_WR;
        if (FD_ISSET(socket->fd, &efds)) ret |= MP_STREAM_POLL_HUP;
        return ret;
    } else if (request == MP_STREAM_CLOSE) {
        if (socket->fd >= 0) {
            int ret = LWIP_CLOSE(socket->fd);
            if (ret != 0) {
                *errcode = LWIP_ERRNO();
                return MP_STREAM_ERROR;
            }
            socket->fd = -1;
            num_sockets_open --;
        }
        return 0;
    }
    *errcode = MP_EINVAL;
    return MP_STREAM_ERROR;
}

STATIC const mp_rom_map_elem_t socket_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR___del__), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR_bind), MP_ROM_PTR(&socket_bind_obj) },
    { MP_ROM_QSTR(MP_QSTR_listen), MP_ROM_PTR(&socket_listen_obj) },
    { MP_ROM_QSTR(MP_QSTR_accept), MP_ROM_PTR(&socket_accept_obj) },
    { MP_ROM_QSTR(MP_QSTR_connect), MP_ROM_PTR(&socket_connect_obj) },
    { MP_ROM_QSTR(MP_QSTR_send), MP_ROM_PTR(&socket_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendall), MP_ROM_PTR(&socket_sendall_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv), MP_ROM_PTR(&socket_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendto), MP_ROM_PTR(&socket_sendto_obj) },
    { MP_ROM_QSTR(MP_QSTR_recvfrom), MP_ROM_PTR(&socket_recvfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_setsockopt), MP_ROM_PTR(&socket_setsockopt_obj) },
    { MP_ROM_QSTR(MP_QSTR_settimeout), MP_ROM_PTR(&socket_settimeout_obj) },
    { MP_ROM_QSTR(MP_QSTR_setblocking), MP_ROM_PTR(&socket_setblocking_obj) },
    { MP_ROM_QSTR(MP_QSTR_makefile), MP_ROM_PTR(&socket_makefile_obj) },

    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
};

STATIC MP_DEFINE_CONST_DICT(socket_locals_dict, socket_locals_dict_table);

STATIC const mp_stream_p_t socket_stream_p = {
    .read = socket_stream_read,
    .write = socket_stream_write,
    .ioctl = socket_stream_ioctl,
};

STATIC const mp_obj_type_t socket_type = {
    { &mp_type_type },
    .name = MP_QSTR_socket,
    .make_new = socket_make_new,
    .protocol = &socket_stream_p,
    .locals_dict = (mp_obj_dict_t*)&socket_locals_dict,
};

// -------
// Methods
// -------

STATIC mp_obj_t get_local_ip(void) {
    // ========================================
    // Retrieves the local IP address.
    // Returns:
    //     A string with the assigned IP address.
    // ========================================
    char ip[16];
    if (!Network_GetIp(ip, sizeof(ip))) {
        mp_raise_ValueError("Failed to retrieve the local IP address");
        return mp_const_none;
    }
    return mp_obj_new_str(ip, strlen(ip));
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(get_local_ip_obj, get_local_ip);

STATIC mp_obj_t getaddrinfo(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // ========================================
    // Translates host/port into arguments to socket constructor.
    // Args:
    //     host (str): host name;
    //     port (int): port number;
    //     af (int): address family: AF_INET or AF_INET6;
    //     type: (int): future socket type: SOCK_STREAM or SOCK_DGRAM;
    //     proto (int): future protocol: IPPROTO_TCP or IPPROTO_UDP;
    //     flag (int): additional socket flags;
    // Returns:
    //     A 5-tuple with arguments to `usocket.socket`.
    // ========================================
    enum { ARG_host, ARG_port, ARG_af, ARG_type, ARG_proto, ARG_flag };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_host, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_port, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_af, MP_ARG_INT, {.u_int = AF_INET} },
        { MP_QSTR_type, MP_ARG_INT, {.u_int = SOCK_STREAM} },
        { MP_QSTR_proto, MP_ARG_INT, {.u_int = IPPROTO_TCP} },
        { MP_QSTR_flag, MP_ARG_INT, {.u_int = 0} },
    };

    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, pos_args, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);
    
    mp_obj_t host = args[ARG_host].u_obj;
    mp_obj_t port = mp_obj_new_int(args[ARG_port].u_int);

    int af;
    switch (args[ARG_af].u_int) {
        case AF_INET:
            af = AF_INET;
            break;
        case AF_INET6:
            mp_raise_ValueError("argument #3: af=AF_INET6 is not implemented");
            break;
        default:
            mp_raise_ValueError("argument #3: 'af' should be one of AF_INET, AF_INET6");
            return mp_const_none;
    }

    int type;
    switch (args[ARG_type].u_int) {
        case SOCK_STREAM:
            type = SOCK_STREAM;
            break;
        case SOCK_DGRAM:
            type = SOCK_DGRAM;
            break;
        default:
            mp_raise_ValueError("Argument #4: 'type' should be one of SOCK_STREAM, SOCK_DGRAM");
            return mp_const_none;
    }

    int proto;
    switch (args[ARG_proto].u_int) {
        case IPPROTO_TCP:
            proto = IPPROTO_TCP;
            break;
        case IPPROTO_UDP:
            proto = IPPROTO_UDP;
            break;
        default:
            mp_raise_ValueError("Argument #5: 'proto' should be one of IPPROTO_TCP, IPPROTO_UDP");
            return mp_const_none;
    }

    int flag = args[ARG_flag].u_int;

    struct sockaddr_in res;
    if (_socket_getaddrinfo2(host, port, &res) < 0) {
        int errno = LWIP_ERRNO();
        exception_from_errno(errno);
    }

    mp_obj_t addrinfo_objs[5] = {
        mp_obj_new_int(res.sin_family),
        mp_obj_new_int(type),
        mp_obj_new_int(proto),
        host,
        mp_const_none,
    };

    // AF_INET
    // This looks odd, but it's really just a u32_t
    ip4_addr_t ip4_addr = { .addr = res.sin_addr.s_addr };
    char buf[16];
    LWIP_IP4ADDR_NTOA_R(&ip4_addr, buf, sizeof(buf));
    mp_obj_t inaddr_objs[2] = {
        mp_obj_new_str(buf, strlen(buf)),
        mp_obj_new_int(LWIP_NTOHS(res.sin_port))
    };
    addrinfo_objs[4] = mp_obj_new_tuple(2, inaddr_objs);

    return mp_obj_new_tuple(5, addrinfo_objs);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(getaddrinfo_obj, 2, getaddrinfo);

STATIC mp_obj_t modusocket_inet_ntop(mp_obj_t af, mp_obj_t bin_addr) {
    // ========================================
    // Converts a binary address into textual representation.
    // Args:
    //     af (int): address family: AF_INET or AF_INET6;
    //     bin_addr (bytearray): binary address;
    // Returns:
    //     A string with the address.
    // ========================================
    mp_raise_NotImplementedError("Not implemented yet");
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(modusocket_inet_ntop_obj, modusocket_inet_ntop);

STATIC mp_obj_t modusocket_inet_pton(mp_obj_t af, mp_obj_t txt_addr) {
    // ========================================
    // Converts a text address into binary representation.
    // Args:
    //     af (int): address family: AF_INET or AF_INET6;
    //     txt_addr (str): address as text;
    // Returns:
    //     A bytearray address.
    // ========================================
    mp_raise_NotImplementedError("Not implemented yet");
    return mp_const_none;
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(modusocket_inet_pton_obj, modusocket_inet_pton);

STATIC mp_obj_t modusocket_get_num_open(void) {
    // ========================================
    // Retrieves the number of open sockets.
    // Returns:
    //     The number of sockets open.
    // ========================================
    return mp_obj_new_int(num_sockets_open);
}

STATIC MP_DEFINE_CONST_FUN_OBJ_0(modusocket_get_num_open_obj, modusocket_get_num_open);

STATIC const mp_map_elem_t mp_module_usocket_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_usocket) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_socket), (mp_obj_t)MP_ROM_PTR(&socket_type) },

    { MP_OBJ_NEW_QSTR(MP_QSTR_get_local_ip), (mp_obj_t)&get_local_ip_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_getaddrinfo), (mp_obj_t)&getaddrinfo_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_inet_ntop), (mp_obj_t)&modusocket_inet_ntop_obj },
    { MP_OBJ_NEW_QSTR(MP_QSTR_inet_pton), (mp_obj_t)&modusocket_inet_pton_obj },

    { MP_OBJ_NEW_QSTR(MP_QSTR_get_num_open), (mp_obj_t)&modusocket_get_num_open_obj },

    { MP_ROM_QSTR(MP_QSTR_AF_INET), MP_ROM_INT(AF_INET) },
    { MP_ROM_QSTR(MP_QSTR_AF_INET6), MP_ROM_INT(AF_INET6) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_STREAM), MP_ROM_INT(SOCK_STREAM) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_DGRAM), MP_ROM_INT(SOCK_DGRAM) },
    { MP_ROM_QSTR(MP_QSTR_SOCK_RAW), MP_ROM_INT(SOCK_RAW) },
    { MP_ROM_QSTR(MP_QSTR_IPPROTO_TCP), MP_ROM_INT(IPPROTO_TCP) },
    { MP_ROM_QSTR(MP_QSTR_IPPROTO_UDP), MP_ROM_INT(IPPROTO_UDP) },
    { MP_ROM_QSTR(MP_QSTR_IPPROTO_IP), MP_ROM_INT(IPPROTO_IP) },
};

STATIC MP_DEFINE_CONST_DICT(mp_module_usocket_globals, mp_module_usocket_globals_table);

const mp_obj_module_t usocket_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&mp_module_usocket_globals,
};
