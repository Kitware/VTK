/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCocoaWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import <Cocoa/Cocoa.h>

@class vtkCocoaGLView;

@interface vtkCocoaWindow : NSWindow
{
    @private
    IBOutlet vtkCocoaGLView *myvtkCocoaGLView;
    void *myVTKRenderWindow;
    void *myVTKRenderWindowInteractor;
}

// accessor and convenience

- (vtkCocoaGLView *)getvtkCocoaGLView;
- (void)setvtkCocoaGLView:vtkCocoaGLView;

- (void *)getVTKRenderWindow;
- (void)setVTKRenderWindow:(void *)theVTKRenderWindow;

- (void *)getVTKRenderWindowInteractor;
- (void)setVTKRenderWindowInteractor:(void *)theVTKRenderWindowInteractor;

- (void)makeCurrentContext;

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)proposedFrameSize;
- (BOOL)windowShouldZoom:(NSWindow *)sender toFrame:(NSRect)newFrame;
- (void)close; //close your face!

@end
