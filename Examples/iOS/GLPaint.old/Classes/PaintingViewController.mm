/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import "PaintingViewController.h"
#import "PaintingView.h"

//CLASS IMPLEMENTATIONS:

@interface PaintingViewController()
{
	CFTimeInterval		lastTime;
}
@end

@implementation PaintingViewController

- (void)viewDidLoad
{
  [super viewDidLoad];
}

- (void)viewDidAppear:(BOOL)animated
{
    [super viewDidAppear:animated];
    [self becomeFirstResponder];
}

- (BOOL)canBecomeFirstResponder
{
    return YES;
}

// Release resources when they are no longer needed,
- (void)dealloc
{
}

// We do not support auto-rotation in this sample
- (BOOL)shouldAutorotate
{
    return NO;
}


@end
