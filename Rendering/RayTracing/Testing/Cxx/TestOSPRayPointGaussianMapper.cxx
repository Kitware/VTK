/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayPointGaussianMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Description
// This is a test for the point gaussian mapper in the ospray backend

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkOSPRayPass.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkPointGaussianMapper.h"
#include "vtkPointSource.h"
#include "vtkRandomAttributeGenerator.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"

#include "vtkOSPRayTestInteractor.h"

int TestOSPRayPointGaussianMapper(int argc, char* argv[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  int desiredPoints = 1.0e3;

  // Create the RenderWindow, Renderer and both Actors
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0.1, 0.2, 0.2);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  renderWindow->SetMultiSamples(0);
  renderWindow->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  vtkOSPRayRendererNode::SetSamplesPerPixel(16, renderer);

  // Create the points
  vtkNew<vtkPointSource> points;
  points->SetNumberOfPoints(desiredPoints);
  points->SetRadius(pow(desiredPoints, 0.33) * 20.0);
  points->Update();

  vtkNew<vtkRandomAttributeGenerator> randomAttr;
  randomAttr->SetInputConnection(points->GetOutputPort());

  vtkNew<vtkActor> actor;
  vtkNew<vtkPointGaussianMapper> mapper;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  randomAttr->SetDataTypeToFloat();
  randomAttr->GeneratePointScalarsOn();
  randomAttr->GeneratePointVectorsOn();
  randomAttr->Update();

  mapper->SetInputConnection(randomAttr->GetOutputPort());
  mapper->SetTriangleScale(3.0);
  mapper->SetScaleFactor(0.75);
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("RandomPointScalars");
  mapper->SetInterpolateScalarsBeforeMapping(0);
  mapper->SetScaleArray("RandomPointVectors");
  mapper->SetScaleArrayComponent(3);

  // Note that LookupTable is 4x faster than
  // ColorTransferFunction. So if you have a choice
  // Usa a lut instead.
  //
  vtkNew<vtkLookupTable> lut;
  lut->SetHueRange(0.1, 0.2);
  lut->SetSaturationRange(1.0, 0.5);
  lut->SetValueRange(0.8, 1.0);
  lut->Build();
  mapper->SetLookupTable(lut);

  vtkNew<vtkOSPRayPass> ospray;
  renderer->SetPass(ospray);
  vtkNew<vtkOSPRayTestInteractor> style;
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(2.0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
