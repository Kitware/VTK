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
#include "vtkWarpScalar.h"

int TestStreamSurface(int argc, char* argv[])
{

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  vtkNew<vtkArrayCalculator> calc;
  calc->AddCoordinateScalarVariable("coordsX", 0);
  calc->AddCoordinateScalarVariable("coordsY", 1);
  calc->AddCoordinateScalarVariable("coordsZ", 2);
  calc->SetFunction("coordsX*iHat + coordsY*jHat + 0.5*(coordsZ^2+coordsX+coordsY)*kHat");
  calc->SetInputConnection(wavelet->GetOutputPort());
  calc->Update();

  vtkNew<vtkRegularPolygonSource> circle;
  circle->SetNumberOfSides(6);
  circle->SetRadius(1);
  circle->SetCenter(0, 0, 0);
  circle->SetNormal(0, 0, 1);
  circle->Update();
  circle->GetOutput()->GetPoints()->InsertNextPoint(circle->GetOutput()->GetPoint(0));

  vtkNew<vtkStreamSurface> stream;
  stream->SetMaximumPropagation(100);
  stream->SetMaximumNumberOfSteps(100);
  stream->SetInputConnection(0, calc->GetOutputPort());
  stream->SetInputConnection(1, circle->GetOutputPort());
  stream->SetInitialIntegrationStep(1);
  stream->SetIntegrationStepUnit(1);
  stream->SetIntegratorTypeToRungeKutta4();
  stream->SetUseIterativeSeeding(true);

  vtkNew<vtkDataSetMapper> streamMapper;
  streamMapper->SetInputConnection(stream->GetOutputPort());

  vtkNew<vtkDataSetMapper> waveletMapper;
  waveletMapper->SetInputConnection(wavelet->GetOutputPort());

  vtkNew<vtkActor> streamActor;
  streamActor->SetMapper(streamMapper);
  streamActor->GetProperty()->SetColor(0.1, 0.1, 0.1);
  streamActor->GetProperty()->SetRepresentationToWireframe();

  vtkNew<vtkActor> waveletActor;
  waveletActor->SetMapper(waveletMapper);
  waveletActor->GetProperty()->SetColor(0.4, 0.4, 0.4);
  waveletActor->GetProperty()->SetOpacity(0.1);
  waveletActor->GetProperty()->SetRepresentationToSurface();

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(waveletActor);
  renderer->AddActor(streamActor);
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
