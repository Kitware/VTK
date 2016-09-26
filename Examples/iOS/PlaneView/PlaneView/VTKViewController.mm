/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#import "VTKViewController.h"

#import <vtk/vtkActor.h>
#import <vtk/vtkCamera.h>
#import <vtk/vtkCommand.h>
#import <vtk/vtkCommand.h>
#import <vtk/vtkDebugLeaks.h>
#import <vtk/vtkIOSRenderWindow.h>
#import <vtk/vtkIOSRenderWindow.h>
#import <vtk/vtkIOSRenderWindowInteractor.h>
#import <vtk/vtkIOSRenderWindowInteractor.h>
#import <vtk/vtkImageData.h>
#import <vtk/vtkInteractorStyleMultiTouchCamera.h>
#import <vtk/vtkNew.h>
#import <vtk/vtkOutlineFilter.h>
#import <vtk/vtkPlaneWidget.h>
#import <vtk/vtkPolyData.h>
#import <vtk/vtkPolyDataMapper.h>
#import <vtk/vtkProbeFilter.h>
#import <vtk/vtkRTAnalyticSource.h>
#import <vtk/vtkRenderer.h>
#import <vtk/vtkRenderingOpenGL2ObjectFactory.h>
#import <vtk/vtkStructuredGridOutlineFilter.h>
#import <vtk/vtkUnstructuredGrid.h>
#import <vtk/vtkXMLImageDataReader.h>
#import <vtk/vtkXMLRectilinearGridReader.h>
#import <vtk/vtkXMLStructuredGridReader.h>
#import <vtk/vtkXMLUnstructuredGridReader.h>

