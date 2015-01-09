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

  [self initializeKiwiApp];
  [self initializeGestureHandler];
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

-(void) initializeGestureHandler
{
  self->mGestureHandler = [[MyGestureHandler alloc] init];
  self->mGestureHandler.view = self.view;
  self->mGestureHandler.kiwiApp = self->mKiwiApp;
  [self->mGestureHandler createGestureRecognizers];
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
  self->mKiwiApp->resizeView(self.view.bounds.size.width*scale, self.view.bounds.size.height*scale);
}

- (void)viewWillLayoutSubviews
{
  [self resizeView];
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
  if (self->mKiwiApp && !self.paused) {
    self->mKiwiApp->render();
  }
}

@end
