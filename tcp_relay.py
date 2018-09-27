#-*- encoding: utf-8 -*-
import socket
import struct
import time

a = [0xAA,0x0F,0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0x01,0xBB]

def set_relay_index(ip, port, i):
    s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s.connect((ip, port))

    #open
    a[2] = i
    a[3] = 0x01
    data=struct.pack("%dB"%(len(a)),*a)
    s.send(data)
    #print s.recv(1024)

    time.sleep(0.5)

    #close
    a[2] = i
    a[3] = 0x02
    data=struct.pack("%dB"%(len(a)),*a)
    s.send(data) 
    #print s.recv(1024)

    time.sleep(0.5)

    s.close()

if __name__=="__main__":
    set_relay_index("192.168.1.103", 8080, 0)
    set_relay_index("192.168.1.103", 8080, 2)

