//
//  AppDelegate.swift
//  VTKViewer
//
//  Created by Benjamin Beney on 11/16/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

import UIKit

@UIApplicationMain
class AppDelegate: UIResponder, UIApplicationDelegate {

    var window: UIWindow?

    func application(_ application: UIApplication, didFinishLaunchingWithOptions launchOptions: [UIApplicationLaunchOptionsKey: Any]?) -> Bool {
        // Hide status bar for App
        application.isStatusBarHidden = true

        // Check if any example data can be downloaded
        ExampleDataManager.downloadExampleData()

        return true
    }

    func application(_ app: UIApplication, open url: URL, options: [UIApplicationOpenURLOptionsKey : Any] = [:]) -> Bool {
        // Application is already launched and a compatible file has been selected
        openFile(at: url)

        return true;
    }

    func applicationWillResignActive(_ application: UIApplication) {
        // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
        // Use this method to pause ongoing tasks, disable timers, and invalidate graphics rendering callbacks. Games should use this method to pause the game.
    }

    func applicationDidEnterBackground(_ application: UIApplication) {
        // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
        // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
    }

    func applicationWillEnterForeground(_ application: UIApplication) {
        // Called as part of the transition from the background to the active state; here you can undo many of the changes made on entering the background.
    }

    func applicationDidBecomeActive(_ application: UIApplication) {
        // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
    }

    func applicationWillTerminate(_ application: UIApplication) {
        // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
    }

    func openFile(at url:URL) {
        let mainViewController = window?.rootViewController as? VTKViewController

        let fileExists = FileManager.default.fileExists(atPath: url.path)
        if (fileExists) {
            // File exists, it can be loaded directly.
            mainViewController?.loadFiles([url])
        }
        else {
            // Otherwise, a UIDocument is required.
            // First instantiate the document. It will also create the file on the file system.
            let document = VTKViewerDocument(fileURL: url)

            // Get the content of URL.
            let data = NSData(contentsOf: url)
            do {
                // Load the data into the document.
                try document.load(fromContents: data as Any, ofType: document.fileType)
            } catch (_) {
                // Let mainViewController.loadFile() function handle the error messages. Worst case scenario : file is empty, and this will be handled in loadFile() function.
            }

            let tempDirectoryURL = URL(fileURLWithPath: NSTemporaryDirectory(), isDirectory: true)
            let localDocumentPath = tempDirectoryURL.appendingPathComponent(url.lastPathComponent)

            // Save document locally in the app's temporary directory.
            document.save(to: localDocumentPath, for: UIDocumentSaveOperation.forOverwriting, completionHandler: { _ in
                mainViewController?.loadFiles([document.fileURL])
            })
        }
    }
}
