// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (C) Copyright © 2017 Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause

//
//  VTKViewController.h
//  VTKViewer
//
//  Created by Max Smolens on 6/19/17.
//

#import <UIKit/UIKit.h>

@interface VTKViewController
  : UIViewController<UIGestureRecognizerDelegate, UIDocumentPickerDelegate>

- (void)loadFiles:(nonnull NSArray<NSURL*>*)urls;

@end
