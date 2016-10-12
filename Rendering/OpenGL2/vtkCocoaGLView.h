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
/**
 * @class   vtkCocoaGLView
 * @brief   Cocoa OpenGL rendering context
 *
 *
 * This class is a subclass of Cocoa's NSView; it uses Objective-C++.
 * This class overrides several NSView methods.
 * To provide the usual VTK keyboard user interface, it overrides the
 * following methods: acceptsFirstResponder, keyDown:,
 * keyUp:, and flagsChanged:
 * To provide the usual VTK mouse user interface, it overrides the
 * following methods: mouseMoved:, mouseEntered:,
 * mouseExited: scrollWheel:, mouseDown:, rightMouseDown:,
 * otherMouseDown:, mouseDragged:, rightMouseDragged:, otherMouseDragged:,
 * and updateTrackingAreas.
 * To be able to render and draw onscreen, it overrides drawRect:.
 *
 * Compatibility notes:
 * - this class was previously a subclass of NSOpenGLView,
 * but starting with VTK 5.0 is now a subclass of NSView.
 * - starting with VTK 6.3 this class overrides the more modern
 * updateTrackingAreas instead of resetCursorRects.
 *
 * @sa
 * vtkCocoaRenderWindow vtkCocoaRenderWindowInteractor
*/

#ifndef vtkCocoaGLView_h
#define vtkCocoaGLView_h

#include "vtkRenderingOpenGL2Module.h" // For export macro
#import <Cocoa/Cocoa.h>

// Note: This file should be includable by both pure Objective-C and Objective-C++ source files.
// To achieve this, we use the neat technique below:
#ifdef __cplusplus
  // Forward declarations
  class vtkCocoaRenderWindow;
  class vtkCocoaRenderWindowInteractor;

  // Type declarations
  typedef vtkCocoaRenderWindow *vtkCocoaRenderWindowRef;
  typedef vtkCocoaRenderWindowInteractor *vtkCocoaRenderWindowInteractorRef;
#else
  // Type declarations
  typedef void *vtkCocoaRenderWindowRef;
  typedef void *vtkCocoaRenderWindowInteractorRef;
#endif

VTKRENDERINGOPENGL2_EXPORT
@interface vtkCocoaGLView : NSView
{
  @private
  vtkCocoaRenderWindowRef _myVTKRenderWindow;
  NSTrackingArea* _rolloverTrackingArea;
}

- (vtkCocoaRenderWindowRef)getVTKRenderWindow;
- (void)setVTKRenderWindow:(vtkCocoaRenderWindowRef)theVTKRenderWindow;

- (vtkCocoaRenderWindowInteractorRef)getInteractor;

@end

#endif /* vtkCocoaGLView_h */
// VTK-HeaderTest-Exclude: vtkCocoaGLView.h
