//
//  VTKGestureHandler.h
//  VTKViewer
//
//  Created by Max Smolens on 6/20/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@class VTKView;

@interface VTKGestureHandler : NSObject<UIGestureRecognizerDelegate>

- (id)initWithView:(UIView *)view vtkView:(VTKView *)vtkView;

- (void)onSingleTap:(UITapGestureRecognizer *)sender;
- (void)onDoubleTap:(UITapGestureRecognizer *)sender;
- (void)onLongPress:(UILongPressGestureRecognizer *)sender;
- (void)onPinch:(UIPinchGestureRecognizer *)sender;
- (void)onRoll:(UIRotationGestureRecognizer *)sender;
- (void)onTrackballMotion:(UIPanGestureRecognizer *)sender;
- (void)onTwoFingerPan:(UIPanGestureRecognizer *)sender;

@end
