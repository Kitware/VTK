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

- (id)initWithVtkView:(VTKView*)vtkView;

- (void)onPinch:(UIPinchGestureRecognizer*)sender;
- (void)onRoll:(UIRotationGestureRecognizer*)sender;
- (void)onPan:(UIPanGestureRecognizer*)sender;

@end
