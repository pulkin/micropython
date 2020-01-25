import cellular
import socket
cellular.gprs("internet", "", "")
print("IP", socket.get_local_ip())
host = "httpstat.us"
port = 80
s = socket.socket()
s.connect((host, port))
message = "GET /200 HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n"
s.write(message.format(host))
print(s.read(256))
s.close()
cellular.gprs(False)

