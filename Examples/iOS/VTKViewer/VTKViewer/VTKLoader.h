//
//  VTKLoader.h
//  VTKViewer
//
//  Created by Alexis Girault on 11/17/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

#import <Foundation/Foundation.h>

#include "vtkActor.h"
#include "vtkSmartPointer.h"

@interface VTKLoader : NSObject

+ (vtkSmartPointer<vtkActor>)loadFromURL:(NSURL*)url;

@end
