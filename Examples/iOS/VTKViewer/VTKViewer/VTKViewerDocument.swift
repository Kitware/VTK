// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (C) Copyright © 2017 Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause

//
//  VTKViewerDocument.swift
//  VTKViewer
//
//  Created by Benjamin Beney on 24/11/2017.
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
