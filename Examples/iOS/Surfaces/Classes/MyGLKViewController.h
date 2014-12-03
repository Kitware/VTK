/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

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

@interface MyGLKViewController : GLKViewController
{
  @private
  vtkIOSRenderWindowRef _myVTKRenderWindow;
}

@property (nonatomic, strong) UIWindow *window;

- (vtkIOSRenderWindowRef)getVTKRenderWindow;
- (void)setVTKRenderWindow:(vtkIOSRenderWindowRef)theVTKRenderWindow;

- (vtkIOSRenderWindowInteractorRef)getInteractor;

- (void)initializeParametricObjects;
- (void)setupPipeline;

@end
