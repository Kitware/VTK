/*==================================================================

  Program:   Visualization Toolkit
  Module:    TestHyperTreeGridTernarySphereMaterialReflections.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

===================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware 2012
// This test was revised by Philippe Pebay, 2016
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisReflection.h"
#include "vtkHyperTreeGridGeometry.h"
#include "vtkHyperTreeGridSource.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkQuadric.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTimerLog.h"

//#define HYPERTREEGRID_GETRUSAGE
#ifdef HYPERTREEGRID_GETRUSAGE
#include <sys/resource.h>
#endif

int TestHyperTreeGridTernarySphereMaterialReflections(int argc, char* argv[])
{
  // Performance instruments
  vtkNew<vtkTimerLog> timer;
#ifdef HYPERTREEGRID_GETRUSAGE
  struct rusage usage0;
  getrusage(RUSAGE_SELF, &usage0);
#endif

  // Hyper tree grid
  vtkNew<vtkHyperTreeGridSource> htGrid;
  htGrid->SetMaxDepth(4);
  htGrid->SetDimensions(6, 6, 7); // GridCell 5, 5, 6
  htGrid->SetGridScale(1.5, 1., .7);
  htGrid->SetBranchFactor(3);
  htGrid->UseDescriptorOff();
  htGrid->UseMaskOn();
  vtkNew<vtkQuadric> quadric;
  quadric->SetCoefficients(1., 1., 1., 0, 0., 0., 0.0, 0., 0., -25.);
  htGrid->SetQuadric(quadric);
  timer->StartTimer();
  htGrid->Update();
  timer->StopTimer();
#ifdef HYPERTREEGRID_GETRUSAGE
  struct rusage usage1;
  getrusage(RUSAGE_SELF, &usage1);
#endif
  vtkHyperTreeGrid* H = vtkHyperTreeGrid::SafeDownCast(htGrid->GetOutput());
  vtkIdType nV = H->GetNumberOfVertices();
  vtkIdType nL = H->GetNumberOfLeaves();
  cerr << "Time for 1 HyperTreeGridSource: " << timer->GetElapsedTime() << endl;
  cerr << "  number of tree vertices: " << nV << endl;
  cerr << "  number of tree leaves: " << nL << " (" << (double)nL / (double)nV * 100.00 << "%)\n";
#ifdef HYPERTREEGRID_GETRUSAGE
  cerr << "  increase in max. resident set size: " << (usage1.ru_maxrss - usage0.ru_maxrss) / 1024
       << " kiB\n";
#endif

  // Axis reflections
  timer->StartTimer();
  vtkNew<vtkHyperTreeGridAxisReflection> reflection1;
  reflection1->SetInputConnection(htGrid->GetOutputPort());
  reflection1->SetPlaneToXMin();
  reflection1->Update();
  vtkNew<vtkHyperTreeGridAxisReflection> reflection2;
  reflection2->SetInputConnection(htGrid->GetOutputPort());
  reflection2->SetPlaneToYMin();
  reflection2->Update();
  vtkNew<vtkHyperTreeGridAxisReflection> reflection3;
  reflection3->SetInputConnection(htGrid->GetOutputPort());
  reflection3->SetPlaneToZMin();
  reflection3->Update();
  vtkNew<vtkHyperTreeGridAxisReflection> reflection4;
  reflection4->SetInputConnection(reflection1->GetOutputPort());
  reflection4->SetPlaneToYMin();
  reflection4->Update();
  vtkNew<vtkHyperTreeGridAxisReflection> reflection5;
  reflection5->SetInputConnection(reflection2->GetOutputPort());
  reflection5->SetPlaneToZMin();
  reflection5->Update();
  vtkNew<vtkHyperTreeGridAxisReflection> reflection6;
  reflection6->SetInputConnection(reflection5->GetOutputPort());
  reflection6->SetPlaneToXMin();
  reflection6->Update();
  vtkNew<vtkHyperTreeGridAxisReflection> reflection7;
  reflection7->SetInputConnection(reflection6->GetOutputPort());
  reflection7->SetPlaneToYMax();
  reflection7->Update();
  timer->StopTimer();
  cerr << "Time for 7 axis-aligned reflections: " << timer->GetElapsedTime() << endl;
#ifdef HYPERTREEGRID_GETRUSAGE
  struct rusage usage2;
  getrusage(RUSAGE_SELF, &usage2);
  cerr << "  increase in max. resident set size: " << (usage2.ru_maxrss - usage1.ru_maxrss) / 1024
       << " kiB\n";
#endif

  // Geometries
  timer->StartTimer();
  vtkNew<vtkHyperTreeGridGeometry> geometry;
  geometry->SetInputConnection(htGrid->GetOutputPort());
  geometry->Update();
  vtkPolyData* pd = geometry->GetPolyDataOutput();
  vtkNew<vtkHyperTreeGridGeometry> geometry1;
  geometry1->SetInputConnection(reflection1->GetOutputPort());
  geometry1->Update();
  vtkNew<vtkHyperTreeGridGeometry> geometry2;
  geometry2->SetInputConnection(reflection2->GetOutputPort());
  geometry2->Update();
  timer->StopTimer();
  vtkNew<vtkHyperTreeGridGeometry> geometry3;
  geometry3->SetInputConnection(reflection3->GetOutputPort());
  geometry3->Update();
  vtkNew<vtkHyperTreeGridGeometry> geometry4;
  geometry4->SetInputConnection(reflection4->GetOutputPort());
  geometry4->Update();
  vtkNew<vtkHyperTreeGridGeometry> geometry5;
  geometry5->SetInputConnection(reflection5->GetOutputPort());
  geometry5->Update();
  vtkNew<vtkHyperTreeGridGeometry> geometry6;
  geometry6->SetInputConnection(reflection6->GetOutputPort());
  geometry6->Update();
  vtkNew<vtkHyperTreeGridGeometry> geometry7;
  geometry7->SetInputConnection(reflection7->GetOutputPort());
  geometry7->Update();
  timer->StopTimer();
  cerr << "Time for 8 geometry filters: " << timer->GetElapsedTime() << endl;
#ifdef HYPERTREEGRID_GETRUSAGE
  struct rusage usage3;
  getrusage(RUSAGE_SELF, &usage3);
  cerr << "  increase in max. resident set size: " << (usage3.ru_maxrss - usage2.ru_maxrss) / 1024
       << " kiB\n";
#endif

  // Mappers
  vtkMapper::SetResolveCoincidentTopologyToPolygonOffset();
  vtkNew<vtkPolyDataMapper> mapper1;
  mapper1->SetInputConnection(geometry->GetOutputPort());
  mapper1->SetScalarRange(pd->GetCellData()->GetScalars()->GetRange());
  vtkNew<vtkPolyDataMapper> mapper2;
  mapper2->SetInputConnection(geometry->GetOutputPort());
  mapper2->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper3;
  mapper3->SetInputConnection(geometry1->GetOutputPort());
  mapper3->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper4;
  mapper4->SetInputConnection(geometry2->GetOutputPort());
  mapper4->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper5;
  mapper5->SetInputConnection(geometry3->GetOutputPort());
  mapper5->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper6;
  mapper6->SetInputConnection(geometry4->GetOutputPort());
  mapper6->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper7;
  mapper7->SetInputConnection(geometry5->GetOutputPort());
  mapper7->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper8;
  mapper8->SetInputConnection(geometry6->GetOutputPort());
  mapper8->ScalarVisibilityOff();
  vtkNew<vtkPolyDataMapper> mapper9;
  mapper9->SetInputConnection(geometry7->GetOutputPort());
  mapper9->ScalarVisibilityOff();

  // Actors
  vtkNew<vtkActor> actor1;
  actor1->SetMapper(mapper1);
  vtkNew<vtkActor> actor2;
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetRepresentationToWireframe();
  actor2->GetProperty()->SetColor(.7, .7, .7);
  vtkNew<vtkActor> actor3;
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetColor(.8, .0, .0);
  vtkNew<vtkActor> actor4;
  actor4->SetMapper(mapper4);
  actor4->GetProperty()->SetColor(.0, .8, .0);
  vtkNew<vtkActor> actor5;
  actor5->SetMapper(mapper5);
  actor5->GetProperty()->SetColor(.0, .0, .8);
  vtkNew<vtkActor> actor6;
  actor6->SetMapper(mapper6);
  actor6->GetProperty()->SetColor(.8, .8, .0);
  vtkNew<vtkActor> actor7;
  actor7->SetMapper(mapper7);
  actor7->GetProperty()->SetColor(.0, .8, .8);
  vtkNew<vtkActor> actor8;
  actor8->SetMapper(mapper8);
  actor8->GetProperty()->SetColor(.8, .0, .8);
  vtkNew<vtkActor> actor9;
  actor9->SetMapper(mapper9);
  actor9->GetProperty()->SetRepresentationToWireframe();
  actor9->GetProperty()->SetColor(.4, .4, .4);

  // Camera
  vtkNew<vtkCamera> camera;
  camera->SetClippingRange(1., 100.);
  camera->SetPosition(-10., -10., 15.);

  // Renderer
  vtkNew<vtkRenderer> renderer;
  renderer->SetActiveCamera(camera);
  renderer->SetBackground(1., 1., 1.);
  renderer->AddActor(actor1);
  renderer->AddActor(actor2);
  renderer->AddActor(actor3);
  renderer->AddActor(actor4);
  renderer->AddActor(actor5);
  renderer->AddActor(actor6);
  renderer->AddActor(actor7);
  renderer->AddActor(actor8);
  renderer->AddActor(actor9);

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

  int retVal = vtkRegressionTestImageThreshold(renWin, 110);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
