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
#include "vtkCellData.h"
#include "vtkIdList.h"
#include "vtkPoints.h"
#include "vtkShrinkFilter.h"
#include "vtkDataArray.h"
#include "vtkPointLocator.h"

#include "vtkTestUtilities.h"
#include "vtkRegressionTestImage.h"

#define compare_doublevec(x, y, e) \
(((x[0]-y[0])<e) && ((x[0]-y[0])>-e) && \
((x[1]-y[1])<e) && ((x[1]-y[1])>-e) && \
((x[2]-y[2])<e) && ((x[2]-y[2])>-e))

#define compare_double(x, y, e) ((x)-(y)<e && (x)-(y)>-e)

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
  
  // test EvaluatePosition and interpolation function
  double weights[8], closestPoint[3], dist2;

  for (int i = 0; i < 8; i++)
    {
    double v;
    poly->GetPointData()->GetScalars()->GetTuple(i, &v);
    std::cout << v << " ";
    }
  std::cout << std::endl;
  
  // case 0: point on the polyhedron
  x[0] = 5.0; x[1] = 0.0; x[2] = 0.0;
  polyhedron->EvaluatePosition(x, closestPoint, subId, pc, dist2, weights);
  
  std::cout << "weights for point [" 
            << x[0] << ", " << x[1] << ", " << x[2] << "]:" << std::endl;
  for (int i = 0; i < 8; i++)
    {
    std::cout << weights[i] << " ";
    }
  std::cout << std::endl;

  double refWeights[8] = {0.0, 0.0, 0.0, 0.0, 0.25, 0.25, 0.25, 0.25};
  for (int i = 0; i < 8; i++)
    {
    if (!compare_double(refWeights[i], weights[i], 0.00001))
      {
      std::cout << "Error computing the weights for a point on the polyhedron."
              << std::endl;
      return EXIT_FAILURE;
      }
    }
  
  double refClosestPoint[3] = {5.0, 0.0, 0.0};
  if (!compare_doublevec(closestPoint, refClosestPoint, 0.00001))
    {
    std::cout << "Error finding the closet point of a point on the polyhedron." 
              << std::endl;
    return EXIT_FAILURE;
    }
  
  double refDist2 = 0.0;
  if (!compare_double(dist2, refDist2, 0.000001))
    {
    std::cout << "Error computing the distance for a point on the polyhedron."
              << std::endl;
    return EXIT_FAILURE;
    }

  // case 1: point inside the polyhedron
  x[0] = 0.0; x[1] = 0.0; x[2] = 0.0;
  polyhedron->EvaluatePosition(x, closestPoint, subId, pc, dist2, weights);
  
  std::cout << "weights for point [" 
            << x[0] << ", " << x[1] << ", " << x[2] << "]:" << std::endl;
  for (int i = 0; i < 8; i++)
    {
    std::cout << weights[i] << " ";
    }
  std::cout << std::endl;

  double refWeights1[8] = {0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125, 0.125};
  for (int i = 0; i < 8; i++)
    {
    if (!compare_double(refWeights1[i], weights[i], 0.00001))
      {
      std::cout << "Error computing the weights for a point inside the polyhedron."
              << std::endl;
      return EXIT_FAILURE;
      }
    }
  
  if (!compare_double(dist2, refDist2, 0.000001))
    {
    std::cout << "Error computing the distance for a point inside the polyhedron."
              << std::endl;
    return EXIT_FAILURE;
    }

  // case 2: point outside the polyhedron
  x[0] = 8.0; x[1] = 0.0; x[2] = 0.0;
  polyhedron->EvaluatePosition(x, closestPoint, subId, pc, dist2, weights);
  
  std::cout << "weights for point [" 
            << x[0] << ", " << x[1] << ", " << x[2] << "]:" << std::endl;
  for (int i = 0; i < 8; i++)
    {
    std::cout << weights[i] << " ";
    }
  std::cout << std::endl;

  double refWeights2[8] = {0.0307, 0.0307, 0.0307, 0.0307, 
                           0.2193, 0.2193, 0.2193, 0.2193};
  for (int i = 0; i < 8; i++)
    {
    if (!compare_double(refWeights2[i], weights[i], 0.0001))
      {
      std::cout << "Error computing the weights for a point outside the polyhedron."
              << std::endl;
      return EXIT_FAILURE;
      }
    }

  if (!compare_doublevec(closestPoint, refClosestPoint, 0.00001))
    {
    std::cout << "Error finding the closet point of a point outside the polyhedron." 
              << std::endl;
    return EXIT_FAILURE;
    }
  
  refDist2 = 9.0;
  if (!compare_double(dist2, refDist2, 0.000001))
    {
    std::cout << "Error computing the distance for a point outside the polyhedron."
              << std::endl;
    return EXIT_FAILURE;
    }

  // test evaluation location
  double weights1[8];
  polyhedron->EvaluateLocation(subId, pc, x, weights1);

  double refPoint[3] = {8.0, 0.0, 0.0};
  if (!compare_doublevec(refPoint, x, 0.00001))
    {
    std::cout << "Error evaluate the point location for its parameter coordinate."
              << std::endl;
    return EXIT_FAILURE;
    }

  for (int i = 0; i < 8; i++)
    {
    if (!compare_double(refWeights2[i], weights1[i], 0.0001))
      {
      std::cout << "Error computing the weights based on parameter coordinates."
              << std::endl;
      return EXIT_FAILURE;
      }
    }
  
  // test derivative
  pc[0] = 0;  pc[1] = 0.5;  pc[2] = 0.5;
  polyhedron->EvaluateLocation(subId, pc, x, weights1);

  double deriv[3], values[8];
  vtkDataArray * dataArray = poly->GetPointData()->GetScalars();
  for (int i = 0; i < 8; i++)
    {
    dataArray->GetTuple(i, values+i);
    }
  polyhedron->Derivatives(subId, pc, values, 1, deriv);
  
  std::cout << "derivative for point [" 
            << x[0] << ", " << x[1] << ", " << x[2] << "]:" << std::endl;
  for (int i = 0; i < 3; i++)
    {
    std::cout << deriv[i] << " ";
    }
  std::cout << std::endl;
  
  double refDeriv[3] = {0.0, 0.0, 0.05};
  if (!compare_doublevec(refDeriv, deriv, 0.00001))
    {
    std::cout << "Error computing derivative for a point inside the polyhedron."
              << std::endl;
    return EXIT_FAILURE;
    }
  
  
  // test triangulation  
  vtkSmartPointer<vtkPoints> tetraPoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkIdList> tetraIdList = vtkSmartPointer<vtkIdList>::New();
  polyhedron->Triangulate(0, tetraIdList, tetraPoints);
  
  std::cout << std::endl << "Triangulation result:" << std::endl;
  
  for (int i = 0; i < tetraPoints->GetNumberOfPoints(); i++)
    {
    double *pt = tetraPoints->GetPoint(i);
    std::cout << "point #" << i << ": [" << pt[0] << ", " 
      << pt[1] << ", " << pt[2] << "]" << std::endl;
    }
  
  vtkIdType * ids = tetraIdList->GetPointer(0);
  for (int i = 0; i < tetraIdList->GetNumberOfIds(); i+=4)
    {
    std::cout << "tetra #" << i/4 << ":" << ids[i] << " " 
      << ids[i+1] << " " << ids[i+2] << " " << ids[i+3] << std::endl; 
    }
  
  vtkSmartPointer<vtkUnstructuredGrid> tetraGrid = 
    vtkSmartPointer<vtkUnstructuredGrid>::New();
  for (int i = 0; i < tetraIdList->GetNumberOfIds(); i+=4)
    {
    tetraGrid->InsertNextCell(VTK_TETRA, 4, ids + i);
    }
  tetraGrid->SetPoints(poly->GetPoints());
  tetraGrid->GetPointData()->DeepCopy(poly->GetPointData());

  // test contour
  vtkSmartPointer<vtkPointLocator> locator = 
    vtkSmartPointer<vtkPointLocator>::New();
  vtkSmartPointer<vtkCellArray> resultPolys = 
    vtkSmartPointer<vtkCellArray>::New();
  vtkSmartPointer<vtkPointData> resultPd = 
    vtkSmartPointer<vtkPointData>::New();
  vtkSmartPointer<vtkCellData> resultCd = 
    vtkSmartPointer<vtkCellData>::New();
  
  polyhedron->Contour(0.5, tetraGrid->GetPointData()->GetScalars(), locator, 
                      NULL, NULL, resultPolys, 
                      tetraGrid->GetPointData(), resultPd,
                      tetraGrid->GetCellData(), 0, resultCd);
  
  // output the contour
  vtkSmartPointer<vtkPolyData> contour = vtkSmartPointer<vtkPolyData>::New();
  contour->SetPoints(locator->GetPoints());
  contour->SetPolys(resultPolys);
  contour->GetPointData()->DeepCopy(resultPd);;
  
  // shrink to show the gaps between tetrahedrons.
  vtkSmartPointer<vtkShrinkFilter> shrink = 
    vtkSmartPointer<vtkShrinkFilter>::New();
  shrink->SetInput( tetraGrid );
  shrink->SetShrinkFactor( 0.7 );

  // create actors
  vtkSmartPointer<vtkDataSetMapper> mapper = 
    vtkSmartPointer<vtkDataSetMapper>::New();
  mapper->SetInput(shrink->GetOutput());

  vtkSmartPointer<vtkActor> actor = 
    vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);

  vtkSmartPointer<vtkDataSetMapper> contourMapper = 
    vtkSmartPointer<vtkDataSetMapper>::New();
  contourMapper->SetInput(contour);

  vtkSmartPointer<vtkActor> contourActor = 
    vtkSmartPointer<vtkActor>::New();
  contourActor->SetMapper(contourMapper);

  // Create rendering infrastructure
  vtkSmartPointer<vtkProperty> lightProp = vtkSmartPointer<vtkProperty>::New();
  lightProp->LightingOff();
  actor->SetProperty(lightProp);
  contourActor->SetProperty(lightProp);

  vtkSmartPointer<vtkRenderer> ren = 
    vtkSmartPointer<vtkRenderer>::New();
  ren->AddActor(actor);
  ren->AddActor(contourActor);
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
