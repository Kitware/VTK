#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkFeatureEdges.h"
#include "vtkJogglePoints.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkOutlineFilter.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkTesting.h"
#include "vtkTimerLog.h"
#include "vtkVoronoiFlower3D.h"

#include <iostream>

// Construct an octahedron with the bottom vertex cut off,
// and the top vertex a slight (degenerate) graze. This tests
// self-degeneracies, and prunes.
int TestVoronoiHull2(int argc, char* argv[])
{
  // Generate some point
  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  pts->InsertNextPoint(0, 0, 0);

  pts->InsertNextPoint(-1, -1, -0.75);
  pts->InsertNextPoint(1, -1, -0.75);
  pts->InsertNextPoint(1, 1, -0.75);
  pts->InsertNextPoint(-1, 1, -0.75);

  pts->InsertNextPoint(-1, -1, 0.75);
  pts->InsertNextPoint(1, -1, 0.75);
  pts->InsertNextPoint(1, 1, 0.75);
  pts->InsertNextPoint(-1, 1, 0.75);

  pts->InsertNextPoint(-5, -5, -5);
  pts->InsertNextPoint(5, -5, -5);
  pts->InsertNextPoint(-5, 5, -5);
  pts->InsertNextPoint(5, 5, -5);
  pts->InsertNextPoint(-5, -5, 5);
  pts->InsertNextPoint(5, -5, 5);
  pts->InsertNextPoint(-5, 5, 5);
  pts->InsertNextPoint(5, 5, 5);

  vtkNew<vtkPolyData> mesh;
  mesh->SetPoints(pts);

  // Optional point joggling
  vtkNew<vtkJogglePoints> joggle;
  joggle->SetInputData(mesh);
  joggle->Update();

  // Voronoi-based surface
  vtkNew<vtkVoronoiFlower3D> v;
  v->SetInputData(mesh);
  v->SetInputConnection(joggle->GetOutputPort());
  v->SetOutputTypeToVoronoi();
  v->SetOutputTypeToBoundary();
  v->ValidateOn();
  v->Update();

  std::cout << "Output Voronoi:\n";
  std::cout << "\tNumber of points:" << v->GetOutput()->GetNumberOfPoints() << endl;
  std::cout << "\tNumber of cells:" << v->GetOutput()->GetNumberOfCells() << endl;
  std::cout << "\tNumber of prunes:" << v->GetNumberOfPrunes() << endl;

  vtkNew<vtkDataSetMapper> vorMapper;
  vorMapper->SetInputConnection(v->GetOutputPort());
  vorMapper->ScalarVisibilityOff();

  vtkNew<vtkActor> vorActor;
  vorActor->SetMapper(vorMapper);
  vorActor->GetProperty()->SetColor(0.8, 0.8, 0.9);
  vorActor->GetProperty()->EdgeVisibilityOn();

  // Delaunay-based mesh
  vtkNew<vtkVoronoiFlower3D> d;
  d->SetInputData(mesh);
  d->SetInputConnection(joggle->GetOutputPort());
  d->SetOutputTypeToDelaunay();
  d->ValidateOn();
  d->Update();

  std::cout << "Output Delaunay:\n";
  std::cout << "\tNumber of points:" << d->GetOutput()->GetNumberOfPoints() << endl;
  std::cout << "\tNumber of cells:" << d->GetOutput()->GetNumberOfCells() << endl;
  std::cout << "\tNumber of prunes:" << d->GetNumberOfPrunes() << endl;

  vtkNew<vtkDataSetMapper> delMapper;
  delMapper->SetInputConnection(d->GetOutputPort());
  delMapper->ScalarVisibilityOff();

  vtkNew<vtkActor> delActor;
  delActor->SetMapper(delMapper);
  delActor->GetProperty()->SetColor(0.8, 0.8, 0.9);
  delActor->GetProperty()->EdgeVisibilityOn();

  // Create the RenderWindow, Renderer and both Actors
  vtkNew<vtkRenderer> ren1;
  ren1->SetViewport(0, 0, 0.5, 1);
  vtkNew<vtkRenderer> ren2;
  ren2->SetViewport(0.5, 0, 1, 1);
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);
  renWin->AddRenderer(ren2);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  ren1->AddActor(vorActor);
  ren1->SetBackground(1, 1, 1);
  ren2->AddActor(delActor);
  ren2->SetBackground(1, 1, 1);
  renWin->SetSize(500, 250);
  renWin->Render();
  vtkCamera* cam1 = ren1->GetActiveCamera();
  ren2->SetActiveCamera(cam1);

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
