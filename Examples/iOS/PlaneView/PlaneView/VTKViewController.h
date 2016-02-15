/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import <UIKit/UIKit.h>
#import <GLKit/GLKit.h>

// Forward declarations
class vtkIOSRenderWindow;
class vtkRenderer;
class vtkIOSRenderWindowInteractor;
class vtkPlaneWidget;
class vtkPolyDataMapper;
class vtkProbeFilter;
class vtkTPWCallback;

@interface VTKViewController : GLKViewController
{
@private
  vtkIOSRenderWindow *RenderWindow;
  vtkPlaneWidget *PlaneWidget;
  vtkRenderer *Renderer;
  vtkProbeFilter *Probe;
  vtkPolyDataMapper *OutlineMapper;
  vtkPolyDataMapper *ProbeMapper;
  vtkTPWCallback *PlaneCallback;
}

@property (nonatomic, strong) UIWindow *window;

- (void)setProbeEnabled:(bool)val;
- (bool)getProbeEnabled;

- (void)setNewDataFile:(NSURL *)url;

- (vtkIOSRenderWindowInteractor *)getInteractor;

@end
