import firebase
f=firebase.FirebaseApplication('https://busyrestroom.firebaseio.com')
f.patch('https://busyrestroom.firebaseio.com/farDoor', {'state':'0'})
