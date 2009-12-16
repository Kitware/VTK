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
// This class is a subclass of Cocoa's NSView; it uses Objective-C++.
// This class overrides several NSView methods. It overrides keyDown and
// keyUp to provide the usual VTK keyboard user interface. It overrides
// mouseMoved, scrollWheel, mouseDown, rightMouseDown, and otherMouseDown
// to provide the usual VTK mouse user interface. It overrides drawRect
// to render and draw onscreen.
// Note that this class was previously a subclass of NSOpenGLView,
// but starting with VTK 5.0 is now a subclass of NSView.
//
// .SECTION See Also
// vtkCocoaRenderWindow vtkCocoaRenderWindowInteractor

#ifndef __vtkCocoaGLView_h
#define __vtkCocoaGLView_h

#import <Cocoa/Cocoa.h>

// Note: This file should be includable by both pure Objective-C and Objective-C++ source files.
// To achieve this, we use the neat technique below:
#ifdef __cplusplus
  // Forward declarations
  class vtkCocoaRenderWindow;
  class vtkCocoaRenderWindowInteractor;
  
  // Type declarations
  typedef vtkCocoaRenderWindow* vtkCocoaRenderWindowRef;
  typedef vtkCocoaRenderWindowInteractor* vtkCocoaRenderWindowInteractorRef;
#else
  // Type declarations
  typedef void* vtkCocoaRenderWindowRef;
  typedef void* vtkCocoaRenderWindowInteractorRef;
#endif

@interface vtkCocoaGLView : NSView
{
  @private
  vtkCocoaRenderWindowRef myVTKRenderWindow;
  int rolloverTrackingRectTag;
}

- (vtkCocoaRenderWindowRef)getVTKRenderWindow;
- (void)setVTKRenderWindow:(vtkCocoaRenderWindowRef)theVTKRenderWindow;

- (vtkCocoaRenderWindowInteractorRef)getInteractor;
- (void)clearTrackingRect;

@end

#endif /* __vtkCocoaGLView_h */
