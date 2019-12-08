//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2012 Sandia Corporation.
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//=============================================================================

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTriangleFilter.h"
#include "vtkmPointElevation.h"

namespace
{
int RunVTKPipeline(vtkPlaneSource* plane, int argc, char* argv[])
{
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;

  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  vtkNew<vtkTriangleFilter> tf;
  tf->SetInputConnection(plane->GetOutputPort());
  tf->Update();

  vtkNew<vtkPolyData> pd;
  pd->CopyStructure(tf->GetOutput());
  vtkIdType numPts = pd->GetNumberOfPoints();
  vtkPoints* oldPts = tf->GetOutput()->GetPoints();
  vtkNew<vtkPoints> newPts;
  newPts->SetNumberOfPoints(numPts);
  for (vtkIdType i = 0; i < numPts; i++)
  {
    auto pt = oldPts->GetPoint(i);
    auto r = sqrt(pow(pt[0], 2) + pow(pt[1], 2));
    auto z = 1.5 * cos(2 * r);
    newPts->SetPoint(i, pt[0], pt[1], z);
  }
  pd->SetPoints(newPts);

  vtkNew<vtkmPointElevation> pe;
  pe->SetInputData(pd);
  pe->SetLowPoint(0, 0, -1.5);
  pe->SetHighPoint(0, 0, 1.5);
  pe->SetScalarRange(-1.5, 1.5);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(pe->GetOutputPort());
  mapper->ScalarVisibilityOn();
  mapper->SelectColorArray("elevation");

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Add the actor to the renderer, set the background and size
  ren->AddActor(actor);

  ren->SetBackground(0, 0, 0);
  vtkNew<vtkCamera> camera;
  camera->SetPosition(1, 50, 50);
  ren->SetActiveCamera(camera);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (!retVal);
}
} //  Anonymous namespace

int TestVTKMPointElevation(int argc, char* argv[])
{
  // Create a plane source
  vtkNew<vtkPlaneSource> plane;
  int res = 200;
  plane->SetXResolution(res);
  plane->SetYResolution(res);
  plane->SetOrigin(-10, -10, 0);
  plane->SetPoint1(10, -10, 0);
  plane->SetPoint2(-10, 10, 0);

  // Run the pipeline
  return RunVTKPipeline(plane, argc, argv);
}
