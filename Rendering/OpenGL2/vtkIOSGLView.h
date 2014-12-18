/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOSGLView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkIOSGLView - IOS OpenGL rendering context
//
// .SECTION Description
// This class is a subclass of IOS's UIView; it uses Objective-C++.
// This class overrides several UIView methods.
// To provide the usual VTK keyboard user interface, it overrides the
// following methods from NSResponder: acceptsFirstResponder, keyDown:,
// keyUp:, and flagsChanged:
// To provide the usual VTK mouse user interface, it overrides the
// following methods from NSResponder: mouseMoved:, mouseEntered:,
// mouseExited: scrollWheel:, mouseDown:, rightMouseDown:,
// otherMouseDown:, and resetCursorRects.
// To be able to render and draw onscreen, it overrides drawRect:
// from UIView.

// .SECTION See Also
// vtkIOSRenderWindow

#ifndef vtkIOSGLView_h
#define vtkIOSGLView_h

#import <UIKit/UIKit.h>

// Note: This file should be includable by both pure Objective-C and Objective-C++ source files.
// To achieve this, we use the neat technique below:
#ifdef __cplusplus
  // Forward declarations
  class vtkIOSRenderWindow;
  class vtkIOSRenderWindowInteractor;

  // Type declarations
  typedef vtkIOSRenderWindow *vtkIOSRenderWindowRef;
  typedef vtkIOSRenderWindowInteractor *vtkIOSRenderWindowInteractorRef;
#else
  // Type declarations
  typedef void *vtkIOSRenderWindowRef;
  typedef void *vtkIOSRenderWindowInteractorRef;
#endif

@interface vtkIOSGLView : UIView
{
  @private
  vtkIOSRenderWindowRef _myVTKRenderWindow;
}

- (vtkIOSRenderWindowRef)getVTKRenderWindow;
- (void)setVTKRenderWindow:(vtkIOSRenderWindowRef)theVTKRenderWindow;

- (vtkIOSRenderWindowInteractorRef)getInteractor;

- (void)setupPipeline;

@end

#endif /* vtkIOSGLView_h */
// VTK-HeaderTest-Exclude: vtkIOSGLView.h
