/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridToDualGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// This test verifies that VTK can obtain the dual grid representation
// for a HyperTreeGrid.

#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetMapper.h"
#include "vtkHyperTreeGridToDualGrid.h"
#include "vtkHyperTreeGridToUnstructuredGrid.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkUnstructuredGrid.h"

#include "vtkDataSetWriter.h"

int TestHyperTreeGridToDualGrid(int argc, char* argv[])
{
  // Writer for debug
  // vtkNew<vtkDataSetWriter> writer;

  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = 6;
  htGrid->SetMaxDepth(maxLevel);
  htGrid->SetDimensions(3, 4, 1);     // Dimension 2 in xy plane GridCell 2, 3
  htGrid->SetGridScale(1.5, 1., 10.); // this is to test that orientation fixes scale
  htGrid->SetBranchFactor(2);
  htGrid->SetDescriptor("RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
                        "...R ..R. .... .R.. R...|.... .... .R.. ....|....");

  // Geometry
  vtkNew<vtkHyperTreeGridToDualGrid> dualfilter;
  dualfilter->SetInputConnection(htGrid->GetOutputPort());
  dualfilter->Update();
  vtkUnstructuredGrid* dual = vtkUnstructuredGrid::SafeDownCast(dualfilter->GetOutput());
  // dual->PrintSelf(cerr, vtkIndent(0));
  // writer->SetFileName("fooDual.vtk");
  // writer->SetInputData(dual);
  // writer->Write();

  vtkNew<vtkHyperTreeGridToUnstructuredGrid> gfilter;
  gfilter->SetInputConnection(htGrid->GetOutputPort());
  gfilter->Update();
  // vtkUnstructuredGrid *primal = vtkUnstructuredGrid::SafeDownCast(gfilter->GetOutput());
  // writer->SetFileName("fooPrimal.vtk");
  // writer->SetInputData(primal);
  // writer->Write();

  vtkNew<vtkHyperTreeGridGeometry> sfilter;
  sfilter->SetInputConnection(htGrid->GetOutputPort());
  sfilter->Update();
  // vtkPolyData *skin = vtkPolyData::SafeDownCast(sfilter->GetOutput());
  // writer->SetFileName("fooSkin.vtk");
  // writer->SetInputData(skin);
  // writer->Write();

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(dualfilter->GetOutputPort());
  // mapper1->SetScalarRange( dual->GetCellData()->GetScalars()->GetRange() ); //no cell data yet
  vtkNew<vtkDataSetMapper> mapper2;
  mapper2->SetInputConnection(dualfilter->GetOutputPort());
  mapper2->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);

  // Camera
  double bd[6];
  dual->GetBounds(bd);
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(dual->GetCenter());
  camera->SetPosition(.5 * (bd[0] + bd[1]), .5 * (bd[2] + bd[3]), 6.);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

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

  int retVal = vtkRegressionTestImageThreshold(renWin, 2);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
