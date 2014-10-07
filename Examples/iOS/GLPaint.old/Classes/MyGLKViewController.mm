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

@interface MyGLKViewController () {

  vesKiwiViewerApp::Ptr mKiwiApp;

  MyGestureHandler* mGestureHandler;
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

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer.Get());

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

  self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

  if (!self.context) {
      NSLog(@"Failed to create ES context");
  }

  GLKView *view = (GLKView *)self.view;
  view.context = self.context;
  view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
  view.drawableMultisample = GLKViewDrawableMultisample4X;

  // setup the vis pipeline
  [self setupPipeline];

  [self getVTKRenderWindow]->SetSize(backingWidth, backingHeight);
  [self getVTKRenderWindow]->Render();
}

-(void) initializeKiwiApp
{
  [EAGLContext setCurrentContext:self.context];

  std::string dataset = [[[NSBundle mainBundle] pathForResource:@"teapot" ofType:@"vtp"] UTF8String];

  self->mKiwiApp = vesKiwiViewerApp::Ptr(new vesKiwiViewerApp);
  self->mKiwiApp->initGL();
  [self resizeView];
  self->mKiwiApp->loadDataset(dataset);
  self->mKiwiApp->resetView();
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

@end
