/*=========================================================================

  Program:   Visualization Toolkit
  Module:    CellLocator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDataArray.h"
#include "vtkGenericCell.h"
#include "vtkPointData.h"

#include "vtkCellTreeLocator.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkSphereSource.h"

#include "vtkDebugLeaks.h"

int TestWithCachedCellBoundsParameter(int cachedCellBounds)
{
  // kuhnan's sample code used to test
  // vtkCellLocator::IntersectWithLine(...9 params...)

  // sphere1: the outer sphere
  vtkNew<vtkSphereSource> sphere1;
  sphere1->SetThetaResolution(100);
  sphere1->SetPhiResolution(100);
  sphere1->SetRadius(1);
  sphere1->Update();

  // sphere2: the inner sphere
  vtkNew<vtkSphereSource> sphere2;
  sphere2->SetThetaResolution(100);
  sphere2->SetPhiResolution(100);
  sphere2->SetRadius(0.8);
  sphere2->Update();

  // the normals obtained from the outer sphere
  vtkDataArray *sphereNormals = sphere1->GetOutput()->GetPointData()->GetNormals();

  // the cell locator
  vtkNew<vtkCellTreeLocator> locator;
  locator->SetDataSet(sphere2->GetOutput());
  locator->SetCacheCellBounds(cachedCellBounds);
  locator->AutomaticOn();
  locator->BuildLocator();

  // init the counter and ray length
  int numIntersected = 0;
  double rayLen = 0.2000001; // = 1 - 0.8 + error tolerance
  int sub_id;
  vtkIdType cell_id;
  double param_t, intersect[3], paraCoord[3];
  double sourcePnt[3], destinPnt[3], normalVec[3];
  vtkNew<vtkGenericCell> cell;

  // this loop traverses each point on the outer sphere (sphere1)
  // and  looks for an intersection on the inner sphere (sphere2)
  for ( int i = 0; i < sphere1->GetOutput()->GetNumberOfPoints(); i ++ )
  {
    sphere1->GetOutput()->GetPoint(i, sourcePnt);
    sphereNormals->GetTuple(i, normalVec);

    // cast a ray in the negative direction toward sphere1
    destinPnt[0] = sourcePnt[0] - rayLen * normalVec[0];
    destinPnt[1] = sourcePnt[1] - rayLen * normalVec[1];
    destinPnt[2] = sourcePnt[2] - rayLen * normalVec[2];

    if ( locator->IntersectWithLine(sourcePnt, destinPnt, 0.0010, param_t,
                                    intersect, paraCoord, sub_id, cell_id, cell.GetPointer()) )
    {
      numIntersected ++;
    }
  }

  if ( numIntersected != 9802 )
  {
    int numMissed = 9802 - numIntersected;
    vtkGenericWarningMacro("ERROR: " << numMissed << " ray-sphere intersections missed! "
                           << "If on a non-WinTel32 platform, try rayLen = 0.200001"
                           << " or 0.20001 for a new test.");
    return EXIT_FAILURE;
  }
  else
  {
    std::cout << "Passed: a total of 9802 ray-sphere intersections detected." << std::endl;
  }

  sphereNormals = NULL;

  return EXIT_SUCCESS;
}

int CellTreeLocator( int vtkNotUsed(argc), char *vtkNotUsed(argv)[] )
{
  int retVal = TestWithCachedCellBoundsParameter(0);
  retVal += TestWithCachedCellBoundsParameter(1);
  return retVal;
}
