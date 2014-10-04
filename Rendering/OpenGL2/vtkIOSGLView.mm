/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOSGLView.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "vtkOpenGLRenderWindow.h"

#import "vtkIOSGLView.h"
#import "vtkIOSRenderWindow.h"
#import "vtkIOSRenderWindowInteractor.h"
#import "vtkCommand.h"

#include "vtkRenderingOpenGL2ObjectFactory.h"


@interface vtkIOSGLView()
{
  // The pixel dimensions of the backbuffer
  GLint backingWidth;
  GLint backingHeight;

  EAGLContext *context;

  // OpenGL names for the renderbuffer and framebuffers used to render to this view
  GLuint viewRenderbuffer, viewFramebuffer;

  // OpenGL name for the depth buffer that is attached to viewFramebuffer, if it exists (0 if it does not exist)
  GLuint depthRenderbuffer;

  BOOL initialized;
}

@end


@implementation vtkIOSGLView

//----------------------------------------------------------------------------
// Private
- (void)emptyMethod:(id)sender
{
  (void)sender;
}

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

// Implement this to override the default layer class (which is [CALayer class]).
// We do this so that our view will be backed by a layer that is capable of OpenGL ES rendering.
+ (Class)layerClass
{
  return [CAEAGLLayer class];
}

// The GL view is stored in the nib file. When it's unarchived it's sent -initWithCoder:
- (id)initWithCoder:(NSCoder*)coder {

    if ((self = [super initWithCoder:coder])) {
    CAEAGLLayer *eaglLayer = (CAEAGLLayer *)self.layer;

    eaglLayer.opaque = YES;
    // In this application, we want to retain the EAGLDrawable contents after a call to presentRenderbuffer.
    eaglLayer.drawableProperties =
      [NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithBool:YES],
          kEAGLDrawablePropertyRetainedBacking,
          kEAGLColorFormatRGBA8,
          kEAGLDrawablePropertyColorFormat,
          nil];

    context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!context || ![EAGLContext setCurrentContext:context])
      {
      return nil;
      }

    // Set the view's scale factor as you wish
    self.contentScaleFactor = [[UIScreen mainScreen] scale];

    self.multipleTouchEnabled = YES;
  }

  return self;
}

// If our view is resized, we'll be asked to layout subviews.
// This is the perfect opportunity to also update the framebuffer so that it is
// the same size as our display area.
-(void)layoutSubviews
{
  [EAGLContext setCurrentContext:context];

  if (!initialized)
    {
    initialized = [self initGL];
    }
  else
    {
    [self resizeFromLayer:(CAEAGLLayer*)self.layer];
    }
}

- (BOOL)initGL
{
  // Generate IDs for a framebuffer object and a color renderbuffer
  glGenFramebuffers(1, &viewFramebuffer);
  glGenRenderbuffers(1, &viewRenderbuffer);

  glBindFramebuffer(GL_FRAMEBUFFER, viewFramebuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
  // This call associates the storage for the current render buffer with the EAGLDrawable (our CAEAGLLayer)
  // allowing us to draw into a buffer that will later be rendered to screen wherever the layer is (which corresponds with our view).
  [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:(id<EAGLDrawable>)self.layer];
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, viewRenderbuffer);

  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);

  // For this sample, we do not need a depth buffer. If you do, this is how you can create one and attach it to the framebuffer:
  glGenRenderbuffers(1, &depthRenderbuffer);
  glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
    NSLog(@"failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    return NO;
    }

  // setup the vis pipeline
  [self setupPipeline];

  [self getVTKRenderWindow]->SetSize(backingWidth, backingHeight);

  [self getVTKRenderWindow]->Render();
  glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
  [context presentRenderbuffer:GL_RENDERBUFFER];

  return YES;
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
}

