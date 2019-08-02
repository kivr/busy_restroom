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
lastState = 'O'

while True:
    if lastState == 'P':
        print("Waiting with timeout...")
        sys.stdout.flush()
        ready = select.select([s], [], [], 20)
    else:
        print("Waiting...")
        sys.stdout.flush()
        ready = select.select([s], [], [])

    if ready[0]:
        c=s.recv(1)
        print("Received %c" % c)
        lastState=c[0:1]

        if lastState == 'P' and lastDoorState == 'O':
            print("Skipping P state while door opened")
            lastState='O'
            continue
        
        if lastState != 'P':
            lastDoorState=lastState

        print("Sending %c" % lastState)
        f.patch('https://busyrestroom.firebaseio.com/farDoor', {'state':lastState})
    else:
        lastState='C'
        print("Sending %c" % lastState)
        f.patch('https://busyrestroom.firebaseio.com/farDoor', {'state':lastState})
