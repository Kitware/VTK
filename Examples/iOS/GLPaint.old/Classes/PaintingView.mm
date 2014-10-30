/*=========================================================================

Program:   Visualization Toolkit

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>
#import <GLKit/GLKit.h>

#import "PaintingView.h"

#include "vtkNew.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkDebugLeaks.h"
#include "vtkGlyph3D.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkIOSRenderWindow.h"
#include "vtkIOSRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkCommand.h"
#include "vtkInteractorStyleMultiTouchCamera.h"


@interface PaintingView()
{
}

@end

@implementation PaintingView


- (void)setupPipeline
{
  [super setupPipeline];

  vtkIOSRenderWindowInteractor *interactor = [self getInteractor];
  if (!interactor)
    {
    return;
    }

  vtkIOSRenderWindow *renWin = [self getVTKRenderWindow];
  if (!renWin)
    {
    return;
    }

  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer.Get());

  vtkInteractorStyleMultiTouchCamera *ismt = vtkInteractorStyleMultiTouchCamera::New();
  interactor->SetInteractorStyle(ismt);
  ismt->Delete();

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper.Get());

  vtkNew<vtkConeSource> cone;
  cone->SetResolution(6);

  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  vtkNew<vtkPolyDataMapper> spikeMapper;
  spikeMapper->SetInputConnection(glyph->GetOutputPort());

  vtkNew<vtkActor> spikeActor;
  spikeActor->SetMapper(spikeMapper.Get());

  renderer->AddActor(sphereActor.Get());
  renderer->AddActor(spikeActor.Get());
  renderer->SetBackground(0.4,0.5,0.6);
}


@end
