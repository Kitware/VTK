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

#include "vtkParametricTorus.h"
#include "vtkParametricBoy.h"
#include "vtkParametricConicSpiral.h"
#include "vtkParametricCrossCap.h"
#include "vtkParametricDini.h"
#include "vtkParametricEllipsoid.h"
#include "vtkParametricEnneper.h"
#include "vtkParametricFigure8Klein.h"
#include "vtkParametricKlein.h"
#include "vtkParametricMobius.h"
#include "vtkParametricRandomHills.h"
#include "vtkParametricRoman.h"
#include "vtkParametricSpline.h"
#include "vtkParametricSuperEllipsoid.h"
#include "vtkParametricSuperToroid.h"
#include "vtkParametricTorus.h"
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkActor2D.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkInteractorStyleMultiTouchCamera.h"
#include "vtkMath.h"
#include "vtkParametricFunctionSource.h"
#include "vtkPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderer.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"

#include <deque>

/* 2 or 3 -- needs to match VTK version */
#define GL_ES_VERSION 2


@interface MyGLKViewController () {
  std::deque<vtkSmartPointer<vtkParametricFunction> > parametricObjects;
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

- (void)initializeParametricObjects
{
  parametricObjects.push_back(vtkSmartPointer<vtkParametricBoy>::New());
  parametricObjects.push_back(vtkSmartPointer<vtkParametricConicSpiral>::New());
  parametricObjects.push_back(vtkSmartPointer<vtkParametricCrossCap>::New());
  parametricObjects.push_back(vtkSmartPointer<vtkParametricDini>::New());

  auto ellipsoid = vtkSmartPointer<vtkParametricEllipsoid>::New();
  ellipsoid->SetXRadius(0.5);
  ellipsoid->SetYRadius(2.0);
  parametricObjects.push_back(ellipsoid);

  parametricObjects.push_back(vtkSmartPointer<vtkParametricEnneper>::New());
  parametricObjects.push_back(vtkSmartPointer<vtkParametricFigure8Klein>::New());
  parametricObjects.push_back(vtkSmartPointer<vtkParametricKlein>::New());
  auto mobius = vtkSmartPointer<vtkParametricMobius>::New();
  mobius->SetRadius(2.0);
  mobius->SetMinimumV(-0.5);
  mobius->SetMaximumV(0.5);
  parametricObjects.push_back(mobius);

  vtkSmartPointer<vtkParametricRandomHills> randomHills =
    vtkSmartPointer<vtkParametricRandomHills>::New();
  randomHills->AllowRandomGenerationOff();
  parametricObjects.push_back(randomHills);

  parametricObjects.push_back(vtkSmartPointer<vtkParametricRoman>::New());

  auto superEllipsoid = vtkSmartPointer<vtkParametricSuperEllipsoid>::New();
  superEllipsoid->SetN1(0.5);
  superEllipsoid->SetN2(0.1);
  parametricObjects.push_back(superEllipsoid);

  auto superToroid = vtkSmartPointer<vtkParametricSuperToroid>::New();
  superToroid->SetN1(0.2);
  superToroid->SetN2(3.0);
  parametricObjects.push_back(superToroid);

  parametricObjects.push_back(vtkSmartPointer<vtkParametricTorus>::New());

  // The spline needs points
  vtkSmartPointer<vtkParametricSpline> spline =
    vtkSmartPointer<vtkParametricSpline>::New();
  vtkSmartPointer<vtkPoints> inputPoints =
    vtkSmartPointer<vtkPoints>::New();
  vtkMath::RandomSeed(8775070);
  for (int p = 0; p < 10; p++)
  {
    double x = vtkMath::Random(0.0, 1.0);
    double y = vtkMath::Random(0.0, 1.0);
    double z = vtkMath::Random(0.0, 1.0);
    inputPoints->InsertNextPoint(x, y, z);
  }
  spline->SetPoints(inputPoints);
  parametricObjects.push_back(spline);
}

- (void)setupPipeline
{
  // Register GL2 objects
  vtkObjectFactory::RegisterFactory(vtkRenderingOpenGL2ObjectFactory::New());

  //
  // Create the parametric objects
  //
  [self initializeParametricObjects];

  std::vector<vtkSmartPointer<vtkParametricFunctionSource> > parametricFunctionSources;
  std::vector<vtkSmartPointer<vtkRenderer> > renderers;
  std::vector<vtkSmartPointer<vtkPolyDataMapper> > mappers;
  std::vector<vtkSmartPointer<vtkActor> > actors;

  // No text mappers/actors in VTK GL2 yet
#if 0
  //std::vector<vtkSmartPointer<vtkTextMapper> > textmappers;
  //std::vector<vtkSmartPointer<vtkActor2D> > textactors;

  // Create one text property for all
  //vtkSmartPointer<vtkTextProperty> textProperty =
  //vtkSmartPointer<vtkTextProperty>::New();
  //textProperty->SetFontSize(10);
  //textProperty->SetJustificationToCentered();
#endif

  // Create a parametric function source, renderer, mapper, and actor
  // for each object
  for(unsigned int i = 0; i < parametricObjects.size(); i++)
  {
    parametricFunctionSources.push_back(vtkSmartPointer<vtkParametricFunctionSource>::New());
    parametricFunctionSources[i]->SetParametricFunction(parametricObjects[i]);
    parametricFunctionSources[i]->Update();

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    assert(mapper);
    mappers.push_back(mapper);
    mappers[i]->SetInputConnection(parametricFunctionSources[i]->GetOutputPort());

    actors.push_back(vtkSmartPointer<vtkActor>::New());
    actors[i]->SetMapper(mappers[i]);

    // No text mappers/actors in VTK GL2 yet
#if 0
    textmappers.push_back(vtkSmartPointer<vtkTextMapper>::New());
    textmappers[i]->SetInput(parametricObjects[i]->GetClassName());
    textmappers[i]->SetTextProperty(textProperty);

    textactors.push_back(vtkSmartPointer<vtkActor2D>::New());
    textactors[i]->SetMapper(textmappers[i]);
    textactors[i]->SetPosition(100, 16);
#endif
    renderers.push_back(vtkSmartPointer<vtkRenderer>::New());
  }
  unsigned int gridDimensions = 4;

  // Need a renderer even if there is no actor
  for(size_t i = parametricObjects.size();
      i < gridDimensions * gridDimensions;
      i++)
  {
    renderers.push_back(vtkSmartPointer<vtkRenderer>::New());
  }

  vtkIOSRenderWindow *renWin = vtkIOSRenderWindow::New();
  //renWin->DebugOn();
  [self setVTKRenderWindow:renWin];

  int rendererSize = 200;
  renWin->SetSize(rendererSize*gridDimensions, rendererSize*gridDimensions);

  for(int row = 0; row < static_cast<int>(gridDimensions); row++)
  {
    for(int col = 0; col < static_cast<int>(gridDimensions); col++)
    {
      int index = row*gridDimensions + col;

      // (xmin, ymin, xmax, ymax)
      double viewport[4] =
      {static_cast<double>(col) * rendererSize / (gridDimensions * rendererSize),
        static_cast<double>(gridDimensions - (row+1)) * rendererSize / (gridDimensions * rendererSize),
        static_cast<double>(col+1)*rendererSize / (gridDimensions * rendererSize),
        static_cast<double>(gridDimensions - row) * rendererSize / (gridDimensions * rendererSize)};

      renWin->AddRenderer(renderers[index]);
      renderers[index]->SetViewport(viewport);
      if(index > static_cast<int>(parametricObjects.size() - 1))
      {
        continue;
      }
      renderers[index]->AddActor(actors[index]);
      //renderers[index]->AddActor(textactors[index]);
      renderers[index]->SetBackground(.2, .3, .4);
      renderers[index]->ResetCamera();
      renderers[index]->GetActiveCamera()->Azimuth(30);
      renderers[index]->GetActiveCamera()->Elevation(-30);
      renderers[index]->GetActiveCamera()->Zoom(0.9);
      renderers[index]->ResetCameraClippingRange();
    }
  }

  // this example uses VTK's built in interaction but you could choose
  // to use your own instead.
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkInteractorStyleMultiTouchCamera *ismt = vtkInteractorStyleMultiTouchCamera::New();
  iren->SetInteractorStyle(ismt);
  ismt->Delete();
  iren->Start();
}


- (void)viewDidLoad
{
  [super viewDidLoad];

#if GL_ES_VERSION == 2
  self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];
#else
  self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
#endif

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
  double newWidth = scale * self.view.bounds.size.width;
  double newHeight = scale * self.view.bounds.size.height;
  [self getVTKRenderWindow]->SetSize(newWidth, newHeight);
}

