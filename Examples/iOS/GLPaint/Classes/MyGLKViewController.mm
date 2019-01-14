/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import "MyGLKViewController.h"
#import "vtkIOSRenderWindow.h"
#import "vtkIOSRenderWindowInteractor.h"
#include "vtkRenderingOpenGL2ObjectFactory.h"

#include "vtkNew.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkDebugLeaks.h"
#include "vtkGlyph3D.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkIOSRenderWindow.h"
#include "vtkIOSRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkCommand.h"
#include "vtkInteractorStyleMultiTouchCamera.h"


@interface MyGLKViewController () {
}

@property (strong, nonatomic) EAGLContext *context;

- (void)tearDownGL;

@end

@implementation MyGLKViewController

//----------------------------------------------------------------------------
- (vtkIOSRenderWindow *)getVTKRenderWindow
{
  return _myVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (void)setVTKRenderWindow:(vtkIOSRenderWindow *)theVTKRenderWindow
{
  _myVTKRenderWindow = theVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (vtkIOSRenderWindowInteractor *)getInteractor
{
  if (_myVTKRenderWindow)
  {
    return (vtkIOSRenderWindowInteractor *)_myVTKRenderWindow->GetInteractor();
  }
  else
  {
    return NULL;
  }
}

- (void)setupPipeline
{
  vtkRenderingOpenGL2ObjectFactory *of = vtkRenderingOpenGL2ObjectFactory::New();
  vtkObjectFactory::RegisterFactory(of);

  vtkIOSRenderWindow *renWin = vtkIOSRenderWindow::New();
  //renWin->DebugOn();
  [self setVTKRenderWindow:renWin];

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer.Get());


  // this example uses VTK's built in interaction but you could choose
  // to use your own instead.
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkInteractorStyleMultiTouchCamera *ismt = vtkInteractorStyleMultiTouchCamera::New();
  iren->SetInteractorStyle(ismt);
  ismt->Delete();

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper.Get());

  vtkNew<vtkConeSource> cone;
  cone->SetResolution(6);

  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  vtkNew<vtkPolyDataMapper> spikeMapper;
  spikeMapper->SetInputConnection(glyph->GetOutputPort());

  vtkNew<vtkActor> spikeActor;
  spikeActor->SetMapper(spikeMapper.Get());

  renderer->AddActor(sphereActor.Get());
  renderer->AddActor(spikeActor.Get());
  renderer->SetBackground(0.4,0.5,0.6);
}


- (void)viewDidLoad
{
  [super viewDidLoad];

  self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];

  if (!self.context) {
      NSLog(@"Failed to create ES context");
  }

  GLKView *view = (GLKView *)self.view;
  view.context = self.context;
  view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
  //view.drawableMultisample = GLKViewDrawableMultisample4X;

  // setup the vis pipeline
  [self setupPipeline];

  [EAGLContext setCurrentContext:self.context];
  [self resizeView];
  [self getVTKRenderWindow]->Render();
}


- (void)dealloc
{
  [self tearDownGL];

  if ([EAGLContext currentContext] == self.context) {
      [EAGLContext setCurrentContext:nil];
  }
}

- (void)didReceiveMemoryWarning
{
  [super didReceiveMemoryWarning];

  if ([self isViewLoaded] && ([[self view] window] == nil)) {
    self.view = nil;

    [self tearDownGL];

    if ([EAGLContext currentContext] == self.context) {
        [EAGLContext setCurrentContext:nil];
    }
    self.context = nil;
  }

  // Dispose of any resources that can be recreated.
}

- (void)tearDownGL
{
  [EAGLContext setCurrentContext:self.context];

  // free GL resources
  // ...
}

-(void) resizeView
{
  double scale = self.view.contentScaleFactor;
  [self getVTKRenderWindow]->SetSize(self.view.bounds.size.width*scale, self.view.bounds.size.height*scale);
}

- (void)viewWillLayoutSubviews
{
  [self resizeView];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
  [self getVTKRenderWindow]->Render();
}



//=================================================================
// this example uses VTK's built in interaction but you could choose
// to use your own instead. The remaining methods forward touch events
// to VTKs interactor.

// Handles the start of a touch
- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event
{
  vtkIOSRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
  {
    return;
  }

  CGRect bounds = [self.view bounds];

  // set the position for all contacts
  NSSet *myTouches = [event touchesForView:self.view];
  for (UITouch *touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

    int index = interactor->GetPointerIndexForContact((size_t)(__bridge void *)touch);
    if (index < VTKI_MAX_POINTERS)
    {
      interactor->SetEventInformation((int)round(location.x),
                                  (int)round(location.y),
                                  0, 0,
                                  0, 0, 0, index);
    }
  }

  // handle begin events
  for (UITouch *touch in touches)
  {
    int index = interactor->GetPointerIndexForContact((size_t)(__bridge void *)touch);
    interactor->SetPointerIndex(index);
    interactor->LeftButtonPressEvent();
    NSLog(@"Starting left mouse");
  }

  // Display the buffer
  [(GLKView *)self.view display];
}

// Handles the continuation of a touch.
- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event
{
  vtkIOSRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
  {
    return;
  }

  CGRect        bounds = [self.view bounds];

  // set the position for all contacts
  int index;
  NSSet *myTouches = [event touchesForView:self.view];
  for (UITouch *touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

    index = interactor->GetPointerIndexForContact((size_t)(__bridge void *)touch);
    if (index < VTKI_MAX_POINTERS)
    {
      interactor->SetEventInformation((int)round(location.x),
                                  (int)round(location.y),
                                  0, 0,
                                  0, 0, 0, index);
    }
  }

  // fire move event on last index
  interactor->SetPointerIndex(index);
  interactor->MouseMoveEvent();
  //  NSLog(@"Moved left mouse");

  // Display the buffer
  [(GLKView *)self.view display];
}

// Handles the end of a touch event when the touch is a tap.
- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event
{
  vtkIOSRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
  {
    return;
  }

  CGRect        bounds = [self.view bounds];
  // set the position for all contacts
  NSSet *myTouches = [event touchesForView:self.view];
  for (UITouch *touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

    int index = interactor->GetPointerIndexForContact((size_t)(__bridge void *)touch);
    if (index < VTKI_MAX_POINTERS)
    {
      interactor->SetEventInformation((int)round(location.x),
                                  (int)round(location.y),
                                  0, 0,
                                  0, 0, 0, index);
    }
  }

  // handle begin events
  for (UITouch *touch in touches)
  {
    int index = interactor->GetPointerIndexForContact((size_t)(__bridge void *)touch);
    interactor->SetPointerIndex(index);
    interactor->LeftButtonReleaseEvent();
    interactor->ClearContact((size_t)(__bridge void *)touch);
    NSLog(@"lifting left mouse");
  }

  // Display the buffer
  [(GLKView *)self.view display];
}

// Handles the end of a touch event.
- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
  vtkIOSRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
  {
    return;
  }

  CGRect        bounds = [self.view bounds];
  UITouch*            touch = [[event touchesForView:self.view] anyObject];
  // Convert touch point from UIView referential to OpenGL one (upside-down flip)
  CGPoint location = [touch locationInView:self.view];
  location.y = bounds.size.height - location.y;

  interactor->SetEventInformation((int)round(location.x),
                                  (int)round(location.y),
                                  0, 0,
                                  0, 0);
  interactor->LeftButtonReleaseEvent();
  // NSLog(@"Ended left mouse");

  // Display the buffer
  [(GLKView *)self.view display];
}


@end
