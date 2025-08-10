/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaHardwareView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkCocoaHardwareView
 * @brief
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
 * To provide file dropping support, it implements the following methods:
 * draggingEntered: and performDragOperation:.
 * To be able to render and draw onscreen, it overrides drawRect:.
 *
 * @sa
 * vtkCocoaHardwareWindow vtkCocoaRenderWindowInteractor
 */

#ifndef vtkCocoaHardwareView_h
#define vtkCocoaHardwareView_h

#import "vtkRenderingUIModule.h" // For export macro
#import <Cocoa/Cocoa.h>

// Note: This file should be includable by both pure Objective-C and Objective-C++ source files.
// To achieve this, we use the neat technique below:
#ifdef __cplusplus
// Forward declarations
VTK_ABI_NAMESPACE_BEGIN
class vtkCocoaHardwareWindow;
class vtkCocoaRenderWindowInteractor;

// Type declarations
typedef vtkCocoaHardwareWindow* vtkCocoaHardwareWindowRef;
typedef vtkCocoaRenderWindowInteractor* vtkCocoaRenderWindowInteractorRef;

VTK_ABI_NAMESPACE_END
#else
// Type declarations
typedef void* vtkCocoaHardwareWindowRef;
typedef void* vtkCocoaRenderWindowInteractorRef;
#endif

VTKRENDERINGUI_EXPORT
@interface vtkCocoaHardwareView : NSView<NSDraggingDestination>
{
@private
  vtkCocoaHardwareWindowRef _myHardwareWindow;
  NSTrackingArea* _rolloverTrackingArea;
}

- (vtkCocoaHardwareWindowRef)getHardwareWindow;
- (void)setHardwareWindow:(vtkCocoaHardwareWindowRef)theHardwareWindow;

- (vtkCocoaRenderWindowInteractorRef)getInteractor;
@end

#endif /* vtkCocoaHardwareView_h */
// VTK-HeaderTest-Exclude: vtkCocoaHardwareView.h
