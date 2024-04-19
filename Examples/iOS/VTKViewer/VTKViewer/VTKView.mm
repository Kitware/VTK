//
//  VTKView.m
//
//  Created by Alexis Girault on 4/3/17.
//

#import "VTKView.h"

#include "vtkCamera.h"
#include "vtkIOSRenderWindow.h"
#include "vtkIOSRenderWindowInteractor.h"
#include "vtkInteractorStyleMultiTouchCamera.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkRenderingOpenGL2ObjectFactory.h"

@interface VTKView ()

@end

@implementation VTKView

- (id)initWithCoder:(NSCoder*)aDecoder
{
  if (self == [super initWithCoder:aDecoder])
  {
    [self initializeVTK];
  }
  return self;
}

- (id)initWithFrame:(CGRect)frame
{
  if (self = [super initWithFrame:frame])
  {
    [self initializeVTK];
  }
  return self;
}

- (void)dealloc
{
  if (self.renderWindow)
  {
    self.renderWindow->Delete();
    _renderWindow = nil;
  }

  if ([EAGLContext currentContext] == self.context)
  {
    [EAGLContext setCurrentContext:nil];
  }
}

- (void)layoutSubviews
{
  [super layoutSubviews];
  [self resizeView];
}

- (void)initializeVTK
{
  // Register VTK GL2 objects
  vtkObjectFactory::RegisterFactory(vtkRenderingOpenGL2ObjectFactory::New());

  // GL ES 3.0 is required for this app
  self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES3];
  if (!self.context)
  {
    NSLog(@"Failed to create OpenGL ES 3.0 context.");
  }
  self.drawableDepthFormat = GLKViewDrawableDepthFormat24;
  [EAGLContext setCurrentContext:self.context];

  // Enable the use of `setNeedsDisplay` to render
  self.enableSetNeedsDisplay = YES;

  // Create Render window and interactor
  _renderWindow = vtkIOSRenderWindow::New();
  auto interactor = self.renderWindow->MakeRenderWindowInteractor();
  auto camera = vtkSmartPointer<vtkInteractorStyleMultiTouchCamera>::New();
  interactor->SetInteractorStyle(camera);
  interactor->EnableRenderOff();
  self.renderWindow->SetInteractor(interactor);

  // Needed to ensure that rotations look smooth
  self.contentMode = UIViewContentModeCenter;
}

- (void)resizeView
{
  if (!self.renderWindow)
  {
    return;
  }

  CGFloat scale = super.contentScaleFactor;
  CGFloat width = scale * super.bounds.size.width;
  CGFloat height = scale * super.bounds.size.height;

  // A VTK camera maintains its vertical view angle
  // when updating the size of a render window, while
  // iOS views will not. Zooming the camera according
  // to the ratio between current height and previous
  // height will ensure a smooth rotation of the view
  // if there is a unique viewport.
  CGFloat ratio = self.renderWindow->GetSize()[1] / height;
  auto renderers = self.renderWindow->GetRenderers();
  renderers->InitTraversal();
  while (auto renderer = renderers->GetNextItem())
  {
    if (auto cam = renderer->GetActiveCamera())
    {
      cam->Zoom(ratio);
    }
  }

  self.renderWindow->SetSize(width, height);
  [self setNeedsDisplay];
}

- (void)drawRect:(CGRect)rect
{
  if (!self.renderWindow)
  {
    return;
  }
  self.renderWindow->Render();
}

- (void)displayCoordinates:(int*)coordinates ofTouch:(CGPoint)touchPoint
{
  if (!self.renderWindow)
  {
    return;
  }
  const int* wh = self.renderWindow->GetSize();
  CGFloat scale = super.contentScaleFactor;
  coordinates[0] = scale * touchPoint.x;
  // UIKit => OpenGL == flip Y axis
  coordinates[1] = (wh[1] - scale * touchPoint.y);
}

- (void)normalizedCoordinates:(double*)coordinates ofTouch:(CGPoint)touch
{
  if (!self.renderWindow)
  {
    return;
  }
  const int* wh = self.renderWindow->GetSize();
  int display[2] = { 0 };
  [self displayCoordinates:display ofTouch:touch];
  coordinates[0] = (double)display[0] / wh[0];
  coordinates[1] = (double)display[1] / wh[1];
}

- (vtkIOSRenderWindowInteractor*)interactor
{
  return self.renderWindow ? (vtkIOSRenderWindowInteractor*)self.renderWindow->GetInteractor()
                           : nullptr;
}

@end
