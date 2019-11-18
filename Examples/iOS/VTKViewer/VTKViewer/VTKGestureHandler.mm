//
//  VTKGestureHandler.m
//  VTKViewer
//
//  Created by Max Smolens on 6/20/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

#import "VTKGestureHandler.h"

#import "VTKView.h"

#include "vtkIOSRenderWindowInteractor.h"

@interface VTKGestureHandler ()

@property (weak, nonatomic) VTKView* vtkView;

@property (strong, nonatomic) UITapGestureRecognizer* singleTapRecognizer;
@property (strong, nonatomic) UITapGestureRecognizer* doubleTapRecognizer;
@property (strong, nonatomic) UILongPressGestureRecognizer* longPressRecognizer;
@property (strong, nonatomic) UIPinchGestureRecognizer* pinchRecognizer;
@property (strong, nonatomic) UIRotationGestureRecognizer* rollRecognizer;
@property (strong, nonatomic) UIPanGestureRecognizer* trackballRecognizer;
@property (strong, nonatomic) UIPanGestureRecognizer* panRecognizer;
@property (assign) NSInteger lastNumberOfTouches;

@end

@implementation VTKGestureHandler

- (id)initWithVtkView:(VTKView*)vtkView
{
  if (self = [super init])
  {
    self.vtkView = vtkView;
    [self setupGestures];
  }
  return self;
}

- (void)setupGestures
{
  // Add the pinch gesture recognizer
  self.pinchRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self
                                                                   action:@selector(onPinch:)];
  [self.vtkView addGestureRecognizer:self.pinchRecognizer];
  self.pinchRecognizer.delegate = self;

  // Add the 'roll' gesture recognizer
  self.rollRecognizer = [[UIRotationGestureRecognizer alloc] initWithTarget:self
                                                                     action:@selector(onRoll:)];
  [self.vtkView addGestureRecognizer:self.rollRecognizer];
  self.rollRecognizer.delegate = self;

  // Add the pan gesture regognizer (trackball if 1 finger)
  self.panRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self
                                                               action:@selector(onPan:)];
  [self.panRecognizer setMinimumNumberOfTouches:1];
  [self.panRecognizer setMaximumNumberOfTouches:2];
  [self.vtkView addGestureRecognizer:self.panRecognizer];
  self.panRecognizer.delegate = self;

  self.lastNumberOfTouches = 0;
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer*)gestureRecognizer
  shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer*)otherGestureRecognizer
{
  return YES;
}

- (void)onPinch:(UIPinchGestureRecognizer*)sender
{
  if (sender.numberOfTouches == 1)
  {
    return;
  }

  // Send the gesture position to the interactor
  [self forwardTouchPositionToInteractor:sender];

  // Set the scale, clamping to prevent blowouts
  const CGFloat MaxScale = 3.0;
  const CGFloat MinScale = -3.0;
  CGFloat scale = [sender scale];
  scale = scale > MaxScale ? MaxScale : (scale < MinScale ? MinScale : scale);
  assert(scale <= MaxScale && scale >= MinScale);

  // Send the gesture scale to the interactor
  self.vtkView.interactor->SetScale(scale);

  // Invoke the right event
  switch ([sender state])
  {
    case UIGestureRecognizerStateBegan:
      self.vtkView.interactor->StartPinchEvent();
      break;
    case UIGestureRecognizerStateChanged:
      self.vtkView.interactor->PinchEvent();
      break;
    case UIGestureRecognizerStateEnded:
      self.vtkView.interactor->EndPinchEvent();
      break;
    default:
      break;
  }

  // Render
  [self.vtkView setNeedsDisplay];
}

