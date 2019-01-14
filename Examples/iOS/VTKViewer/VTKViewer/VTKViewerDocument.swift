//
//  VTKViewerDocument.swift
//  VTKViewer
//
//  Created by Benjamin on 24/11/2017.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

import Foundation

class VTKViewerDocument: UIDocument {
    var documentData : Data?

    override func contents(forType typeName: String) throws -> Any {
        return documentData as Any
    }

    override func load(fromContents contents: Any, ofType typeName: String?) throws {
        let data = contents as! NSData
        documentData = data as Data?
    }
}
