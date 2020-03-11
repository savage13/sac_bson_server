# sac_bson_server

Basic Client (client.py) and Server (simple.c) that reads and writes data in BSON format

```
% make
cc -Wall -Wextra -I/opt/local/include/libbson-1.0   -c -o simple.o simple.c
cc -Wall -Wextra -I/opt/local/include/libbson-1.0 -o simple simple.o -L/opt/local/lib  -lbson-static-1.0

# Best to run the server in a separate terminal
% simple
FILE: CDV.IU.00.BHZ delta: 0.010000
FILE: CDV.IU.00.BHZ delta: 0.010000
  [  0/  4]: netread
  [  1/  4]: rtrendtransfer from evalresp to none
  [  2/  4]: lp co 1.0 p 2 n 4
  [  3/  4]: netwrite

% ./client.py
Got:      b'OK\r\n'
Sending:  b'9412\r\n'
Got:      b'OK\r\n'
Sending:  'bson data'
%
```
