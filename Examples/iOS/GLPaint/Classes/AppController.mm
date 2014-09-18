/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#import "AppController.h"
#import "PaintingViewController.h"

@implementation AppController

- (void)applicationDidFinishLaunching:(UIApplication*)application
{
    self.window = [[UIWindow alloc] initWithFrame:[[UIScreen mainScreen] bounds]];

    PaintingViewController *controller = [[PaintingViewController alloc] initWithNibName:@"PaintingViewController" bundle:nil];
    self.window.rootViewController = controller;
    [self.window makeKeyAndVisible];
}

@end
