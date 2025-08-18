// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#import "vtkCocoaAutoreleasePool.h"

#import "vtkCocoaMacOSXSDKCompatibility.h"
#import <Cocoa/Cocoa.h>

#if VTK_OBJC_IS_ARC
#error vtkCocoaAutoreleasePool must not be compiled with ARC
#endif

vtkCocoaAutoreleasePool::vtkCocoaAutoreleasePool()
{
  // Create the pool on construction
  this->Pool = [[NSAutoreleasePool alloc] init];
}

vtkCocoaAutoreleasePool::~vtkCocoaAutoreleasePool()
{
  // Release the pool on destruction
  this->Release();
}

void vtkCocoaAutoreleasePool::Release()
{
  [(NSAutoreleasePool*)Pool release];
  this->Pool = nullptr;
}
