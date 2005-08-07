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
// but is now a subclass of NSView.
//
// .SECTION See Also
// vtkCocoaRenderWindow vtkCocoaRenderWindowInteractor

#import <Cocoa/Cocoa.h>

// Forward declarations
class vtkCocoaRenderWindow;
class vtkCocoaRenderWindowInteractor;

@interface vtkCocoaGLView : NSView
{
  @private
    vtkCocoaRenderWindow *myVTKRenderWindow;
}

- (vtkCocoaRenderWindow *)getVTKRenderWindow;
- (void)setVTKRenderWindow:(vtkCocoaRenderWindow *)theVTKRenderWindow;

- (vtkCocoaRenderWindowInteractor *)getInteractor;

@end
