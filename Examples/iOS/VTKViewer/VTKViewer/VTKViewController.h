//
//  VTKViewController.h
//  VTKViewer
//
//  Created by Max Smolens on 6/19/17.
//  Copyright © 2017 Kitware, Inc. All rights reserved.
//

#import <UIKit/UIKit.h>

@interface VTKViewController
  : UIViewController<UIGestureRecognizerDelegate, UIDocumentPickerDelegate>

- (void)loadFiles:(nonnull NSArray<NSURL*>*)urls;

@end
