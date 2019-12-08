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
#include "vtkDataSetMapper.h"
#include "vtkEvenlySpacedStreamlines2D.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStreamTracer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLMultiBlockDataReader.h"

int TestEvenlySpacedStreamlines2D(int argc, char* argv[])
{
  char* fname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/clt.vtm");
  auto reader = vtkSmartPointer<vtkXMLMultiBlockDataReader>::New();
  reader->SetFileName(fname);
  delete[] fname;
  reader->Update();

  auto stream = vtkSmartPointer<vtkEvenlySpacedStreamlines2D>::New();
  stream->SetInputConnection(reader->GetOutputPort());
  stream->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "result");
  stream->SetInitialIntegrationStep(0.2);
  stream->SetClosedLoopMaximumDistance(0.2);
  stream->SetMaximumNumberOfSteps(2000);
  stream->SetSeparatingDistance(2);
  stream->SetSeparatingDistanceRatio(0.3);
  stream->SetStartPosition(0, 0, 200);

  auto streamMapper = vtkSmartPointer<vtkDataSetMapper>::New();
  streamMapper->SetInputConnection(stream->GetOutputPort());
  streamMapper->ScalarVisibilityOff();

  auto streamActor = vtkSmartPointer<vtkActor>::New();
  streamActor->SetMapper(streamMapper);
  streamActor->GetProperty()->SetColor(0, 0, 0);
  streamActor->GetProperty()->SetLineWidth(1.);
  streamActor->SetPosition(0, 0, 1);

  auto renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->AddActor(streamActor);
  renderer->ResetCamera();
  renderer->SetBackground(1., 1., 1.);

  auto renWin = vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  renWin->SetSize(300, 300);

  auto iren = vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
