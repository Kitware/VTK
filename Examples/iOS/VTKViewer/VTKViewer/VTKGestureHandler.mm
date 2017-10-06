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

@interface VTKGestureHandler()

@property (weak, nonatomic) UIView *view;
@property (weak, nonatomic) VTKView *vtkView;

@property (strong, nonatomic) UITapGestureRecognizer *singleTapRecognizer;
@property (strong, nonatomic) UITapGestureRecognizer *doubleTapRecognizer;
@property (strong, nonatomic) UILongPressGestureRecognizer *longPressRecognizer;
@property (strong, nonatomic) UIPinchGestureRecognizer *pinchRecognizer;
@property (strong, nonatomic) UIRotationGestureRecognizer *rollRecognizer;
@property (strong, nonatomic) UIPanGestureRecognizer *trackballRecognizer;
@property (strong, nonatomic) UIPanGestureRecognizer *twoFingerPanRecognizer;
@property (assign) CGPoint lastTouch;

@end

@implementation VTKGestureHandler

- (id)initWithView:(UIView *)view vtkView:(VTKView *)vtkView {
    if (self = [super init]) {
        self.view = view;
        self.vtkView = vtkView;
        [self setupGestures];
    }
    return self;
}

- (void)setupGestures {
    // Add the single tap gesture recognizer
    self.singleTapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self
                                                                       action:@selector(onSingleTap:)];
    [self.view addGestureRecognizer:self.singleTapRecognizer];
    self.singleTapRecognizer.delegate = self;

    // Add the double tap gesture recognizer
    self.doubleTapRecognizer = [[UITapGestureRecognizer alloc] initWithTarget:self
                                                                       action:@selector(onDoubleTap:)];
    [self.doubleTapRecognizer setNumberOfTapsRequired:2];
    [self.view addGestureRecognizer:self.doubleTapRecognizer];
    self.doubleTapRecognizer.delegate = self;
    [self.singleTapRecognizer requireGestureRecognizerToFail:self.doubleTapRecognizer];

    // Add the long press gesture recognizer
    self.longPressRecognizer = [[UILongPressGestureRecognizer alloc] initWithTarget:self
                                                                             action:@selector(onLongPress:)];
    [self.longPressRecognizer setMinimumPressDuration:0.3];
    [self.view addGestureRecognizer:self.longPressRecognizer];
    self.longPressRecognizer.delegate = self;

    // Add the pinch gesture recognizer
    self.pinchRecognizer = [[UIPinchGestureRecognizer alloc] initWithTarget:self
                                                                     action:@selector(onPinch:)];
    [self.view addGestureRecognizer:self.pinchRecognizer];
    self.pinchRecognizer.delegate = self;

    // Add the 'roll' gesture recognizer
    self.rollRecognizer = [[UIRotationGestureRecognizer alloc] initWithTarget:self
                                                                       action:@selector(onRoll:)];
    [self.view addGestureRecognizer:self.rollRecognizer];
    self.rollRecognizer.delegate = self;

    // Add pan gesture recognizer for trackball motion
    self.trackballRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self
                                                                       action:@selector(onTrackballMotion:)];
    [self.trackballRecognizer setMinimumNumberOfTouches:1];
    [self.trackballRecognizer setMaximumNumberOfTouches:1];
    [self.view addGestureRecognizer:self.trackballRecognizer];
    self.trackballRecognizer.delegate = self;

    // Two fingers to pan
    self.twoFingerPanRecognizer = [[UIPanGestureRecognizer alloc] initWithTarget:self
                                                                          action:@selector(onTwoFingerPan:)];
    [self.twoFingerPanRecognizer setMinimumNumberOfTouches:2];
    [self.twoFingerPanRecognizer setMaximumNumberOfTouches:2];
    [self.view addGestureRecognizer:self.twoFingerPanRecognizer];
    self.twoFingerPanRecognizer.delegate = self;
}

- (BOOL)gestureRecognizerShouldBegin:(UIGestureRecognizer *)gestureRecognizer {
    CGPoint p = [gestureRecognizer locationInView:self.view];
    return CGRectContainsPoint(self.vtkView.frame, p);
}

