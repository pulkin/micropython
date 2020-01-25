import cellular
import usocket

class WWAN:
    def __new__(cls):
        return cls

    @classmethod
    def active(cls, is_active=None):
        return True

    @classmethod
    def connect(cls, apn, user, password):
        return cellular.gprs(apn, user, password)

    @classmethod
    def disconnect(cls):
        return cellular.gprs(False)

    @classmethod
    def isconnected(cls):
        return cellular.gprs()

    @classmethod
    def ifconfig(cls):
        return usocket.get_local_ip(), None, None, None

