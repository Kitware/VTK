/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyhedron1.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataSetMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkExtractEdges.h"
#include "vtkProperty.h"
#include "vtkUnstructuredGrid.h"
#include "vtkPolyhedron.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPoints.h"
#include "vtkDataArray.h"
#include "vtkPointLocator.h"
#include "vtkPlaneSource.h"
#include "vtkPlane.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

// Test of vtkPolyhedron. A dodecahedron is created for testing clip and contour
int TestPolyhedron1( int argc, char* argv[] )
{
  // create a dodecahedron
  double dodechedronPoint[20][3] = { {1.21412,    0,          1.58931},
			                               {0.375185,   1.1547,     1.58931},
			                               {-0.982247,  0.713644,   1.58931},
			                               {-0.982247,  -0.713644,  1.58931},
			                               {0.375185,   -1.1547,    1.58931},
			                               {1.96449,    0,          0.375185},
			                               {0.607062,   1.86835,    0.375185},
			                               {-1.58931,   1.1547,     0.375185},
			                               {-1.58931,   -1.1547,    0.375185},
			                               {0.607062,   -1.86835,   0.375185},
			                               {1.58931,    1.1547,     -0.375185},
			                               {-0.607062,  1.86835,    -0.375185},
			                               {-1.96449,   0,          -0.375185},
			                               {-0.607062,  -1.86835,   -0.375185},
			                               {1.58931,    -1.1547,    -0.375185},
			                               {0.982247,   0.713644,   -1.58931},
			                               {-0.375185,  1.1547,     -1.58931},
			                               {-1.21412,   0,          -1.58931},
			                               {-0.375185,  -1.1547,    -1.58931},
			                               {0.982247,   -0.713644,  -1.58931}};
  vtkSmartPointer<vtkPoints> dodechedronPoints = vtkSmartPointer<vtkPoints>::New();
  dodechedronPoints->Initialize();
  for (int i = 0; i < 20; i++)
    {
    dodechedronPoints->InsertNextPoint(dodechedronPoint[i]);
    }

  vtkIdType dodechedronPointsIds[20] = {0,   1,  2,  3,  4,  5,  6,  7,  8,  9,
                                        10, 11, 12, 13, 14, 15, 16, 17, 18, 19};

  vtkIdType dodechedronFace[12][5] = {{0, 1, 2, 3, 4},
                                      {0, 5, 10, 6, 1},
                                      {1, 6, 11, 7, 2},
                                      {2, 7, 12, 8, 3},
                                      {3, 8, 13, 9, 4},
                                      {4, 9, 14, 5, 0},
                                      {15, 10, 5, 14, 19},
                                      {16, 11, 6, 10, 15},
                                      {17, 12, 7, 11, 16},
                                      {18, 13, 8, 12, 17},
                                      {19, 14, 9, 13, 18},
                                      {19, 18, 17, 16, 15}};

  vtkSmartPointer<vtkCellArray> dodechedronFaces = vtkSmartPointer<vtkCellArray>::New();
  for (int i = 0; i < 12; i++)
    {
    dodechedronFaces->InsertNextCell(5, dodechedronFace[i]);
    }

  double offset = 0;//0.375185;


  double normal[3] = {0.0, 0.0, 1.0};
  double origin[3] = {0.0, 0.0, offset};
  double x[3] = {1.0, 0.0, 0.0};
  double y[3] = {0.0, 1.0, 0.0};

  vtkSmartPointer<vtkPlaneSource> planeSource = vtkSmartPointer<vtkPlaneSource>::New();
  planeSource->SetNormal(normal);
  planeSource->SetOrigin(origin);
  planeSource->SetPoint1(origin[0] + 5*x[0], origin[1] + 5*x[1], origin[2] + 5*x[2]);
  planeSource->SetPoint2(origin[0] + 7*y[0], origin[1] + 7*y[1], origin[2] + 7*y[2]);
  planeSource->SetCenter(origin);
  planeSource->SetResolution(1, 1);
  planeSource->Update();


  vtkSmartPointer<vtkPlane> plane = vtkSmartPointer<vtkPlane>::New();
  plane->SetNormal(normal);
  plane->SetOrigin(origin);
  vtkSmartPointer<vtkDoubleArray> pointDataArray =
    vtkSmartPointer<vtkDoubleArray>::New();
  pointDataArray->Initialize();
  for (int i = 0; i < 20; i++)
    {
    cout << plane->EvaluateFunction(dodechedronPoint[i]) << endl;
    pointDataArray->InsertNextValue(plane->EvaluateFunction(dodechedronPoint[i])+0.01);
    }

  vtkSmartPointer<vtkDoubleArray> cellDataArray =
    vtkSmartPointer<vtkDoubleArray>::New();
  cellDataArray->Initialize();
  for (int i = 0; i < 12; i++)
    {
    cellDataArray->InsertNextValue(static_cast<double>(1.0));
    }

  vtkSmartPointer<vtkUnstructuredGrid> ugrid =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  ugrid->SetPoints(dodechedronPoints);
  ugrid->InsertNextCell(VTK_POLYHEDRON, 20, dodechedronPointsIds,
    12, dodechedronFaces->GetPointer());
  ugrid->GetPointData()->SetScalars(pointDataArray);
  //ugrid->GetCellData()->SetScalars(cellDataArray);

  vtkPolyhedron *polyhedron = static_cast<vtkPolyhedron*>(ugrid->GetCell(0));
  vtkPolyData * planePoly = planeSource->GetOutput();
  polyhedron->GetPolyData()->GetPointData()->SetScalars(pointDataArray);
  //polyhedron->GetPolyData()->GetCellData()->SetScalars(cellDataArray);

  // test contour
  vtkSmartPointer<vtkPointLocator> locator =
    vtkSmartPointer<vtkPointLocator>::New();
  vtkSmartPointer<vtkCellArray> resultPolys =
    vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkPointData> resultPd =
    vtkSmartPointer<vtkPointData>::New();
  vtkSmartPointer<vtkCellData> resultCd =
    vtkSmartPointer<vtkCellData>::New();
  vtkSmartPointer<vtkPoints> resultPoints =
    vtkSmartPointer<vtkPoints>::New();
  resultPoints->DeepCopy(ugrid->GetPoints());
  locator->InitPointInsertion(resultPoints, ugrid->GetBounds());

  polyhedron->Contour(0, ugrid->GetPointData()->GetScalars(), locator,
                      NULL, NULL, resultPolys,
                      ugrid->GetPointData(), resultPd,
                      ugrid->GetCellData(), 0, resultCd);

  // output the contour
  vtkSmartPointer<vtkUnstructuredGrid> contourResult =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  contourResult->SetPoints(locator->GetPoints());
  contourResult->SetCells(VTK_POLYGON, resultPolys);
  contourResult->GetPointData()->DeepCopy(resultPd);

  // test clip
  vtkSmartPointer<vtkPointLocator> locator1 =
    vtkSmartPointer<vtkPointLocator>::New();
  vtkSmartPointer<vtkCellArray> resultPolys1 =
    vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkPointData> resultPd1 =
    vtkSmartPointer<vtkPointData>::New();
  vtkSmartPointer<vtkCellData> resultCd1 =
    vtkSmartPointer<vtkCellData>::New();
  vtkSmartPointer<vtkPoints> resultPoints1 =
    vtkSmartPointer<vtkPoints>::New();
  resultPoints1->DeepCopy(ugrid->GetPoints());
  locator1->InitPointInsertion(resultPoints1, ugrid->GetBounds());

  polyhedron->Clip(0, ugrid->GetPointData()->GetScalars(), locator1,
                   resultPolys1, ugrid->GetPointData(), resultPd1,
                   ugrid->GetCellData(), 0, resultCd1, 1);

  // output the clipped polyhedron
  vtkSmartPointer<vtkUnstructuredGrid> clipResult =
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  clipResult->SetPoints(locator1->GetPoints());
  clipResult->SetCells(VTK_POLYHEDRON, resultPolys1);
  clipResult->GetPointData()->DeepCopy(resultPd1);

  // create actors
  vtkSmartPointer<vtkDataSetMapper> mapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInputData(polyhedron->GetPolyData());

  vtkSmartPointer<vtkActor> actor =
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkDataSetMapper> planeMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  planeMapper->SetInputData(planePoly);

  vtkSmartPointer<vtkActor> planeActor =
    vtkSmartPointer<vtkActor>::New();
  planeActor->SetMapper(planeMapper);


  vtkSmartPointer<vtkDataSetMapper> contourMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  contourMapper->SetInputData(contourResult);

  vtkSmartPointer<vtkActor> contourActor =
    vtkSmartPointer<vtkActor>::New();
  contourActor->SetMapper(contourMapper);

  vtkSmartPointer<vtkDataSetMapper> clipPolyhedronMapper =
    vtkSmartPointer<vtkDataSetMapper>::New();
  clipPolyhedronMapper->SetInputData(clipResult);

  vtkSmartPointer<vtkActor> clipPolyhedronActor =
    vtkSmartPointer<vtkActor>::New();
  clipPolyhedronActor->SetMapper(clipPolyhedronMapper);

  // Create rendering infrastructure
  vtkSmartPointer<vtkProperty> prop = vtkSmartPointer<vtkProperty>::New();
  prop->LightingOff();
  prop->SetRepresentationToSurface();
  prop->EdgeVisibilityOn();
  prop->SetLineWidth(3.0);
  prop->SetOpacity(1.0);
  prop->SetInterpolationToFlat();

  vtkSmartPointer<vtkProperty> prop1 = vtkSmartPointer<vtkProperty>::New();
  prop1->LightingOff();
  prop1->SetRepresentationToSurface();
  prop1->EdgeVisibilityOn();
  prop1->SetLineWidth(3.0);
  prop1->SetOpacity(0.5);
  prop1->SetInterpolationToFlat();

  // set property
  actor->SetProperty(prop1);
  planeActor->SetProperty(prop1);
  contourActor->SetProperty(prop1);
  clipPolyhedronActor->SetProperty(prop);

  vtkSmartPointer<vtkRenderer> ren =
    vtkSmartPointer<vtkRenderer>::New();
  ren->AddActor(actor);
  ren->AddActor(planeActor);
  ren->AddActor(contourActor);
  ren->AddActor(clipPolyhedronActor);
  ren->SetBackground(.5,.5,.5);

  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  iren->Initialize();

  renWin->Render();

  int retVal = vtkRegressionTestImage( renWin );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
