/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkShaderProperty.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

#include "vtkRenderWindowInteractor.h"

//------------------------------------------------------------------------------
int TestProgramPointSize(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);

  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  mapper->SetInputConnection(sphere->GetOutputPort());
  mapper->UseProgramPointSizeOn();
  vtkNew<vtkActor> actor;
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToPoints();

  vtkShaderProperty* sp = actor->GetShaderProperty();
  sp->AddVertexShaderReplacement("//VTK::ValuePass::Impl", // replace the normal block
    true,                                                  // before the standard replacements
    "gl_PointSize = (1.0 - gl_Position.z) * 8.0;\n"
    "///VTK::ValuePass::Impl\n", // we still want the default
    false                        // only do it once
  );

  renderWindow->SetMultiSamples(0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(-45);
  renderer->GetActiveCamera()->OrthogonalizeViewUp();
  renderer->GetActiveCamera()->Zoom(1.5);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();

  int retVal = vtkRegressionTestImageThreshold(renderWindow, 0.5);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
