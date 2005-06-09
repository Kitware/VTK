/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaGLView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCocoaGLView - Cocoa OpenGL rendering context
//
// .SECTION Description
// This class only takes care of allocating an NSOpenGLView which simply
// provides a rendering context. This class uses Objective-C++
//
// .SECTION See Also
// vtkCocoaRenderWindow

#import <Cocoa/Cocoa.h>
#import <OpenGL/gl.h>
#define id Id
#import "vtkCocoaRenderWindow.h"
#import "vtkCocoaRenderWindowInteractor.h"
#include "vtkInteractorStyle.h"
#undef id

@interface vtkCocoaGLView : NSOpenGLView
{
  NSOpenGLPixelFormatAttribute bitsPerPixel, depthSize;

  @private
    vtkCocoaRenderWindow *myVTKRenderWindow;
    vtkCocoaRenderWindowInteractor *myVTKRenderWindowInteractor;
}

// Overrides
- (void) drawRect:(NSRect)theRect;
- (id)initWithFrame:(NSRect)theFrame;

- (vtkCocoaRenderWindow *)getVTKRenderWindow;
- (void)setVTKRenderWindow:(vtkCocoaRenderWindow *)theVTKRenderWindow;

- (vtkCocoaRenderWindowInteractor *)getVTKRenderWindowInteractor;
- (void)setVTKRenderWindowInteractor:(vtkCocoaRenderWindowInteractor *)theVTKRenderWindowInteractor;

- (void*)getOpenGLContext;

@end
