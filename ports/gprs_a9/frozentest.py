# Tests sending SMS
test_send_sms = None
# test_send_sms = "8800"

# Tests filesystem manipulations
test_fs = False
# test_fs = True

print("================")
print("std")
print("================")

print('a long string that is not interned')
print('a string that has unicode αβγ chars')
print(b'bytes 1234\x01')
print(123456789)
for i in range(4):
    print(i)

print("================")
print("gc")
print("================")
print("400k allocation ...")
x = bytearray(400000)
print("Testing loop ...")
long_string = "-" * 2 * 2048
for i in range(512):
    x = str(i) + long_string
del long_string
del x

import gc
gc.collect()
mem_free = gc.mem_free()
mem_alloc = gc.mem_alloc()
mem_tot = mem_free + mem_alloc
print("Free:", mem_free, "alloc:", mem_alloc, "total:", mem_tot)
assert mem_tot > 512e3

if test_fs:
    print("================")
    print("filesystem")
    print("================")

    import os

    os.chdir("/")
    cwd = os.getcwd()
    print("CWD:", cwd)
    assert cwd == "/"
    nfiles = len(os.listdir())
    print("Files:", nfiles)

    test_file = "__test_file__"
    test_contents = "abcDEF"
    f = open(test_file, 'w')
    f.write(test_contents)
    f.close()
    f = open(test_file, 'r')
    data = f.read()
    f.close()
    print("Contents:", data)
    assert data == test_contents

    f = open(test_file, 'w')
    f.write(test_contents[:-1])
    f.close()
    f = open(test_file, 'r')
    data = f.read()
    f.close()
    print("Contents:", data)
    assert data == test_contents[:-1]

    os.remove(test_file)
    _nfiles = len(os.listdir())
    print("Files:", _nfiles)
    assert nfiles == _nfiles

    test_dir1 = "__temp_test_dir__"
    os.mkdir(test_dir1)
    os.chdir(test_dir1)
    cwd = os.getcwd()
    print("CWD:", cwd)
    assert cwd == "/"+test_dir1

    test_dir2 = "__temp_test_dir2__"
    os.mkdir(test_dir2)
    os.chdir(test_dir2)
    cwd = os.getcwd()
    print("CWD:", cwd)
    assert cwd == "/"+test_dir1+"/"+test_dir2

    ls = os.listdir()
    print("Files:", ls)
    assert len(ls) == 0

    test_file = "__test_file__"
    test_contents = "abcDEF"
    f = open(test_file, 'w')
    f.write(test_contents)
    f.close()
    ls = os.listdir()
    print("Files:", ls)
    assert len(ls) == 1
    assert ls[0] == test_file

    f = open(test_file, 'r')
    data = f.read()
    f.close()
    print("Contents:", data)
    assert data == test_contents

    os.chdir("/")

    f = open("/" + test_dir1 + "/" + test_dir2 + "/" + test_file, 'r')
    data = f.read()
    f.close()
    print("Contents:", data)
    assert data == test_contents

    ls = os.listdir("/" + test_dir1)
    print("Files:", ls)
    assert len(ls) == 1
    assert ls[0] == test_dir2

    ls = os.listdir("/" + test_dir1 + "/" + test_dir2)
    print("Files:", ls)
    assert len(ls) == 1
    assert ls[0] == test_file

    os.remove("/" + test_dir1 + "/" + test_dir2 + "/" + test_file)
    ls = os.listdir("/" + test_dir1 + "/" + test_dir2)
    print("Files:", ls)
    assert len(ls) == 0

    os.rmdir("/" + test_dir1 + "/" + test_dir2)
    ls = os.listdir("/" + test_dir1)
    print("Files:", ls)
    assert len(ls) == 0

    os.rmdir("/" + test_dir1)

print("================")
print("GPS")
print("================")

import gps
gps.on()

fw = gps.get_firmware_version()
print("GPS firmware:", fw)
assert len(fw) > 3

vis, tracked = gps.get_satellites()
print("GPS sats:", vis, tracked)
assert vis == int(vis)
assert tracked == int(tracked)
assert 0 <= tracked <= vis

lat, lng = gps.get_location()
print("GPS location:", lat, lng)
assert -90 <= lat <= 90
assert -180 <= lng <= 180

gps.off()
_lat, _lng = gps.get_last_location()
print("GPS last location:", lat, lng)
assert lat == _lat
assert lng == _lng

try:
    gps.on(0)
    raise ValueError("No GPS exception was raised")
except gps.GPSError as e:
    print("GPS error OK:", e)
gps.off()
del gps

print("================")
print("Network")
print("================")

