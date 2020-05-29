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

#include "vtkmWarpScalar.h"

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

int TestVTKMWarpScalar(int argc, char* argv[])
{
  vtkNew<vtkRenderer> xyplaneRen, dataNormalRen, customNormalRen;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(900, 300);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Define viewport ranges
  // (xmin, ymin, xmax, ymax)
  double leftViewport[4] = { 0.0, 0.0, 0.33, 1.0 };
  double centerViewport[4] = { 0.33, 0.0, .66, 1.0 };
  double rightViewport[4] = { 0.66, 0.0, 1.0, 1.0 };

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
  xySource->SetZMag(5);
  xySource->SetSubsampleRate(1);

  vtkNew<vtkmWarpScalar> xyWarpScalar;
  xyWarpScalar->SetScaleFactor(2);
  xyWarpScalar->XYPlaneOn();
  xyWarpScalar->SetNormal(1, 0, 0); // should be ignored
  xyWarpScalar->SetInputConnection(xySource->GetOutputPort());
  xyWarpScalar->Update();
  vtkPointSet* points = xyWarpScalar->GetOutput();
  for (vtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
  {
    assert(points->GetPoint(i)[2] == 3.0);
    if (points->GetPoint(i)[2] != 3.0)
    {
      std::cout << "XYPlane result is wrong at i=" << i << std::endl;
    }
  }

  vtkNew<vtkDataSetMapper> xyplaneMapper;
  xyplaneMapper->SetInputConnection(xyWarpScalar->GetOutputPort());

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
  // Create a scalar array
  auto dataNormalSourceOutput = dataNormalSource->GetOutput();
  vtkNew<vtkFloatArray> scalarArray;
  scalarArray->SetName("scalarfactor");
  scalarArray->SetNumberOfValues(dataNormalSourceOutput->GetNumberOfPoints());
  for (vtkIdType i = 0; i < dataNormalSourceOutput->GetNumberOfPoints(); i++)
  {
    scalarArray->SetValue(i, 2);
  }
  dataNormalSourceOutput->GetPointData()->AddArray(scalarArray);

  vtkNew<vtkmWarpScalar> dataNormalWarpScalar;
  dataNormalWarpScalar->SetScaleFactor(2);
  dataNormalWarpScalar->SetInputData(dataNormalSource->GetOutput());
  dataNormalWarpScalar->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "scalarfactor");
  vtkNew<vtkDataSetMapper> dataNormalMapper;
  dataNormalMapper->SetInputConnection(dataNormalWarpScalar->GetOutputPort());

  vtkNew<vtkActor> dataNormalActor;
  dataNormalActor->SetMapper(dataNormalMapper);

  renWin->AddRenderer(dataNormalRen);
  dataNormalRen->SetViewport(centerViewport);
  dataNormalRen->SetBackground(0.0, 0.7, 0.2);
  dataNormalRen->AddActor(dataNormalActor);

  /// Third window - custom normal
  vtkSmartPointer<vtkRTAnalyticSource> customNormalSource =
    vtkSmartPointer<vtkRTAnalyticSource>::New();
  customNormalSource->SetWholeExtent(-100, 100, -100, 100, 1, 1);
  customNormalSource->SetCenter(0, 0, 0);
  customNormalSource->SetMaximum(255);
  customNormalSource->SetStandardDeviation(.5);
  customNormalSource->SetXFreq(60);
  customNormalSource->SetYFreq(30);
  customNormalSource->SetZFreq(40);
  customNormalSource->SetXMag(10);
  customNormalSource->SetYMag(18);
  customNormalSource->SetZMag(5);
  customNormalSource->SetSubsampleRate(1);

  vtkNew<vtkmWarpScalar> customNormalWarpScalar;
  customNormalWarpScalar->SetScaleFactor(2);
  customNormalWarpScalar->SetNormal(0.333, 0.333, 0.333);
  customNormalWarpScalar->SetInputConnection(customNormalSource->GetOutputPort());
  vtkNew<vtkDataSetMapper> customNormalMapper;
  customNormalMapper->SetInputConnection(customNormalWarpScalar->GetOutputPort());

  vtkNew<vtkActor> customNormalActor;
  customNormalActor->SetMapper(customNormalMapper);
  renWin->AddRenderer(customNormalRen);
  customNormalRen->SetViewport(rightViewport);
  customNormalRen->SetBackground(0.3, 0.2, 0.5);
  customNormalRen->AddActor(customNormalActor);

  xyplaneRen->ResetCamera();
  dataNormalRen->ResetCamera();
  customNormalRen->ResetCamera();

  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (!retVal);
}
