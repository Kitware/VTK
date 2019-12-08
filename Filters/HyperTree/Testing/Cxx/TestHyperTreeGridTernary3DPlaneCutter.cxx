/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernary3DPlaneCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay and Rogeli Grima, 2016
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridPlaneCutter.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkHyperTreeGridToUnstructuredGrid.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetMapper.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShrinkFilter.h"
#include "vtkUnstructuredGrid.h"

int TestHyperTreeGridTernary3DPlaneCutter(int argc, char* argv[])
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(5);
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

  // Hyper tree grid to unstructured grid filter
  vtkNew<vtkHyperTreeGridToUnstructuredGrid> htg2ug;
  htg2ug->SetInputConnection(htGrid->GetOutputPort());
  htg2ug->Update();
  vtkUnstructuredGrid* ug = htg2ug->GetUnstructuredGridOutput();
  double* range = ug->GetCellData()->GetScalars()->GetRange();

  // Plane cutters
  vtkNew<vtkHyperTreeGridPlaneCutter> cut1;
  cut1->SetInputConnection(htGrid->GetOutputPort());
  cut1->SetPlane(1., -.2, .2, 3.);
  vtkNew<vtkHyperTreeGridPlaneCutter> cut2;
  cut2->SetInputConnection(htGrid->GetOutputPort());
  cut2->SetPlane(-.2, -.6, 1., .05);

  // Geometry
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection(htGrid->GetOutputPort());
  geometry->Update();

  // Shrinks
  vtkNew<vtkShrinkFilter> shrink1;
  shrink1->SetInputConnection(cut1->GetOutputPort());
  shrink1->SetShrinkFactor(.95);
  vtkNew<vtkShrinkFilter> shrink2;
  shrink2->SetInputConnection(cut2->GetOutputPort());
  shrink2->SetShrinkFactor(.95);

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(shrink1->GetOutputPort());
  mapper1->SetScalarRange(range);
  vtkNew<vtkDataSetMapper> mapper2;
  mapper2->SetInputConnection(shrink2->GetOutputPort());
  mapper2->SetScalarRange(range);
  vtkNew<vtkDataSetMapper> mapper3;
  mapper3->SetInputConnection(htg2ug->GetOutputPort());
  mapper3->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  vtkNew<vtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetRepresentationToWireframe();
  actor3->GetProperty()->SetColor(.7, .7, .7);

  // Camera
  double bd[6];
  ug->GetBounds(bd);
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(ug->GetCenter());
  camera->SetPosition(-.8 * bd[1], 2.1 * bd[3], -4.8 * bd[5]);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->AddActor(actor3);

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

  int retVal = vtkRegressionTestImageThreshold(renWin, 50);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
