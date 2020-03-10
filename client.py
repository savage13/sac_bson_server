#!/usr/bin/env python3

import socket
import bson

# Read in Sac File data
data = []
with open("seismo.sac", "rb") as f:
    data = f.read()

# Create BSON DAta
a = bson.dumps({"data": [data,data],
                "process": ["netread",
                            "rtrend"
                            "transfer from evalresp to none",
                            "lp co 1.0 p 2 n 4",
                            "netwrite"]})

# Open Sockey
clientsocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
clientsocket.connect(('localhost', 5555))

# Get Ok
out = clientsocket.recv(64)
print("Got:     ",out)

# Send Length
msg = "{}\r\n".format(2 + len(a)).encode()
print("Sending: ",msg)
clientsocket.send(msg)

# Get Ok
out = clientsocket.recv(64)
print("Got:     ",out)

# Send Data
print("Sending: ","'bson data'");
clientsocket.send(a)
clientsocket.send("\r\n".encode())

# Close connection
clientsocket.close()

