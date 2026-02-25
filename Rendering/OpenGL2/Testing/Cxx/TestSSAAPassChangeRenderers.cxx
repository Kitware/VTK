// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This test exercises changing renderers with the same SSAAPass.
// It does not verify the output image. The passing criteria is that
// the test should not print any error messages, and should not crash.
// The TestSSAAPass.cxx test verifies the correctness of the SSAAPass output, so if that test is
// passing, we can be reasonably sure that the SSAAPass is working correctly in this test

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderStepsPass.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSSAAPass.h"
#include "vtkTestUtilities.h"

int TestSSAAPassChangeRenderers(int argc, char* argv[])
{
  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);
  iren->SetRenderWindow(renWin);

  vtkNew<vtkRenderer> renderer1;
  renWin->AddRenderer(renderer1);

  vtkNew<vtkActor> actor;
  vtkNew<vtkPolyDataMapper> mapper;
  renderer1->AddActor(actor);
  actor->SetMapper(mapper);
  actor->GetProperty()->SetLineWidth(2);

  // create the basic VTK render steps
  vtkNew<vtkRenderStepsPass> basicPasses;
  vtkNew<vtkSSAAPass> ssaa;
  ssaa->SetDelegatePass(basicPasses);

  // tell the renderer to use our render pass pipeline
  renderer1->SetPass(ssaa);

  renWin->SetSize(500, 500);

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  mapper->SetInputConnection(reader->GetOutputPort());
  renderer1->ResetCamera();
  renWin->Render();
  vtkLog(INFO, << "Finished first render");

  vtkNew<vtkRenderer> renderer2;
  renWin->RemoveRenderer(renderer1);
  renWin->AddRenderer(renderer2);
  renderer2->SetPass(ssaa);
  renderer2->AddActor(actor);
  renderer2->ResetCamera();
  renWin->Render();
  vtkLog(INFO, << "Finished second render");

  return EXIT_SUCCESS;
}
