/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridBinary2DAxisClipEllipse.cxx

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
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyLine.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"

int TestHyperTreeGridBinary2DAxisClipEllipse(int argc, char* argv[])
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

  // Axis clip
  vtkNew<vtkHyperTreeGridAxisClip> clip;
  clip->SetInputConnection(htGrid->GetOutputPort());
  clip->SetClipTypeToQuadric();
  double a = .99;
  double b = .465;
  double x0 = 1.17;
  double y0 = 1.1;
  double a2 = a * a;
  double b2 = b * b;
  double b2x0 = b2 * x0;
  double a2y0 = a2 * y0;
  double q[10];
  q[0] = b2;
  q[1] = a2;
  q[2] = 1.;
  q[3] = 0.;
  q[4] = 0.;
  q[5] = 0.;
  q[6] = -2. * b2x0;
  q[7] = -2. * a2y0;
  q[8] = 0.;
  q[9] = x0 * b2x0 + y0 * a2y0 - a2 * b2;
  clip->SetQuadricCoefficients(q);

  // Geometries
  vtkNew<vtkHyperTreeGridGeometry> geometry1;
  geometry1->SetInputConnection(htGrid->GetOutputPort());
  geometry1->Update();
  vtkPolyData* pd = geometry1->GetPolyDataOutput();
  vtkNew<vtkHyperTreeGridGeometry> geometry2;
  geometry2->SetInputConnection(clip->GetOutputPort());

  // Ellipse
  vtkNew<vtkPoints> points;
  double pt[3];
  pt[2] = 0.;
  vtkIdType np = 500;
  double sec = 2. * vtkMath::Pi() / np;
  double arg = 0.;
  vtkNew<vtkPolyLine> polyLine;
  polyLine->GetPointIds()->SetNumberOfIds(np + 1);
  for (vtkIdType i = 0; i < np; ++i, arg += sec)
  {
    pt[0] = x0 + a * cos(arg);
    pt[1] = y0 + b * sin(arg);
    points->InsertNextPoint(pt);
    polyLine->GetPointIds()->SetId(i, i);
  } // i
  polyLine->GetPointIds()->SetId(np, 0);
  vtkNew<vtkCellArray> edges;
  edges->InsertNextCell(polyLine);
  vtkNew<vtkPolyData> ellipse;
  ellipse->SetPoints(points);
  ellipse->SetLines(edges);

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkDataSetMapper> mapper1;
  mapper1->SetInputConnection(geometry2->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry1->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper3;
  mapper3->SetInputData(ellipse);
  mapper3->ScalarVisibilityOff();

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

  // Camera
  vtkHyperTreeGrid* ht = htGrid->GetHyperTreeGridOutput();
  double bd[6];
  ht->GetBounds(bd);
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
