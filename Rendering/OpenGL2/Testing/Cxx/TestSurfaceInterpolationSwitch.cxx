/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDelaunay2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"

// Regression testing for following crash:
// - polydata with point and cell normals is rendered as phong
// - surface interpolation is then switched to flat
// - next rendering call would provoke a nullptr access because
//   polydata mapper was previously not handling this change correctly
int TestSurfaceInterpolationSwitch(int argc, char* argv[])
{
  auto sphereSource = vtkSmartPointer<vtkSphereSource>::New();
  auto normalsFilter = vtkSmartPointer<vtkPolyDataNormals>::New();
  normalsFilter->SetInputConnection(sphereSource->GetOutputPort());
  normalsFilter->SetComputePointNormals(true);
  normalsFilter->SetComputeCellNormals(true);
  normalsFilter->Update();

  auto polydata = normalsFilter->GetOutput();

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkProperty* property = actor->GetProperty();
  property->SetRepresentationToSurface();
  property->SetInterpolationToPhong();

  // Render image
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.0, 0.0, 0.0);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 300);
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  if (!renWin->SupportsOpenGL())
  {
    cerr << "The platform does not support OpenGL as required\n";
    cerr << vtkOpenGLRenderWindow::SafeDownCast(renWin)->GetOpenGLSupportMessage();
    cerr << renWin->ReportCapabilities();
    return 1;
  }

  renWin->Render(); // this render call was always ok

  property->SetInterpolationToFlat();
  mapper->Update();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
