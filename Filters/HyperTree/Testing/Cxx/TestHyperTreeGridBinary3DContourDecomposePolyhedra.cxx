// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// .SECTION Thanks
// Funded by CEA, DAM, DIF, F-91297 Arpajon, France

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridContour.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"
#include "vtkHyperTreeGridToDualGrid.h"
#include "vtkHyperTreeGridToUnstructuredGrid.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

#include "vtkHyperTreeGridSource.h"

int TestHyperTreeGridBinary3DContourDecomposePolyhedra(int argc, char* argv[])
{
  constexpr int depth = 5;
  constexpr int nbOfContours = 4;

  // Generate HTG
  vtkNew<vtkHyperTreeGridSource> htgSource;
  htgSource->SetMaxDepth(depth);
  htgSource->SetDimensions(4, 4, 3); // Dimension 3 GridCell 3, 3, 2
  htgSource->SetGridScale(1.5, 1., .7);
  htgSource->SetBranchFactor(2);
  htgSource->SetDescriptor(
    "RRR .R. .RR ..R ..R .R.|R....... ........ ........ ...R.... .RRRR.R. RRRRR.RR ........ "
    "........ ........|........ ........ ........ RR.RR.RR ........ RR...... ........ ........ "
    "........ ........ ........ ........ ........ ..RRR...|........ ..R..... ........ ........ "
    "........ ........ ........ ........ ........ ........ ........|........");
  htgSource->Update();

  // Set contouring scalars
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(htgSource->GetOutput());
  htg->GetCellData()->SetScalars(htg->GetCellData()->GetArray("Depth"));

  // Contour
  vtkNew<vtkHyperTreeGridContour> contour;
  contour->SetInputConnection(htgSource->GetOutputPort());
  contour->SetStrategy3D(vtkHyperTreeGridContour::USE_DECOMPOSED_POLYHEDRA);
  contour->SetNumberOfContours(nbOfContours);
  double resolution = (depth - 1) / (nbOfContours + 1.);
  double isovalue = resolution;
  for (int i = 0; i < nbOfContours; ++i, isovalue += resolution)
  {
    contour->SetValue(i, isovalue);
  }

  // Geometry
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection(htgSource->GetOutputPort());
  geometry->Update();
  vtkPolyData* pd = geometry->GetPolyDataOutput();

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(contour->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetArray("Depth")->GetRange());
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(contour->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(geometry->GetOutputPort());
  mapper3->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.3, .3, .3);
  actor2->GetProperty()->SetLineWidth(1);
  vtkNew<vtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetRepresentationToWireframe();
  actor3->GetProperty()->SetColor(.7, .7, .7);

  // Camera
  double bd[6] = { 0. };
  pd->GetBounds(bd);
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(pd->GetCenter());
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

  int retVal = vtkRegressionTestImageThreshold(renWin, 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
