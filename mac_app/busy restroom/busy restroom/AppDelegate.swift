//
//  AppDelegate.swift
//  busy restroom
//
//  Created by Kristian Villalobos on 7/29/19.
//  Copyright © 2019 Kristian Villalobos. All rights reserved.
//

import Cocoa

@NSApplicationMain
class AppDelegate: NSObject, NSApplicationDelegate {

    @IBOutlet weak var window: NSWindow!

    let statusItem = NSStatusBar.system.statusItem(withLength:NSStatusItem.squareLength)
    weak var button:NSStatusBarButton!

    func applicationDidFinishLaunching(_ aNotification: Notification) {
        NSApp.setActivationPolicy(.accessory)
        
        button = statusItem.button
        
        if button != nil {
            button.image = NSImage(named:NSImage.Name("StatusBarWarn"))
        }
        
        _ = Timer.scheduledTimer(timeInterval: 5.0, target: self, selector: #selector(check), userInfo: nil, repeats: true)
    }
    
    @objc func check() {
        let url = URL(string: "https://busyrestroom.firebaseio.com/farDoor/.json")!
        
        let task = URLSession.shared.dataTask(with: url) {(data, response, error) in
            guard let data = data else { return }
            
            let response = String(data: data, encoding: .utf8)
            
            print(response!)
            
            if response == "{\"state\":\"O\"}" {
                DispatchQueue.main.async {
                    self.button.image = NSImage(named:NSImage.Name("StatusBarFree"))
                }
            } else if response == "{\"state\":\"P\"}" {
                DispatchQueue.main.async {
                    self.button.image = NSImage(named:NSImage.Name("StatusBarBusy"))
                }
            } else {
                DispatchQueue.main.async {
                    self.button.image = NSImage(named:NSImage.Name("StatusBarWarn"))
                }
            }
        }
        
        task.resume()
    }

    func applicationWillTerminate(_ aNotification: Notification) {
        // Insert code here to tear down your application
    }


}

