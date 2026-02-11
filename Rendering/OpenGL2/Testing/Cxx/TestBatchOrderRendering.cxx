// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkCutter.h"
#include "vtkDataArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkPlane.h"
#include "vtkRTAnalyticSource.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include <cstdlib>

namespace
{

/**
 * We need a very particular configuration of overlapped blocks to reproduce
 * the original bug (2 cells aren't rendered after applying an unaligned plane cut).
 */
void CreateTestData(vtkMultiBlockDataSet* mbds)
{
  vtkNew<vtkRTAnalyticSource> rtSource1;
  rtSource1->SetWholeExtent(-2, 2, -2, 2, -2, 2);
  rtSource1->Update();

  vtkNew<vtkRTAnalyticSource> rtSource2;
  rtSource2->SetWholeExtent(-2, 0, -2, 0, -2, 0);
  rtSource2->Update();

  vtkNew<vtkRTAnalyticSource> rtSource3;
  rtSource3->SetWholeExtent(0, 2, -2, 0, -2, 0);
  rtSource3->Update();

  mbds->SetBlock(0, rtSource1->GetOutputDataObject(0));
  mbds->SetBlock(1, rtSource2->GetOutputDataObject(0));
  mbds->SetBlock(2, rtSource3->GetOutputDataObject(0));
}

} // end anonymous namespace

/*
 * This test ensure that rendering composite dataset with the batch polydata mapper works correctly
 * when the cut plane is unaligned with the axes.
 */
int TestBatchOrderRendering(int argc, char* argv[])
{

  vtkNew<vtkMultiBlockDataSet> multiblock;
  ::CreateTestData(multiblock);

  vtkNew<vtkCutter> cutter;
  cutter->SetInputData(multiblock);
  vtkNew<vtkPlane> plane;
  plane->SetOrigin(0, 0, 0);
  // Unalignment is intended
  plane->SetNormal(1, 0, 1);
  cutter->SetCutFunction(plane);
  cutter->Update();

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(cutter->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  // Standard testing code.
  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(actor);
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->AddRenderer(renderer);
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
