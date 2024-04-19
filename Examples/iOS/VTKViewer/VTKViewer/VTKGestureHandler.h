// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (C) Copyright © 2017 Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause

//
//  VTKGestureHandler.h
//  VTKViewer
//
//  Created by Max Smolens on 6/20/17.
//

#import <UIKit/UIKit.h>

@class VTKView;

@interface VTKGestureHandler : NSObject<UIGestureRecognizerDelegate>

- (id)initWithVtkView:(VTKView*)vtkView;

- (void)onPinch:(UIPinchGestureRecognizer*)sender;
- (void)onRoll:(UIRotationGestureRecognizer*)sender;
- (void)onPan:(UIPanGestureRecognizer*)sender;

@end
