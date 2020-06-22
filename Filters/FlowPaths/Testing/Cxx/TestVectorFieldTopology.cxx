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
#include "vtkMath.h"
#include "vtkNew.h"
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

int TestVectorFieldTopology(int argc, char* argv[])
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

  vtkNew<vtkVectorFieldTopology> topology;
  topology->SetInputData(calc->GetOutput());
  topology->SetIntegrationStepUnit(1);
  topology->SetSeparatrixDistance(1);
  topology->SetIntegrationStepSize(1);
  topology->SetMaxNumSteps(1000);
  topology->SetComputeSurfaces(1);
  topology->SetUseIterativeSeeding(1);

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
  pointActor->GetProperty()->SetRenderPointsAsSpheres(1);

  // the separating lines
  vtkNew<vtkDataSetMapper> lineMapper;
  lineMapper->SetInputConnection(topology->GetOutputPort(1));

  vtkNew<vtkActor> lineActor;
  lineActor->SetMapper(lineMapper);
  lineActor->GetProperty()->SetColor(0.2, 0.2, 0.2);
  lineActor->GetProperty()->SetLineWidth(10.);
  lineActor->GetProperty()->SetRenderLinesAsTubes(1);

  // the separating surfaces
  vtkNew<vtkDataSetMapper> surfaceMapper;
  surfaceMapper->SetInputConnection(topology->GetOutputPort(2));

  vtkNew<vtkActor> surfaceActor;
  surfaceActor->SetMapper(surfaceMapper);
  surfaceActor->GetProperty()->SetColor(0.1, 0.1, 0.1);
  surfaceActor->GetProperty()->SetRepresentationToWireframe();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(waveletActor);
  renderer->AddActor(pointActor);
  renderer->AddActor(lineActor);
  renderer->AddActor(surfaceActor);
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