- (void)viewWillLayoutSubviews
{
  [self resizeView];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
  //std::cout << [self getVTKRenderWindow]->ReportCapabilities() << std::endl;
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

  vtkIOSRenderWindow *renWin = [self getVTKRenderWindow];
  if (!renWin)
  {
    return;
  }

  CGRect bounds = [self.view bounds];
  double scale = self.view.contentScaleFactor;

  // set the position for all contacts
  NSSet *myTouches = [event touchesForView:self.view];
  for (UITouch *touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

    // Account for the content scaling factor
    location.x *= scale;
    location.y *= scale;

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
    interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
    //NSLog(@"Starting left mouse");
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

  vtkIOSRenderWindow *renWin = [self getVTKRenderWindow];
  if (!renWin)
  {
    return;
  }

  CGRect bounds = [self.view bounds];
  double scale = self.view.contentScaleFactor;

  // set the position for all contacts
  int index;
  NSSet *myTouches = [event touchesForView:self.view];
  for (UITouch *touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

    // Account for the content scaling factor
    location.x *= scale;
    location.y *= scale;

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
  interactor->InvokeEvent(vtkCommand::MouseMoveEvent,NULL);
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

  vtkIOSRenderWindow *renWin = [self getVTKRenderWindow];
  if (!renWin)
  {
    return;
  }

  CGRect bounds = [self.view bounds];
  double scale = self.view.contentScaleFactor;

  // set the position for all contacts
  NSSet *myTouches = [event touchesForView:self.view];
  for (UITouch *touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

    // Account for the content scaling factor
    location.x *= scale;
    location.y *= scale;

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
    interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
    interactor->ClearContact((size_t)(__bridge void *)touch);
      //NSLog(@"lifting left mouse");
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

  vtkIOSRenderWindow *renWin = [self getVTKRenderWindow];
  if (!renWin)
  {
    return;
  }

  CGRect bounds = [self.view bounds];
  double scale = self.view.contentScaleFactor;

  UITouch* touch = [[event touchesForView:self.view] anyObject];
  // Convert touch point from UIView referential to OpenGL one (upside-down flip)
  CGPoint location = [touch locationInView:self.view];
  location.y = bounds.size.height - location.y;

  // Account for the content scaling factor
  location.x *= scale;
  location.y *= scale;

  interactor->SetEventInformation((int)round(location.x),
                                  (int)round(location.y),
                                  0, 0,
                                  0, 0);
  interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
  // NSLog(@"Ended left mouse");

  // Display the buffer
  [(GLKView *)self.view display];
}


@end