- (BOOL)resizeFromLayer:(CAEAGLLayer *)layer
{
  // Allocate color buffer backing based on the current layer size
  glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
  [context renderbufferStorage:GL_RENDERBUFFER fromDrawable:layer];
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth);
  glGetRenderbufferParameteriv(GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight);

  // For this sample, we do not need a depth buffer. If you do, this is how you can allocate depth buffer backing:
  glBindRenderbuffer(GL_RENDERBUFFER, depthRenderbuffer);
  glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight);
  glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    {
    NSLog(@"Failed to make complete framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
    return NO;
    }

  [self getVTKRenderWindow]->SetSize(backingWidth, backingHeight);

  [self getVTKRenderWindow]->Render();
  glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
  [context presentRenderbuffer:GL_RENDERBUFFER];

  return YES;
}

// Releases resources when they are not longer needed.
- (void)dealloc
{
    // Destroy framebuffers and renderbuffers
  if (viewFramebuffer)
    {
    glDeleteFramebuffers(1, &viewFramebuffer);
    viewFramebuffer = 0;
    }
  if (viewRenderbuffer)
    {
    glDeleteRenderbuffers(1, &viewRenderbuffer);
    viewRenderbuffer = 0;
    }
  if (depthRenderbuffer)
    {
    glDeleteRenderbuffers(1, &depthRenderbuffer);
    depthRenderbuffer = 0;
   }

  // tear down context
  if ([EAGLContext currentContext] == context)
    {
    [EAGLContext setCurrentContext:nil];
    }

  [super dealloc];
}

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

  CGRect bounds = [self bounds];

  // set the position for all contacts
  NSSet *myTouches = [event touchesForView:self];
  for (UITouch *touch in myTouches)
    {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self];
    location.y = bounds.size.height - location.y;

    int index = interactor->GetContactIndex((void *)touch);
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
    int index = interactor->GetContactIndex((void *)touch);
    interactor->SetPointerIndex(index);
    interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
    NSLog(@"Starting left mouse");
    }

  // Display the buffer
  glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
  [context presentRenderbuffer:GL_RENDERBUFFER];
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

  CGRect        bounds = [self bounds];

  // set the position for all contacts
  int index;
  NSSet *myTouches = [event touchesForView:self];
  for (UITouch *touch in myTouches)
    {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self];
    location.y = bounds.size.height - location.y;

    index = interactor->GetContactIndex((void *)touch);
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
  glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
  [context presentRenderbuffer:GL_RENDERBUFFER];
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

  CGRect        bounds = [self bounds];
  // set the position for all contacts
  NSSet *myTouches = [event touchesForView:self];
  for (UITouch *touch in myTouches)
    {
    // Convert touch point from UIView referential to OpenGL one (upside-down flip)
    CGPoint location = [touch locationInView:self];
    location.y = bounds.size.height - location.y;

    int index = interactor->GetContactIndex((void *)touch);
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
    int index = interactor->GetContactIndex((void *)touch);
    interactor->SetPointerIndex(index);
    interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
    interactor->ClearContactIndex((void *)touch);
    NSLog(@"lifting left mouse");
    }

  // Display the buffer
  glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
  [context presentRenderbuffer:GL_RENDERBUFFER];
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

  CGRect        bounds = [self bounds];
  UITouch*            touch = [[event touchesForView:self] anyObject];
  // Convert touch point from UIView referential to OpenGL one (upside-down flip)
  CGPoint location = [touch locationInView:self];
  location.y = bounds.size.height - location.y;

  interactor->SetEventInformation((int)round(location.x),
                                  (int)round(location.y),
                                  0, 0,
                                  0, 0);
  interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
 // NSLog(@"Ended left mouse");

  // Display the buffer
  //[self getVTKRenderWindow]->Render();
  glBindRenderbuffer(GL_RENDERBUFFER, viewRenderbuffer);
  [context presentRenderbuffer:GL_RENDERBUFFER];
}

- (BOOL)canBecomeFirstResponder
{
    return YES;
}


@end
