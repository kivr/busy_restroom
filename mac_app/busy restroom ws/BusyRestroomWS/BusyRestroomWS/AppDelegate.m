//
//  AppDelegate.m
//  BusyRestroomWS
//
//  Created by Kristian Villalobos on 10/18/19.
//  Copyright © 2019 Kristian Villalobos. All rights reserved.
//

#import "AppDelegate.h"
#import "firebase_api.h"
#import <sys/socket.h>
#import <netinet/in.h>
#import <SystemConfiguration/SystemConfiguration.h>

#define DATA_PREFIX @"{\"t\":\"d\",\"d\":{\"b\":{\"p\":\"farDoor/state\",\"d\":\""

static BOOL connected = NO;

@interface AppDelegate ()

@property (nonatomic, strong) NSStatusBarButton *button;

@end

@implementation AppDelegate {
    NSStatusItem *statusItem;
}

-(void)messageReceived:(const char*) msg {
    NSString *message = [NSString stringWithCString:msg encoding:NSASCIIStringEncoding];
    
    __block NSStatusBarButton *blockButton = _button;
    
    if ([message hasPrefix:DATA_PREFIX]) {
        if ([message characterAtIndex:[DATA_PREFIX length]] == 'O') {
            NSLog(@"Opened");
            dispatch_async(dispatch_get_main_queue(), ^{
               [blockButton setImage:[NSImage imageNamed:@"Empty"]];
            });
        } else if ([message characterAtIndex:[DATA_PREFIX length]] == 'C') {
            NSLog(@"Closed");
            dispatch_async(dispatch_get_main_queue(), ^{
               [blockButton setImage:[NSImage imageNamed:@"Semi"]];
            });
        } else if ([message characterAtIndex:[DATA_PREFIX length]] == 'P') {
            NSLog(@"Person");
            dispatch_async(dispatch_get_main_queue(), ^{
               [blockButton setImage:[NSImage imageNamed:@"Busy"]];
            });
        }
    }
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    __block id<NSApplicationDelegate> blockSelf = self;
    
    statusItem = [NSStatusBar.systemStatusBar statusItemWithLength:NSSquareStatusItemLength];
    _button = statusItem.button;
    
    [_button setImage:[NSImage imageNamed:@"Disconnected"]];
    
    __block NSStatusBarButton *blockButton = _button;
    [NSTimer scheduledTimerWithTimeInterval:5.0 repeats:YES block:^(NSTimer *timer){
        if([AppDelegate hasConnectivity] && !connected) {
            connected = YES;
            dispatch_async(dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0), ^{
               firebase_connect(blockSelf);
            });
        } else if (![AppDelegate hasConnectivity] && connected) {
            connected = NO;
            dispatch_async(dispatch_get_global_queue(QOS_CLASS_BACKGROUND, 0), ^{
                firebase_disconnect();
                [blockButton setImage:[NSImage imageNamed:@"Disconnected"]];
            });
        }
    }];
    
    [NSTimer scheduledTimerWithTimeInterval:45.0 repeats:YES block:^(NSTimer *timer){
        firebase_ping();
    }];
}

/*
Connectivity testing code pulled from Apple's Reachability Example: https://developer.apple.com/library/content/samplecode/Reachability
 */
+(BOOL)hasConnectivity {
    struct sockaddr_in zeroAddress;
    bzero(&zeroAddress, sizeof(zeroAddress));
    zeroAddress.sin_len = sizeof(zeroAddress);
    zeroAddress.sin_family = AF_INET;

    SCNetworkReachabilityRef reachability = SCNetworkReachabilityCreateWithAddress(kCFAllocatorDefault, (const struct sockaddr*)&zeroAddress);
    if (reachability != NULL) {
        //NetworkStatus retVal = NotReachable;
        SCNetworkReachabilityFlags flags;
        if (SCNetworkReachabilityGetFlags(reachability, &flags)) {
            if ((flags & kSCNetworkReachabilityFlagsReachable) == 0)
            {
                // If target host is not reachable
                return NO;
            }

            if ((flags & kSCNetworkReachabilityFlagsConnectionRequired) == 0)
            {
                // If target host is reachable and no connection is required
                //  then we'll assume (for now) that your on Wi-Fi
                return YES;
            }


            if ((((flags & kSCNetworkReachabilityFlagsConnectionOnDemand ) != 0) ||
                 (flags & kSCNetworkReachabilityFlagsConnectionOnTraffic) != 0))
            {
                // ... and the connection is on-demand (or on-traffic) if the
                //     calling application is using the CFSocketStream or higher APIs.

                if ((flags & kSCNetworkReachabilityFlagsInterventionRequired) == 0)
                {
                    // ... and no [user] intervention is needed
                    return YES;
                }
            }
        }
    }

    return NO;
}


- (void)applicationWillTerminate:(NSNotification *)aNotification {
    // Insert code here to tear down your application
}


@end