import cellular as cel
import time
sim_present = cel.is_sim_present()
print("SIM present:", sim_present)

if sim_present:

    print("----------------")
    print("Network")
    print("----------------")

    reg = cel.is_network_registered()
    print("Ntw registered:", reg)
    assert reg

    roam = cel.is_roaming()
    print("Roaming:", roam)
    assert not roam

    qual, rxq = cel.get_signal_quality()
    print("Signal:", qual, rxq)
    assert 0 < qual < 32
    assert rxq is None

    status = cel.get_network_status()
    print("Ntw status:", status)
    # TODO: check here after implementing constants
    assert status != 0

    imei = cel.get_imei()
    print("IMEI:", imei)
    assert len(imei) == 15

    iccid = cel.get_iccid()
    print("ICCID:", iccid)
    assert len(iccid) == 19

    imsi = cel.get_imsi()
    print("IMSI:", imsi)
    assert len(imsi) <= 15

    cel.poll_network_exception()

    print("----------------")
    print("SMS")
    print("----------------")

    sms_received = cel.sms_received()
    print("SMS received count:", sms_received)

    sms_list = cel.sms_list()
    print("SMS:", sms_list)
    assert len(sms_list) > 0
    for i in sms_list:
        assert isinstance(i, cel.SMS)

    if test_send_sms is not None:
        sms = cel.SMS(test_send_sms, "hello")
        sms.send()

    print("----------------")
    print("GPRS")
    print("----------------")

    cel.gprs_deactivate()
    cel.gprs_detach()
    cel.network_status_changed()
    time.sleep(2)

    cel.gprs_attach()
    # LEBARA NL credentials
    cel.gprs_activate("internet", "", "")

    cb = cel.network_status_changed()
    print("Status changed:", cb, "->", cel.get_network_status())
    assert cb

    import socket as sock
    loc_ip = sock.get_local_ip()
    print("Local IP:", loc_ip)
    assert len(loc_ip.split(".")) == 4

    assert sock.get_num_open() == 0

    host = "httpstat.us"
    port = 80
    host_ai = sock.getaddrinfo(host, port)
    print("Addrinfo:", host_ai)
    assert host_ai[:3] == (sock.AF_INET, sock.SOCK_STREAM, sock.IPPROTO_TCP)
    assert host_ai[3] == host
    assert len(host_ai[4][0].split(".")) == 4
    assert host_ai[4][1] == 80

    s = sock.socket()
    s.connect((host, port))
    assert sock.get_num_open() == 1
    assert s.makefile() == s

    message = "GET /200 HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n"
    response_expected = b"HTTP/1.1 200 OK\r\n"
    message_f = message.format(host)
    bytes_sent = s.send(message_f[:10])
    bytes_sent += s.write(message_f[10:20])
    bytes_sent += s.sendto(message_f[20:30], ("23.99.0.12", 80))
    print("Socket sent:", bytes_sent)
    assert bytes_sent == 30
    s.sendall(message_f[30:])

    # Chunk 1: recv
    response = s.recv(3)
    # Chunk 2: read
    response += s.read(3)
    # Chunk 3: readinto
    chunk3 = bytearray(len(response_expected) - 6)
    s.readinto(chunk3)
    response += chunk3
    print("Socket recvd:", response)
    assert response == response_expected

    line = s.readline()
    print("... recvd:", line)
    assert len(line) > 0
    s.close()

    s = sock.socket()
    s.connect((host, port))
    bytes_sent = s.sendto(message_f, ("127.0.0.1", 80))
    print("Socket sent (2):", bytes_sent)
    assert bytes_sent == len(message_f)

    response, fr = s.recvfrom(len(response_expected))
    print("Socket recvd (2):", response, fr)
    assert response == response_expected
    assert fr == host_ai[4]
    del s
    gc.collect()

    import ssl
    port = 443
    s = sock.socket()
    s.connect((host, port))
    s = ssl.wrap_socket(s)
    s.write(message_f)
    response = s.read(len(response_expected))
    print("SSL socket recvd:", response)
    assert response == response_expected
    s.close()

    assert sock.get_num_open() == 0

    cel.gprs_deactivate()
    cel.gprs_detach()

print("================")
print("machine")
print("================")

import machine

machine.idle()

cause = machine.power_on_cause()
print("Power on cause:", cause)

v, percent = machine.get_input_voltage()
print("Voltage:", 1e-3 * v, percent, "%")
assert 3 < 1e-3 * v < 5
assert 10 < percent < 100

print("Resetting ...")
machine.reset()
raise RuntimeError("Failed to reset the module")

