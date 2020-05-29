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

#include "vtkmWarpVector.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetMapper.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

int TestVTKMWarpVector(int argc, char* argv[])
{
  vtkNew<vtkRenderer> xyplaneRen, dataNormalRen;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(600, 300);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Define viewport ranges
  // (xmin, ymin, xmax, ymax)
  double leftViewport[4] = { 0.0, 0.0, 0.5, 1.0 };
  double centerViewport[4] = { 0.5, 0.0, 1.0, 1.0 };

  /// First window - xy plane
  vtkSmartPointer<vtkRTAnalyticSource> xySource = vtkSmartPointer<vtkRTAnalyticSource>::New();
  xySource->SetWholeExtent(-100, 100, -100, 100, 1, 1);
  xySource->SetCenter(0, 0, 0);
  xySource->SetMaximum(255);
  xySource->SetStandardDeviation(.5);
  xySource->SetXFreq(60);
  xySource->SetYFreq(30);
  xySource->SetZFreq(40);
  xySource->SetXMag(10);
  xySource->SetYMag(18);
  xySource->SetZMag(10);
  xySource->SetSubsampleRate(1);
  xySource->Update();
  vtkNew<vtkFloatArray> xyVector;
  xyVector->SetNumberOfComponents(3);
  xyVector->SetName("scalarVector");
  xyVector->SetNumberOfTuples(xySource->GetOutput()->GetNumberOfPoints());
  for (vtkIdType i = 0; i < xySource->GetOutput()->GetNumberOfPoints(); i++)
  {
    xyVector->SetTuple3(i, 0.0, 0.0, 1.0);
  }
  xySource->GetOutput()->GetPointData()->AddArray(xyVector);

  vtkNew<vtkmWarpVector> xyWarpVector;
  xyWarpVector->SetScaleFactor(2);
  xyWarpVector->SetInputConnection(xySource->GetOutputPort());

  // Create a scalarVector array
  xyWarpVector->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "scalarVector");
  xyWarpVector->Update();

  vtkNew<vtkDataSetMapper> xyplaneMapper;
  xyplaneMapper->SetInputConnection(xyWarpVector->GetOutputPort());

  vtkNew<vtkActor> xyplaneActor;
  xyplaneActor->SetMapper(xyplaneMapper);

  renWin->AddRenderer(xyplaneRen);
  xyplaneRen->SetViewport(leftViewport);
  xyplaneRen->SetBackground(0.5, 0.4, 0.3);
  xyplaneRen->AddActor(xyplaneActor);

  /// Second window - data normal
  vtkSmartPointer<vtkSphereSource> dataNormalSource = vtkSmartPointer<vtkSphereSource>::New();
  dataNormalSource->SetRadius(100);
  dataNormalSource->SetThetaResolution(20);
  dataNormalSource->SetPhiResolution(20);
  dataNormalSource->Update();
  auto dataNormalSourceOutput = dataNormalSource->GetOutput();

  vtkNew<vtkmWarpVector> dataNormalWarpVector;
  dataNormalWarpVector->SetScaleFactor(5);
  dataNormalWarpVector->SetInputData(dataNormalSource->GetOutput());
  dataNormalWarpVector->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::POINT, dataNormalSourceOutput->GetPointData()->GetNormals()->GetName());

  vtkNew<vtkDataSetMapper> dataNormalMapper;
  dataNormalMapper->SetInputConnection(dataNormalWarpVector->GetOutputPort());

  vtkNew<vtkActor> dataNormalActor;
  dataNormalActor->SetMapper(dataNormalMapper);

  renWin->AddRenderer(dataNormalRen);
  dataNormalRen->SetViewport(centerViewport);
  dataNormalRen->SetBackground(0.0, 0.7, 0.2);
  dataNormalRen->AddActor(dataNormalActor);

  xyplaneRen->ResetCamera();
  dataNormalRen->ResetCamera();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (!retVal);
}
