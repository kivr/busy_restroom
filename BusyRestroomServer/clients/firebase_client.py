#!/usr/bin/python

import firebase
import select
import sys
from socket import *

f=firebase.FirebaseApplication('https://busyrestroom.firebaseio.com')

s=socket(AF_UNIX, SOCK_STREAM)
s.connect(('/var/run/busy_restroom'))
s.setblocking(0)

lastDoorState='O'
state = 'O'
personCount = 0

while True:
    print("Waiting...")
    sys.stdout.flush()
    ready = select.select([s], [], [])

    if ready[0]:
        c=s.recv(1)
        print("Received %c" % c)
        state=c[0:1]

        if state == 'P' and lastDoorState == 'C':
            personCount = personCount + 1
        
        if state == 'C':
            personCount = 0
        elif state != 'P':
            lastDoorState=state

        if personCount < 2:
            state = 'O' #Green toilet
        elif personCount == 2:
            state = 'C' #Yellow toilet
        else
            state = 'P' #Red toilet

        print("Sending %c" % state)
        f.patch('https://busyrestroom.firebaseio.com/farDoor', {'state':state})