// This does the actual work: updates the probe.
// Callback for the interaction
class vtkTPWCallback : public vtkCommand
{
public:
  static vtkTPWCallback *New()
  { return new vtkTPWCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
  {
    vtkPlaneWidget *planeWidget = reinterpret_cast<vtkPlaneWidget*>(caller);
    planeWidget->GetPolyData(this->PolyData);
    this->Actor->VisibilityOn();
  }
  vtkTPWCallback():PolyData(0),Actor(0) {}
  vtkPolyData *PolyData;
  vtkActor *Actor;
};


@interface VTKViewController ()
{
}

@property (strong, nonatomic) EAGLContext *context;

- (void)tearDownGL;

@end

@implementation VTKViewController

- (void)setProbeEnabled:(bool)val
{
  self->PlaneWidget->SetEnabled(val ? 1 : 0);
}

- (bool)getProbeEnabled
{
  return (self->PlaneWidget->GetEnabled() ? true : false);
}

//----------------------------------------------------------------------------
- (vtkIOSRenderWindowInteractor *)getInteractor
{
  if (self->RenderWindow)
  {
    return (vtkIOSRenderWindowInteractor *)self->RenderWindow->GetInteractor();
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

  self->RenderWindow = vtkIOSRenderWindow::New();
  self->Renderer = vtkRenderer::New();
  self->RenderWindow->AddRenderer(self->Renderer);

  // this example uses VTK's built in interaction but you could choose
  // to use your own instead.
  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(self->RenderWindow);

  vtkInteractorStyleMultiTouchCamera *ismt = vtkInteractorStyleMultiTouchCamera::New();
  iren->SetInteractorStyle(ismt);
  ismt->Delete();

  vtkNew<vtkPolyData> plane;
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->Update();

  self->Probe = vtkProbeFilter::New();
  self->Probe->SetInputData(plane.Get());
  self->Probe->SetSourceData(wavelet->GetOutput());

  self->ProbeMapper = vtkPolyDataMapper::New();
  self->ProbeMapper->SetInputConnection(self->Probe->GetOutputPort());
  double tmp[2];
  wavelet->GetOutput()->GetScalarRange(tmp);
  self->ProbeMapper->SetScalarRange(tmp[0], tmp[1]);

  vtkNew<vtkActor> probeActor;
  probeActor->SetMapper(self->ProbeMapper);
  probeActor->VisibilityOff();

  // An outline is shown for context.
  vtkNew<vtkOutlineFilter> outline;
  outline->SetInputData(wavelet->GetOutput());

  self->OutlineMapper = vtkPolyDataMapper::New();
  self->OutlineMapper->SetInputConnection(outline->GetOutputPort());

  vtkNew<vtkActor> outlineActor;
  outlineActor->SetMapper(self->OutlineMapper);

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  self->PlaneCallback = vtkTPWCallback::New();
  self->PlaneCallback->PolyData = plane.Get();
  self->PlaneCallback->Actor = probeActor.Get();

  // The plane widget is used probe the dataset.
  vtkNew<vtkPlaneWidget> planeWidget;
  planeWidget->SetInteractor(iren);
  planeWidget->SetDefaultRenderer(self->Renderer);
  planeWidget->SetInputData(wavelet->GetOutput());
  planeWidget->NormalToXAxisOn();
  planeWidget->SetResolution(30);
  planeWidget->SetHandleSize(0.07);
  planeWidget->SetRepresentationToOutline();
  planeWidget->PlaceWidget();
  planeWidget->AddObserver(vtkCommand::InteractionEvent,self->PlaneCallback);
  planeWidget->On();
  self->PlaneWidget = planeWidget.Get();
  self->PlaneCallback->Execute(self->PlaneWidget,0,NULL);
  planeWidget->Register(0);

  self->Renderer->AddActor(outlineActor.Get());
  self->Renderer->AddActor(probeActor.Get());
  self->Renderer->SetBackground(0.3,0.5,0.4);
}

- (void)setNewDataFile:(NSURL *)url
{
  vtkXMLDataReader *reader = NULL;
  vtkPolyDataAlgorithm *outline = NULL;

  // setup the reader and outline filter based
  // on data type
  if ([url.pathExtension isEqual:@"vtu"])
  {
    reader = vtkXMLUnstructuredGridReader::New();
    outline = vtkOutlineFilter::New();
  }
  if ([url.pathExtension isEqual:@"vts"])
  {
    reader = vtkXMLStructuredGridReader::New();
    outline = vtkStructuredGridOutlineFilter::New();
  }
  if ([url.pathExtension isEqual:@"vtr"])
  {
    reader = vtkXMLRectilinearGridReader::New();
    outline = vtkOutlineFilter::New();
  }
  if ([url.pathExtension isEqual:@"vti"])
  {
    reader = vtkXMLImageDataReader::New();
    outline = vtkOutlineFilter::New();
  }

  if (!reader || !outline)
  {
    return;
  }

  reader->SetFileName([url.path cStringUsingEncoding:NSASCIIStringEncoding]);
  reader->Update();
  self->Probe->SetSourceData(reader->GetOutputDataObject(0));
  double tmp[2];
    vtkDataSet *ds = vtkDataSet::SafeDownCast(reader->GetOutputDataObject(0));
    ds->GetScalarRange(tmp);
  self->ProbeMapper->SetScalarRange(tmp[0], tmp[1]);
  outline->SetInputData(ds);
  self->OutlineMapper->SetInputConnection(outline->GetOutputPort(0));
  self->PlaneWidget->SetInputData(ds);
  self->PlaneWidget->PlaceWidget(ds->GetBounds());

  self->PlaneCallback->Execute(self->PlaneWidget,0,NULL);

  self->Renderer->ResetCamera();
  self->RenderWindow->Render();
  self->PlaneWidget->PlaceWidget(ds->GetBounds());
  self->RenderWindow->Render();
  reader->Delete();
  outline->Delete();
}

- (void)viewDidLoad
{
  [super viewDidLoad];

  self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];

  if (!self.context)
  {
    NSLog(@"Failed to create ES context");
    return;
  }

  UITapGestureRecognizer *tapRecognizer = [[UITapGestureRecognizer alloc]
     initWithTarget:self action:@selector(handleTap:)];
  tapRecognizer.numberOfTapsRequired = 2;
  [self.view addGestureRecognizer:tapRecognizer];


  GLKView *view = (GLKView *)self.view;
  view.context = self.context;
  view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
  //view.drawableMultisample = GLKViewDrawableMultisample4X;

  // setup the vis pipeline
  [self setupPipeline];

  [EAGLContext setCurrentContext:self.context];
  [self resizeView];
  self->RenderWindow->Render();
}

- (void)dealloc
{
  [self tearDownGL];

  if ([EAGLContext currentContext] == self.context)
  {
    [EAGLContext setCurrentContext:nil];
  }
}

- (void)didReceiveMemoryWarning
{
  [super didReceiveMemoryWarning];

  if ([self isViewLoaded] && ([[self view] window] == nil))
  {
    self.view = nil;

    [self tearDownGL];

    if ([EAGLContext currentContext] == self.context)
    {
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
  self->RenderWindow->SetSize(self.view.bounds.size.width*scale, self.view.bounds.size.height*scale);
}

- (void)viewWillLayoutSubviews
{
  [self resizeView];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
  self->RenderWindow->Render();
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
  double scale = self.view.contentScaleFactor;

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
      interactor->SetEventInformation((int)round(location.x*scale),
                                  (int)round(location.y*scale),
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
  double scale = self.view.contentScaleFactor;

  // set the position for all contacts
  int index = -1;
  NSSet *myTouches = [event touchesForView:self.view];
  for (UITouch *touch in myTouches)
  {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;

    index = interactor->GetPointerIndexForContact((size_t)(__bridge void *)touch);
    if (index < VTKI_MAX_POINTERS)
    {
      interactor->SetEventInformation((int)round(location.x*scale),
                                  (int)round(location.y*scale),
                                  0, 0,
                                  0, 0, 0, index);
    }
  }

  // fire move event on last index
  if (index > -1)
  {
    interactor->SetPointerIndex(index);
    interactor->MouseMoveEvent();
  }

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
  double scale = self.view.contentScaleFactor;

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
      interactor->SetEventInformation((int)round(location.x*scale),
                                  (int)round(location.y*scale),
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
  double scale = self.view.contentScaleFactor;

  for (UITouch *touch in touches)
  {
    int index = interactor->GetPointerIndexForContact((size_t)(__bridge void *)touch);
    interactor->SetPointerIndex(index);
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self.view];
    location.y = bounds.size.height - location.y;
    interactor->SetEventInformation((int)round(location.x*scale),
                                    (int)round(location.y*scale),
                                    0, 0,
                                    0, 0);
    interactor->LeftButtonReleaseEvent();
    interactor->ClearContact((size_t)(__bridge void *)touch);
  }
}

- (void)handleTap:(UITapGestureRecognizer *)sender
{
  if (sender.state == UIGestureRecognizerStateEnded)
  {
    vtkIOSRenderWindowInteractor *interactor = [self getInteractor];
    if (!interactor)
    {
      return;
    }
    self->Renderer->ResetCamera();
    self->RenderWindow->Render();
  }
}

@end
