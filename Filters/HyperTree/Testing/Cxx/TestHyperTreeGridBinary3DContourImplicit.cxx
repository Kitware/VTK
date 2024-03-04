// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridContour.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestHyperTreeGridBinary3DContourImplicit(int argc, char* argv[])
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = 5;
  htGrid->SetMaxDepth(maxLevel);
  htGrid->SetDimensions(4, 4, 3); // Dimension 3 GridCell 3, 3, 2
  htGrid->SetGridScale(1.5, 1., .7);
  htGrid->SetBranchFactor(2);
  htGrid->SetDescriptor(
    "RRR .R. .RR ..R ..R .R.|R....... ........ ........ ...R.... .RRRR.R. RRRRR.RR ........ "
    "........ ........|........ ........ ........ RR.RR.RR ........ RR...... ........ ........ "
    "........ ........ ........ ........ ........ ..RRR...|........ ..R..... ........ ........ "
    "........ ........ ........ ........ ........ ........ ........|........");
  htGrid->Update();

  // Set scalars to contour with
  vtkHyperTreeGrid* htg = vtkHyperTreeGrid::SafeDownCast(htGrid->GetOutput());
  htg->GetCellData()->SetScalars(htg->GetCellData()->GetArray("Depth"));

  // Contour
  vtkNew<vtkHyperTreeGridContour> contour;
  contour->SetInputConnection(htGrid->GetOutputPort());
  int nContours = 4;
  contour->SetNumberOfContours(nContours);
  contour->SetInputConnection(htGrid->GetOutputPort());
  double resolution = (maxLevel - 1) / (nContours + 1.);
  double isovalue = resolution;
  for (int i = 0; i < nContours; ++i, isovalue += resolution)
  {
    contour->SetValue(i, isovalue);
  }

  // Use implict arrays to store contouring values ("Depth") in the output contour
  contour->SetUseImplicitArrays(true);
  contour->Update();

  // Since the output "Depth" array have been replaced by an implicit array,
  // the input "Depth" scalars status is not preserved.
  // We should explicitely set the scalars here
  vtkPolyData* contourPd = contour->GetPolyDataOutput();
  contourPd->GetPointData()->SetScalars(contourPd->GetPointData()->GetArray("Depth"));

  // Geometry
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection(htGrid->GetOutputPort());
  geometry->Update();
  vtkPolyData* geometryPd = geometry->GetPolyDataOutput();

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkPolyDataMapper> contourMapper;
  contourMapper->SetInputConnection(contour->GetOutputPort());
  contourMapper->SetScalarRange(contourPd->GetPointData()->GetArray("Depth")->GetRange());
  vtkNew<vtkPolyDataMapper> contourMapperWireframe;
  contourMapperWireframe->SetInputConnection(contour->GetOutputPort());
  contourMapperWireframe->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> geometryMapper;
  geometryMapper->SetInputConnection(geometry->GetOutputPort());
  geometryMapper->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> contourActor;
  contourActor->SetMapper(contourMapper);
  vtkNew<vtkActor> contourWireframeActor;
  contourWireframeActor->SetMapper(contourMapperWireframe);
  contourWireframeActor->GetProperty()->SetRepresentationToWireframe();
  contourWireframeActor->GetProperty()->SetColor(.3, .3, .3);
  contourWireframeActor->GetProperty()->SetLineWidth(1);
  vtkNew<vtkActor> geometryActor;
  geometryActor->SetMapper(geometryMapper);
  geometryActor->GetProperty()->SetRepresentationToWireframe();
  geometryActor->GetProperty()->SetColor(.7, .7, .7);

  // Camera
  double bd[6];
  geometryPd->GetBounds(bd);
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetFocalPoint(geometryPd->GetCenter());
  camera->SetPosition(-.8 * bd[1], 2.1 * bd[3], -4.8 * bd[5]);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(contourActor);
  renderer->AddActor(contourWireframeActor);
  renderer->AddActor(geometryActor);

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
