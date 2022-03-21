/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DContour.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Charles Gueunet, 2022
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGradient.h"
#include "vtkHyperTreeGridMapper.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestHyperTreeGridTernary3DGradient(int argc, char* argv[])
{
  // Hyper tree grid
  // From the TestHyperTreeGridTernary3DContour
  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = 5;
  htGrid->SetMaxDepth(maxLevel);
  htGrid->SetDimensions(4, 4, 3); // GridCell 3, 3, 2
  htGrid->SetGridScale(1.5, 1., .7);
  htGrid->SetBranchFactor(3);
  htGrid->SetDescriptor(
    "RRR .R. .RR ..R ..R .R.|R.......................... ........................... "
    "........................... .............R............. ....RR.RR........R......... "
    ".....RRRR.....R.RR......... ........................... ........................... "
    "...........................|........................... ........................... "
    "........................... ...RR.RR.......RR.......... ........................... "
    "RR......................... ........................... ........................... "
    "........................... ........................... ........................... "
    "........................... ........................... "
    "............RRR............|........................... ........................... "
    ".......RR.................. ........................... ........................... "
    "........................... ........................... ........................... "
    "........................... ........................... "
    "...........................|........................... ...........................");
  htGrid->Update();
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(htGrid->GetOutput());
  htg->GetCellData()->SetScalars(htg->GetCellData()->GetArray("Depth"));

  // Gradient
  vtkNew<vtkHyperTreeGridGradient> gradient;
  gradient->SetInputConnection(htGrid->GetOutputPort());
  gradient->Update();
  gradient->Print(std::cout);

  // Mappers
  // Surfacic, only shows the boundary. Not the best fit for a gradient computation
  vtkNew<vtkHyperTreeGridMapper> mapper1;
  mapper1->SetInputConnection(gradient->GetOutputPort());

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetBackground(0., 0., 0.);
  renderer->AddActor(actor1);

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

  int retVal = vtkRegressionTestImageThreshold(renWin, 60);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
