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
#include "vtkTransform.h"
#include "vtkTriangleFilter.h"
#include "vtkmPointTransform.h"

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

  vtkNew<vtkmPointTransform> pf;
  pf->SetInputData(pd);
  vtkNew<vtkTransform> transformMatrix;
  transformMatrix->RotateX(30);
  transformMatrix->RotateY(60);
  transformMatrix->RotateZ(90);
  pf->SetTransform(transformMatrix);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(pf->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  ren->AddActor(actor);

  ren->SetBackground(0.0, 0.0, 0.0);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (!retVal);
}
}

int TestVTKMPointTransform(int argc, char* argv[])
{
  vtkNew<vtkPlaneSource> plane;
  int res = 300;
  plane->SetXResolution(res);
  plane->SetYResolution(res);
  plane->SetOrigin(-10.0, -10.0, 0.0);
  plane->SetPoint1(10.0, -10.0, 0.0);
  plane->SetPoint2(-10.0, 10.0, 0.0);

  return RunVTKPipeline(plane, argc, argv);
}
