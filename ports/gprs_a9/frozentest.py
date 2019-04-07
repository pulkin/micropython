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
except gps.GPSError as e:
    print("GPS error OK:", e)
gps.off()
del gps

print("================")
print("Network status")
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

    import usocket as sock
    loc_ip = sock.get_local_ip()
    print("Local IP:", loc_ip)
    assert len(loc_ip.split(".")) == 4

    host = "httpstat.us"
    host_ip = sock.dns_resolve(host)
    print("Resolved:", host, "->", host_ip)
    assert len(host_ip.split(".")) == 4

    s = sock.socket()
    s.connect((host, 80))
    message = "GET /200 HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n"
    response_expected = b"HTTP/1.1 200 OK"
    message_f = message.format(host)
    bytes_sent = s.send(message_f)
    print("Socket(name) sent:", bytes_sent)
    assert bytes_sent == len(message_f)

    response = s.recv(len(response_expected))
    print("Socket(name) rcvd:", response)
    assert response == response_expected
    s.close()

    s = sock.socket()
    s.connect((host_ip, 80))
    bytes_sent = s.send(message_f)
    print("Socket(ip) sent:", bytes_sent)
    assert bytes_sent == len(message_f)

    response = s.recv(len(response_expected))
    print("Socket(ip) rcvd:", response)
    assert response == response_expected
    s.close()

    cel.gprs_deactivate()
    cel.gprs_detach()

