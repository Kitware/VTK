// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (C) Copyright © 2017 Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause

//
//  VTKLoader.h
//  VTKViewer
//
//  Created by Alexis Girault on 11/17/17.
//

#import <Foundation/Foundation.h>

#include "vtkActor.h"
#include "vtkSmartPointer.h"

@interface VTKLoader : NSObject

+ (vtkSmartPointer<vtkActor>)loadFromURL:(NSURL*)url;

@end
