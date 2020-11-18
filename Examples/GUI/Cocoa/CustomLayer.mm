#import "CustomLayer.h"

#import "vtkCocoaRenderWindow.h"
#import "vtkCocoaRenderWindowInteractor.h"
#import "vtkRenderWindow.h"
#import "vtkRenderWindowInteractor.h"
#import "vtkRenderer.h"

@implementation CustomLayer

// ---------------------------------------------------------------------------------------------------------------
// This is CAOpenGLLayer's entry point for drawing. The OS calls it when it's time to draw.
- (void)drawInCGLContext:(CGLContextObj)inCGLContext
             pixelFormat:(CGLPixelFormatObj)inPixelFormat
            forLayerTime:(CFTimeInterval)inTimeInterval
             displayTime:(nullable const CVTimeStamp*)inTimeStamp
{
  assert(inCGLContext);
  assert(inPixelFormat);

  // Get the related view.
  CustomView* customView = [self customView];
  assert(customView);

  // Tell VTK to render.
  assert([customView renderWindowInteractor] -> GetInitialized());
  vtkCocoaRenderWindow* renderWindow = [customView renderWindow];
  if (renderWindow && renderWindow->GetMapped())
  {
    bool contextInitialised = renderWindow->Superclass::InitializeFromCurrentContext();
    assert(contextInitialised);
    (void)contextInitialised;
    renderWindow->Render();
  }

  // Generally, subclasses should call the superclass implementation of the method to flush the
  // context after rendering. But VTK itself flushes, so it's probably not necessary to call super.
#if 0
  [super drawInCGLContext:inCGLContext
              pixelFormat:inPixelFormat
             forLayerTime:inTimeInterval
              displayTime:inTimeStamp];
#endif
}

// ---------------------------------------------------------------------------------------------------------------
// CAOpenGLLayer triggers this function when a context is needed by the receiver. Here we return the
// VTK context.
- (CGLContextObj)copyCGLContextForPixelFormat:(CGLPixelFormatObj)inPixelFormat
{
  assert(inPixelFormat);
  (void)inPixelFormat;

  // Get the related view.
  CustomView* customView = [self customView];
  assert(customView);

  // Fetch the rendering window's context.
  vtkCocoaRenderWindow* renderWindow = [customView renderWindow];
  assert(renderWindow);

  // Get the OpenGL context from VTK.
  assert([customView renderWindowInteractor] -> GetInitialized());
  NSOpenGLContext* openGLContext = (__bridge NSOpenGLContext*)(renderWindow->GetContextId());
  assert(openGLContext);

  // Convert to CGLContextObj.
  CGLContextObj cglContext = (CGLContextObj)[openGLContext CGLContextObj];
  assert(cglContext);

  return cglContext;
}

// ---------------------------------------------------------------------------------------------------------------
- (void)releaseCGLContext:(CGLContextObj)inContext
{
  (void)inContext;
}

// ---------------------------------------------------------------------------------------------------------------
- (void)releaseCGLPixelFormat:(CGLPixelFormatObj)inPixelFormat
{
  (void)inPixelFormat;
}

@end
