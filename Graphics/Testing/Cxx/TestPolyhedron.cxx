/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkExtractPolyhedralMesh.h"
#include "vtkDataSetMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGridReader.h"
#include "vtkExtractEdges.h"
#include "vtkProperty.h"
#include "vtkUnstructuredGrid.h"
#include "vtkGenericCell.h"
#include "vtkPolyhedron.h"
#include "vtkCubeSource.h"
#include "vtkElevationFilter.h"
#include "vtkCellArray.h"
#include "vtkPointData.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

// Test of vtkPolyhedron. A structured grid is converted to a polyhedral
// mesh.
int TestPolyhedron( int argc, char* argv[] )
{
  // create the a cube
  vtkSmartPointer<vtkCubeSource> cube = 
    vtkSmartPointer<vtkCubeSource>::New();
  cube->SetXLength(10); 
  cube->SetYLength(10); 
  cube->SetZLength(20); 
  cube->SetCenter(0, 0, 0);  
  cube->Update();

  // add scaler
  vtkSmartPointer<vtkElevationFilter> ele = 
    vtkSmartPointer<vtkElevationFilter>::New();
  ele->SetInput(cube->GetOutput());
  ele->SetLowPoint(0,0,-10);
  ele->SetHighPoint(0,0,10);
  ele->Update();
  vtkPolyData* poly = vtkPolyData::SafeDownCast(ele->GetOutput());
  
  // create a test polyhedron
  vtkIdType pointIds[8] = {0, 1, 2, 3, 4, 5, 6, 7};

  vtkSmartPointer<vtkCellArray> faces = vtkSmartPointer<vtkCellArray>::New();
  vtkIdType face0[4] = {0, 2, 6, 4};
  vtkIdType face1[4] = {1, 3, 7, 5};
  vtkIdType face2[4] = {0, 1, 3, 2};
  vtkIdType face3[4] = {4, 5, 7, 6};
  vtkIdType face4[4] = {0, 1, 5, 4};
  vtkIdType face5[4] = {2, 3, 7, 6};
  faces->InsertNextCell(4, face0);
  faces->InsertNextCell(4, face1);
  faces->InsertNextCell(4, face2);
  faces->InsertNextCell(4, face3);
  faces->InsertNextCell(4, face4);
  faces->InsertNextCell(4, face5);
  
  vtkSmartPointer<vtkUnstructuredGrid> ugrid = 
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  ugrid->SetPoints(poly->GetPoints());
  ugrid->GetPointData()->DeepCopy(poly->GetPointData());

  ugrid->InsertNextCell(VTK_POLYHEDRON, 8, pointIds, 
    6, faces->GetPointer());

  vtkPolyhedron *polyhedron = static_cast<vtkPolyhedron*>(ugrid->GetCell(0));
  
  // Print out basic information
  std::cout << "Testing polyhedron is a cube of with bounds "
            << "[-5, 5, -5, 5, -10, 10]. It has "
            << polyhedron->GetNumberOfEdges() << " edges and " 
            << polyhedron->GetNumberOfFaces() << " faces." << std::endl;
  
  double p1[3] = {-100,0,0};
  double p2[3] = { 100,0,0};
  double tol = 0.001;
  double t, x[3], pc[3];
  int subId=0;
  
  // test intersection
  int numInts = polyhedron->IntersectWithLine(p1,p2,tol,t,x,pc,subId); //should be 2
  if (numInts != 2)
    {
    std::cerr << "Expect 2 intersections, but get " << numInts << std::endl;
    return EXIT_FAILURE;
    }
  
  // test inside
  int inside = polyhedron->IsInside(p1,tol); //should be out
  if (inside)
    {
    std::cerr << "Expect point [" << p1[0] << ", " << p1[1] << ", " << p1[2] 
              << "] to be outside the polyhedral, but it's inside." << std::endl;
    return EXIT_FAILURE;
    }
  
  p2[0] = 0.0; p2[1] = 0.0; p2[2] = 0.0;
  inside = polyhedron->IsInside(p2,tol); //should be in
  if (!inside)
    {
    std::cerr << "Expect point [" << p2[0] << ", " << p2[1] << ", " << p2[2] 
              << "] to be inside the polyhedral, but it's outside." << std::endl;
    return EXIT_FAILURE;
    }
  
  // test EvaluatePosition and 
  x[0] = 5.0; x[1] = 0.0; x[2] = 0.0;
  double pcoords[3], weights[8], closestPoint[3], dist2;
  polyhedron->EvaluatePosition(x, closestPoint, subId, pc, dist2, weights);
  
  for (int i = 0; i < 8; i++)
    {
    std::cout << weights[i] << " ";
    }
  std::cout << std::endl;
  
  // void EvaluateLocation(int& subId, double pcoords[3], double x[3],
  //                      double *weights);
  
  // create actors
  vtkSmartPointer<vtkDataSetMapper> mapper = 
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInput(ugrid);

  vtkSmartPointer<vtkActor> actor = 
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  // Okay let's extract some edges
  vtkSmartPointer<vtkExtractEdges> edges =
    vtkSmartPointer<vtkExtractEdges>::New();
  edges->SetInput(ugrid);
  
  vtkSmartPointer<vtkDataSetMapper> eMapper = 
    vtkSmartPointer<vtkDataSetMapper>::New();
  eMapper->SetInputConnection(edges->GetOutputPort());

  vtkSmartPointer<vtkActor> eActor = 
    vtkSmartPointer<vtkActor>::New();
  eActor->SetMapper(eMapper);
  eActor->GetProperty()->SetColor(0,0,0);

  // Create rendering infrastructure
  vtkSmartPointer<vtkProperty> lightProp = vtkSmartPointer<vtkProperty>::New();
  lightProp->LightingOff();
  actor->SetProperty(lightProp);
  eActor->SetProperty(lightProp);  

  vtkSmartPointer<vtkRenderer> ren = 
    vtkSmartPointer<vtkRenderer>::New();
  ren->AddActor(actor);
  ren->AddActor(eActor);
  ren->SetBackground(.5,.5,.5);

  vtkSmartPointer<vtkRenderWindow> renWin = 
    vtkSmartPointer<vtkRenderWindow>::New();
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
