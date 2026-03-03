// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test vtkFeatureEdgesDispatcher with a 2D binary HTG containing masked cells.

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFeatureEdgesDispatcher.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTestUtilities.h"
#include "vtkXMLHyperTreeGridReader.h"

int TestFeatureEdgesDispatcher(int argc, char* argv[])
{
  // HTG reader — binary 2D XY-oriented HTG containing masked cells
  vtkNew<vtkXMLHyperTreeGridReader> reader;
  char* fileNameC =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/HTG/binary_2D_XY_331_mask.htg");
  reader->SetFileName(fileNameC);
  delete[] fileNameC;

  // Feature edges dispatcher
  vtkNew<vtkFeatureEdgesDispatcher> featureEdges;
  featureEdges->SetInputConnection(reader->GetOutputPort());
  featureEdges->SetMergePoints(true);

  // Mapper
  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(featureEdges->GetOutputPort());

  // Actor
  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetLineWidth(2.0);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->ResetCamera();

  // Render window
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  // Interactor
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
