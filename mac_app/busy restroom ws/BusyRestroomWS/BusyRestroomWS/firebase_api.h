//
//  firebase_api.h
//  BusyRestroomWS
//
//  Created by Kristian Villalobos on 10/18/19.
//  Copyright © 2019 Kristian Villalobos. All rights reserved.
//

#ifndef firebase_api_h
#define firebase_api_h

bool firebase_ping(void);
int firebase_connect(id appDelegate);
void firebase_disconnect();

#endif /* firebase_api_h */
