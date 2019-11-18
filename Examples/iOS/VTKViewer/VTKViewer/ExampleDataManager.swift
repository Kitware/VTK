//
//  ExampleDataManager.swift
//  VTKViewer
//
//  Created by Alexis Girault on 11/20/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

import Foundation

class ExampleDataManager: NSObject {
    private static var downloadSession = URLSession(configuration: URLSessionConfiguration.default)
    private static var exampleDataDir = "ExampleData"
    private static var exampleDataURLs = [
        "bunny.obj" : "https://data.kitware.com/api/v1/file/5a1343508d777f31ac64f1d5/download",
        "teapot.obj" : "https://data.kitware.com/api/v1/item/5a1343ed8d777f31ac64f1d7/download",
    ]
    private static var downloadedDataList = [String : Bool]()
    private static var downloadedDataListFileName = ".downloadedDataList.json"

    static func downloadExampleData() {
        // Define example data directory path
        let documentsDir = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0]
        var exampleDataDirUrl = URL.init(fileURLWithPath: documentsDir)
        exampleDataDirUrl.appendPathComponent(exampleDataDir)

        // Create example data directory if needed
        var isDir = ObjCBool(false)
        if !FileManager.default.fileExists(atPath: exampleDataDirUrl.path, isDirectory: &isDir) || !isDir.boolValue {
            do {
                try FileManager.default.createDirectory(at: exampleDataDirUrl,
                                                        withIntermediateDirectories: true,
                                                        attributes: nil)
            } catch {
                print("Error creating directory at \(exampleDataDirUrl) : will skip downloading example data.")
                return
            }
        }

        // Ensure list of downloaded data is up to date
        ExampleDataManager.readDownloadedDataList()

        // Look at all file urls
        for (fileName, urlStr) in exampleDataURLs {
            // Check if already downloaded
            let val = downloadedDataList[fileName]
            if val != nil && val == true {
                continue
            }

            // Check that url is valid
            guard let url = URL.init(string: urlStr) else {
                continue
            }

            // Setup download request
            let downloadTask = downloadSession.downloadTask(with: url) {tempURL, downloadResponse, error in
                if error != nil || tempURL == nil {
                    print("Error downloading file \(url) : \(error!.localizedDescription)")
                    return
                }

                // Once downloaded, move item to destination
                let destinationURL = exampleDataDirUrl.appendingPathComponent(fileName)
                do {
                    try FileManager.default.moveItem(at: tempURL!, to: destinationURL)
                } catch (let writeError) {
                    print("Error writing file \(destinationURL) : \(writeError.localizedDescription)")
                    return
                }

                // Mark as downloaded to avoid downloading again
                markDataDownloaded(for: fileName)
            }

            downloadTask.resume()
        }
    }

    private static func markDataDownloaded(for fileName:String) {
        // Update dictionary
        downloadedDataList[fileName] = true

        // Write dictionary
        let downloadedDataListUrl = getDownloadedDataListUrl()
        do {
            let data = try JSONSerialization.data(withJSONObject: downloadedDataList, options: .prettyPrinted)
            try data.write(to: downloadedDataListUrl)
        } catch {
        }
    }

    static func readDownloadedDataList() {
        let downloadedDataListUrl = getDownloadedDataListUrl()
        if FileManager.default.fileExists(atPath: downloadedDataListUrl.path) {
            do {
                let data = try Data(contentsOf: downloadedDataListUrl)
                let json = try JSONSerialization.jsonObject(with: data, options: [])
                downloadedDataList = json as! [String : Bool]
            } catch {
            }
        }
    }

    static func deleteDownloadedDataList() {
        let downloadedDataListUrl = getDownloadedDataListUrl()
        do {
            try FileManager.default.removeItem(at: downloadedDataListUrl)
        } catch {
        }
    }

    private static func getDownloadedDataListUrl() -> URL {
        let documentsDir = NSSearchPathForDirectoriesInDomains(.documentDirectory, .userDomainMask, true)[0]
        var downloadedDataListPath = URL.init(fileURLWithPath:documentsDir)
        downloadedDataListPath.appendPathComponent(downloadedDataListFileName)

        return downloadedDataListPath
    }
}