- (BOOL)gestureRecognizer:(UIGestureRecognizer *)gestureRecognizer shouldRecognizeSimultaneouslyWithGestureRecognizer:(UIGestureRecognizer *)otherGestureRecognizer {
    if (gestureRecognizer == self.trackballRecognizer ||
        otherGestureRecognizer == self.trackballRecognizer) {
        return NO;
    }
    return YES;
}

- (void)onSingleTap:(UITapGestureRecognizer *)sender {
}

- (void)onDoubleTap:(UITapGestureRecognizer *)sender {
}

- (void)onLongPress:(UILongPressGestureRecognizer *)sender {
}

- (void)onPinch:(UIPinchGestureRecognizer *)sender {
    // Send the event position so the correct renderer is affected
    int position[2] = { 0 };
    CGPoint touchPoint = [sender locationInView:self.vtkView];
    [self.vtkView displayCoordinates:position ofTouch:touchPoint];
    self.vtkView.interactor->SetEventInformation(position[0], position[1], 0, 0, 0, 0, 0, 0);

    // Set the scale, clamping to prevent blowouts
    const CGFloat MaxScale = 3.0;
    const CGFloat MinScale = -3.0;
    CGFloat scale = [sender scale];
    scale = scale > MaxScale ? MaxScale : (scale < MinScale ? MinScale : scale);
    assert(scale <= MaxScale && scale >= MinScale);
    self.vtkView.interactor->SetScale(scale);

    // Invoke the right event
    switch ([sender state]) {
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

- (void)onRoll:(UIRotationGestureRecognizer *)sender {
    CGPoint touchPoint = [sender locationInView:self.vtkView];
    if ([sender state] == UIGestureRecognizerStateBegan) {
        self.lastTouch = touchPoint;
    }

    // Send the event position so the correct renderer is affected
    int position[2] = { 0 };
    [self.vtkView displayCoordinates:position ofTouch:touchPoint];
    self.vtkView.interactor->SetEventInformation(position[0], position[1], 0, 0, 0,  0, 0, 0);

    // Set the rotation
    const double SPEED = 1.0;
    self.vtkView.interactor->SetRotation(SPEED * -[sender rotation] * 180.0 / M_PI);

    // Invoke the right event
    switch ([sender state]) {
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

- (void)onTrackballMotion:(UIPanGestureRecognizer *)sender {
    // Get touch point
    CGPoint touchPoint = [sender locationInView:self.vtkView];

    int position[2] = { 0 };
    [self.vtkView displayCoordinates:position ofTouch:touchPoint];

    // Send the event position so the correct renderer is affected
    self.vtkView.interactor->SetEventInformation(position[0], position[1], 0, 0, 0,  0, 0, 0);
    switch ([sender state]) {
        case UIGestureRecognizerStateBegan:
            // TODO not yet upstream
            //double center[3];
            //self.boundingBox.GetCenter(center);
            //self.vtkView.interactor->SetRotationCenter(center);
            self.vtkView.interactor->InvokeEvent(vtkCommand::LeftButtonPressEvent,NULL);
            break;
        case UIGestureRecognizerStateChanged:
            self.vtkView.interactor->InvokeEvent(vtkCommand::MouseMoveEvent,NULL);
            break;
        case UIGestureRecognizerStateEnded:
            self.vtkView.interactor->InvokeEvent(vtkCommand::LeftButtonReleaseEvent,NULL);
            break;
        default:
            break;
    }

    // Render
    [self.vtkView setNeedsDisplay];

    // Update last touch
    self.lastTouch = touchPoint;
}

- (void)onTwoFingerPan:(UIPanGestureRecognizer *)sender {
    // Send the event position so the correct renderer is affected
    int position[2] = { 0 };
    CGPoint touchPoint = [sender locationInView:self.vtkView];
    [self.vtkView displayCoordinates:position ofTouch:touchPoint];
    self.vtkView.interactor->SetEventInformation(position[0], position[1], 0, 0, 0,  0, 0, 0);

    // Set the translation vector
    CGPoint d = [sender translationInView:self.vtkView];
    const double SPEED = 1.0;
    double t[] = {
        SPEED * d.x, -SPEED * d.y
    };

    // Set the translation vector
    self.vtkView.interactor->SetTranslation(t);

    // Invoke the right event
    switch ([sender state]) {
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

    // Render
    [self.vtkView setNeedsDisplay];
}

@end
