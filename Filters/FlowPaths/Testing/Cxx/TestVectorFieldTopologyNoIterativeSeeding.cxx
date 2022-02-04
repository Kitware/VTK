/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDistancePolyDataFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkActor.h"
#include "vtkArrayCalculator.h"
#include "vtkDataSetMapper.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointSource.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRegularPolygonSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStreamSurface.h"
#include "vtkVectorFieldTopology.h"
#include "vtkWarpScalar.h"

int TestVectorFieldTopologyNoIterativeSeeding(int argc, char* argv[])
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  vtkNew<vtkArrayCalculator> calc;
  calc->AddCoordinateScalarVariable("coordsX", 0);
  calc->AddCoordinateScalarVariable("coordsY", 1);
  calc->AddCoordinateScalarVariable("coordsZ", 2);
  calc->SetFunction("(coordsX+coordsZ)*iHat + coordsY*jHat + (coordsX-coordsZ)*kHat");
  calc->SetInputConnection(wavelet->GetOutputPort());
  calc->Update();

  vtkNew<vtkImageData> calcOutput;
  calcOutput->ShallowCopy(calc->GetImageDataOutput());

  vtkNew<vtkDoubleArray> array;
  array->DeepCopy(calc->GetImageDataOutput()->GetPointData()->GetVectors());
  array->SetName("array");
  calcOutput->GetPointData()->AddArray(array);
  calcOutput->GetPointData()->RemoveArray(0);
  calcOutput->GetPointData()->RemoveArray(0);

  vtkNew<vtkVectorFieldTopology> topology;
  topology->SetInputData(calcOutput);
  topology->SetIntegrationStepUnit(1);
  topology->SetSeparatrixDistance(1);
  topology->SetIntegrationStepSize(1);
  topology->SetMaxNumSteps(1000);
  topology->SetComputeSurfaces(true);
  topology->SetUseBoundarySwitchPoints(true);
  topology->SetUseIterativeSeeding(false);
  topology->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "array");
  topology->Update();

  // the bounding box
  vtkNew<vtkDataSetMapper> waveletMapper;
  waveletMapper->SetInputConnection(wavelet->GetOutputPort());

  vtkNew<vtkActor> waveletActor;
  waveletActor->SetMapper(waveletMapper);
  waveletActor->GetProperty()->SetColor(0.4, 0.4, 0.4);
  waveletActor->GetProperty()->SetOpacity(0.1);
  waveletActor->GetProperty()->SetRepresentationToSurface();

  // the critical points
  vtkNew<vtkDataSetMapper> pointMapper;
  pointMapper->SetInputConnection(topology->GetOutputPort(0));

  vtkNew<vtkActor> pointActor;
  pointActor->SetMapper(pointMapper);
  pointActor->GetProperty()->SetColor(0.1, 0.1, 0.1);
  pointActor->GetProperty()->SetPointSize(20.);
  pointActor->GetProperty()->SetRenderPointsAsSpheres(true);

  // the separating lines
  vtkNew<vtkDataSetMapper> lineMapper;
  lineMapper->SetInputConnection(topology->GetOutputPort(1));

  vtkNew<vtkActor> lineActor;
  lineActor->SetMapper(lineMapper);
  lineActor->GetProperty()->SetColor(0.2, 0.2, 0.2);
  lineActor->GetProperty()->SetLineWidth(10.);
  lineActor->GetProperty()->SetRenderLinesAsTubes(true);

  // the separating surfaces
  vtkNew<vtkDataSetMapper> surfaceMapper;
  surfaceMapper->SetInputConnection(topology->GetOutputPort(2));

  vtkNew<vtkActor> surfaceActor;
  surfaceActor->SetMapper(surfaceMapper);
  surfaceActor->GetProperty()->SetColor(0.1, 0.1, 0.1);
  surfaceActor->GetProperty()->SetRepresentationToWireframe();

  // the boundary switch lines
  vtkNew<vtkDataSetMapper> lineMapper2;
  lineMapper2->SetInputConnection(topology->GetOutputPort(3));

  vtkNew<vtkActor> lineActor2;
  lineActor2->SetMapper(lineMapper2);
  lineActor2->GetProperty()->SetColor(0.2, 0.2, 0.2);
  lineActor2->GetProperty()->SetLineWidth(10.);
  lineActor2->GetProperty()->SetRenderLinesAsTubes(true);

  // the boundary switch surfaces
  vtkNew<vtkDataSetMapper> surfaceMapper2;
  surfaceMapper2->SetInputConnection(topology->GetOutputPort(4));

  vtkNew<vtkActor> surfaceActor2;
  surfaceActor2->SetMapper(surfaceMapper2);
  surfaceActor2->GetProperty()->SetColor(0.1, 0.1, 0.1);
  surfaceActor2->GetProperty()->SetRepresentationToWireframe();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(waveletActor);
  renderer->AddActor(pointActor);
  renderer->AddActor(lineActor);
  renderer->AddActor(surfaceActor);
  renderer->AddActor(lineActor2);
  renderer->AddActor(surfaceActor2);
  renderer->ResetCamera();
  renderer->SetBackground(1., 1., 1.);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
