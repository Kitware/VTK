/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridBinary2DAxisClipPlanes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, 2016
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisClip.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkDataSetMapper.h"
#include "vtkLineSource.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestHyperTreeGridBinary2DAxisClipPlanes(int argc, char* argv[])
{
  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  int maxLevel = 6;
  htGrid->SetMaxDepth(maxLevel);
  htGrid->SetDimensions(3, 4, 1);     // Dimension 2 in xy plane GridCell 2, 3
  htGrid->SetGridScale(1.5, 1., 10.); // this is to test that orientation fixes scale
  htGrid->SetBranchFactor(2);
  htGrid->SetDescriptor("RRRRR.|.... .R.. RRRR R... R...|.R.. ...R ..RR .R.. R... .... ....|.... "
                        "...R ..R. .... .R.. R...|.... .... .R.. ....|....");

  // Axis clips
  vtkNew<vtkHyperTreeGridAxisClip> clip1;
  clip1->SetInputConnection(htGrid->GetOutputPort());
  clip1->SetPlaneNormalAxis(0);
  double k0 = 1.;
  clip1->SetPlanePosition(k0);
  clip1->InsideOutOff();
  vtkNew<vtkHyperTreeGridAxisClip> clip2;
  clip2->SetInputConnection(clip1->GetOutputPort());
  clip2->SetPlaneNormalAxis(1);
  double k1 = 1.2;
  clip2->SetPlanePosition(k1);
  clip2->InsideOutOn();

  // Geometries
  vtkNew<vtkHyperTreeGridGeometry> geometry1;
  geometry1->SetInputConnection(htGrid->GetOutputPort());
  geometry1->Update();
  vtkPolyData* pd = geometry1->GetPolyDataOutput();
  vtkNew<vtkHyperTreeGridGeometry> geometry2;
  geometry2->SetInputConnection(clip2->GetOutputPort());

  // Lines
  vtkHyperTreeGrid* ht = htGrid->GetHyperTreeGridOutput();
  double bd[6];
  ht->GetBounds(bd);
  double x0 = bd[0] - .1;
  double x1 = bd[1] + .1;
  double y0 = bd[2] - .1;
  double y1 = bd[3] + .1;
  vtkNew<vtkLineSource> line1;
  line1->SetPoint1(k0, y0, 0.);
  line1->SetPoint2(k0, y1, 0.);
  vtkNew<vtkLineSource> line2;
  line2->SetPoint1(x0, k1, 0.);
  line2->SetPoint2(x1, k1, 0.);

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(geometry2->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry1->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(line1->GetOutputPort());
  mapper3->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper4;
  mapper4->SetInputConnection(line2->GetOutputPort());
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
  actor3->GetProperty()->SetColor(.3, .3, .3);
  actor3->GetProperty()->SetLineWidth(3);
  vtkNew<vtkActor> actor4;
  actor4->SetMapper(mapper4);
  actor4->GetProperty()->SetColor(.3, .3, .3);
  actor4->GetProperty()->SetLineWidth(3);

  // Camera
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

  int retVal = vtkRegressionTestImageThreshold(renWin, 70);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
