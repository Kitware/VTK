// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware 2013
// This test was revised by Philippe Pebay, 2016
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkContourFilter.h"
#include "vtkDataSetMapper.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkHyperTreeGridToDualGrid.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestHyperTreeGridTernary2DMaterial(int argc, char* argv[])
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = 6;
  htGrid->SetDimensions(3, 4, 1);     // Dimension 2 in xy plane GridCell 2, 3, 1
  htGrid->SetGridScale(1.5, 1., 10.); // this is to test that orientation fixes scale
  htGrid->SetBranchFactor(3);
  htGrid->UseMaskOn();
  htGrid->SetDescriptor(
    "RRRRR.|......... ..R...... RRRRRRRRR R........ R........|..R...... ........R ......RRR "
    "......RRR ..R..R..R RRRRRRRRR R..R..R.. ......... ......... ......... ......... "
    ".........|......... ......... ......... ......... ......... ......... ......... ......... "
    "........R ..R..R..R ......... ......RRR ......R.. ......... RRRRRRRRR R..R..R.. ......... "
    "......... ......... ......... ......... ......... .........|......... ......... ......... "
    "......... ......... ......... ......... ......... ......... RRRRRRRRR ......... ......... "
    "......... ......... ......... ......... ......... ......... ......... .........|......... "
    "......... ......... ......... ......... ......... ......... ......... .........");
  htGrid->SetMask(
    "111111|000000000 111111111 111111111 111111111 111111111|111111111 000000001 000000111 "
    "011011111 001001001 111111111 100100100 001001001 111111111 111111111 111111111 "
    "001111111|111111111 001001001 111111111 111111111 111111111 111111111 111111111 111111111 "
    "001001111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 "
    "111111111 111111111 111111111 111111111 111111111 111111111|111111111 111111111 111111111 "
    "111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111 "
    "111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111|111111111 "
    "111111111 111111111 111111111 111111111 111111111 111111111 111111111 111111111");
  htGrid->SetMaxDepth(maxLevel);
  htGrid->Update();
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(htGrid->GetOutput());
  htg->GetCellData()->SetScalars(htg->GetCellData()->GetArray("Depth"));

  // DualGrid
  vtkNew<vtkHyperTreeGridToDualGrid> dualFilter;
  dualFilter->SetInputConnection(htGrid->GetOutputPort());

  // Geometry
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection(htGrid->GetOutputPort());
  geometry->Update();
  vtkPolyData* pd = geometry->GetPolyDataOutput();

  // Contour
  vtkNew<vtkContourFilter> contour;
  int nContours = 3;
  contour->SetNumberOfContours(nContours);
  contour->SetInputConnection(dualFilter->GetOutputPort());
  double resolution = (maxLevel - 1) / (nContours + 1.);
  double isovalue = resolution;
  for (int i = 0; i < nContours; ++i, isovalue += resolution)
  {
    contour->SetValue(i, isovalue);
  }

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(geometry->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetArray("Depth")->GetRange());
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(contour->GetOutputPort());
  mapper3->ScalarVisibilityOff();
  vtkNew<vtkDataSetMapper> mapper4;
  mapper4->SetInputConnection(dualFilter->GetOutputPort());
  mapper4->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);
  vtkNew<vtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetColor(.8, .4, .3);
  actor3->GetProperty()->SetLineWidth(3);
  vtkNew<vtkActor> actor4;
  actor4->SetMapper(mapper4);
  actor4->GetProperty()->SetRepresentationToWireframe();
  actor4->GetProperty()->SetColor(.0, .0, .0);

  // Camera
  double bd[6];
  pd->GetBounds(bd);
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(pd->GetCenter());
  camera->SetPosition(.5 * bd[1], .5 * bd[3], 6.);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->AddActor(actor3);
  renderer->AddActor(actor4);

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

  int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