- (void)onRoll:(UIRotationGestureRecognizer*)sender
{
  if (sender.numberOfTouches == 1)
  {
    return;
  }

  // Send the gesture position to the interactor
  [self forwardTouchPositionToInteractor:sender];

  // Set the rotation angle
  double rotationAngle = -[sender rotation] * 180.0 / M_PI;

  // Send the gesture rotation to the interactor
  self.vtkView.interactor->SetRotation(rotationAngle);

  // Invoke the right event
  switch ([sender state])
  {
    case UIGestureRecognizerStateBegan:
      self.vtkView.interactor->StartRotateEvent();
      break;
    case UIGestureRecognizerStateChanged:
      self.vtkView.interactor->RotateEvent();
      break;
    case UIGestureRecognizerStateEnded:
      self.vtkView.interactor->EndRotateEvent();
      break;
    default:
      break;
  }

  // Render
  [self.vtkView setNeedsDisplay];
}

- (void)onPan:(UIPanGestureRecognizer*)sender
{
  // Send the gesture position to the interactor
  [self forwardTouchPositionToInteractor:sender];

  // Two fingers means panning
  if (sender.numberOfTouches == 2 ||
    (self.lastNumberOfTouches == 2 && [sender state] == UIGestureRecognizerStateEnded))
  {
    // Set the translation vector
    CGPoint d = [sender translationInView:self.vtkView];
    double scaleFactor = self.vtkView.contentScaleFactor;
    double t[] = { scaleFactor * d.x, -scaleFactor * d.y };

    // Send the gesture translation to the interactor
    self.vtkView.interactor->SetTranslation(t);

    if (self.lastNumberOfTouches == 1)
    {
      // If switching from one finger, cancel one finger pan
      // and begin two finger pan
      [self onTrackballMotion:UIGestureRecognizerStateEnded];
      [self onTwoFingerPan:UIGestureRecognizerStateBegan];
    }
    else
    {
      // Invoke the right event
      [self onTwoFingerPan:[sender state]];
    }
    // One finger means trackball
  }
  else if (sender.numberOfTouches == 1 ||
    (self.lastNumberOfTouches == 1 && [sender state] == UIGestureRecognizerStateEnded))
  {
    if (self.lastNumberOfTouches == 2)
    {
      // If switching from two fingers, cancel two fingers pan,
      // roll and pinch, and start one finger pan
      [self onTrackballMotion:UIGestureRecognizerStateBegan];
      [self onTwoFingerPan:UIGestureRecognizerStateEnded];
      self.vtkView.interactor->EndPinchEvent();
      self.vtkView.interactor->EndRotateEvent();
    }
    else
    {
      // Invoke the right event
      [self onTrackballMotion:[sender state]];
    }
  }

  // Update number of touches
  self.lastNumberOfTouches = sender.numberOfTouches;

  // Render
  [self.vtkView setNeedsDisplay];
}

- (void)onTrackballMotion:(UIGestureRecognizerState)state
{
  switch (state)
  {
    case UIGestureRecognizerStateBegan:
      self.vtkView.interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent, NULL);
      break;
    case UIGestureRecognizerStateChanged:
      self.vtkView.interactor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
      break;
    case UIGestureRecognizerStateEnded:
      self.vtkView.interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
      break;
    default:
      break;
  }
}

- (void)onTwoFingerPan:(UIGestureRecognizerState)state
{
  switch (state)
  {
    case UIGestureRecognizerStateBegan:
      self.vtkView.interactor->StartPanEvent();
      break;
    case UIGestureRecognizerStateChanged:
      self.vtkView.interactor->PanEvent();
      break;
    case UIGestureRecognizerStateEnded:
      self.vtkView.interactor->EndPanEvent();
      break;
    default:
      break;
  }
}

- (void)forwardTouchPositionToInteractor:(UIGestureRecognizer*)sender
{
  // Position in screen
  CGPoint touchPoint = [sender locationInView:self.vtkView];

  // Translate to position in VTK coordinates
  int position[2] = { 0 };
  [self.vtkView displayCoordinates:position ofTouch:touchPoint];

  // Forward position to VTK interactor
  self.vtkView.interactor->SetEventInformation(position[0], position[1], 0, 0, 0, 0, 0, 0);
}

@end
