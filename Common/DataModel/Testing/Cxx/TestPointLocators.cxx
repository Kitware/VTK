/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointLocators.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIdList.h"
#include "vtkKdTree.h"
#include "vtkKdTreePointLocator.h"
#include "vtkMath.h"
#include "vtkOctreePointLocator.h"
#include "vtkPointLocator.h"
#include "vtkPoints.h"
#include "vtkStructuredGrid.h"

// returns true if 2 points are equidistant from x, within a tolerance
bool ArePointsEquidistant(double x[3], vtkIdType id1, vtkIdType id2,
                          vtkPointSet* grid)
{
  if(id1 == id2)
    {
    return true;
    }
  float firstDist2 = vtkMath::Distance2BetweenPoints(
    x, grid->GetPoint(id1));
  float secondDist2 = vtkMath::Distance2BetweenPoints(
    x, grid->GetPoint(id2));

  float differenceDist2 = firstDist2 - secondDist2;
  if(differenceDist2 < 0)
    {
    differenceDist2 = -differenceDist2;
    }

  if(differenceDist2/(firstDist2+secondDist2) >
     .00001)
    {
    cerr << "Results do not match (first dist2="
         << firstDist2 << " , second dist2=" << secondDist2 << ") ";
    return false;
    }
  return true;
}

// checks that every point in firstList has a matching point (based
// on distance) in secondList
bool DoesListHaveProperPoints(double x[3], vtkIdList* firstList,
                              vtkIdList* secondList,
                              vtkPointSet* grid)
{
  for(vtkIdType uid=0;uid<firstList->GetNumberOfIds();uid++)
    {
    int found = 0;
    for(vtkIdType kid=0;kid<secondList->GetNumberOfIds();kid++)
      {
      if(firstList->GetId(uid) == secondList->GetId(kid))
        {
        found = 1;
        break;
        }
      }
    if(!found)
      {
      for(vtkIdType kid=0;kid<secondList->GetNumberOfIds();kid++)
        {
        if(ArePointsEquidistant(x, firstList->GetId(uid),
                                secondList->GetId(kid), grid))
          {
          found = 1;
          break;
          }
        }
      }
    if(!found)
      {
      return 0;
      }
    }
  return 1;

}

// This test compares results for different point locators since they should
// all return the same results (within a tolerance)
int ComparePointLocators(vtkAbstractPointLocator* locator1, vtkAbstractPointLocator* locator2)
{
  int rval = 0;
  int i, j, k, kOffset, jOffset, offset;
  float x[3];
  static int dims[3]={39,31,31};

  // Create the structured grid.
  vtkStructuredGrid *sgrid = vtkStructuredGrid::New();
    sgrid->SetDimensions(dims);

  // We also create the points.
  vtkPoints *points = vtkPoints::New();
    points->Allocate(dims[0]*dims[1]*dims[2]);

  for ( k=0; k<dims[2]; k++)
    {
    x[2] = 1.0 + k*1.2;
    kOffset = k * dims[0] * dims[1];
    for (j=0; j<dims[1]; j++)
      {
      x[1] = sqrt(10.+j*2.);
      jOffset = j * dims[0];
      for (i=0; i<dims[0]; i++)
        {
        x[0] = 1+i*i*.5;
        offset = i + jOffset + kOffset;
        points->InsertPoint(offset,x);
        }
      }
    }
  sgrid->SetPoints(points);
  points->Delete();

  locator1->SetDataSet(sgrid);
  locator2->SetDataSet(sgrid);

  double bounds[6];
  sgrid->GetBounds(bounds);
  for(i=0;i<3;i++)
    {
    //expand the search so we are looking for points inside and outside the BB
    bounds[i*2] *= .5;
    bounds[i*2+1] *= 1.2;
    }
  int numSearchPoints = 20;
  vtkIdList* locator1List = vtkIdList::New();
  vtkIdList* locator2List = vtkIdList::New();
  for(i=0;i<numSearchPoints;i++)
    {
    double point[3] = {(bounds[0]+(bounds[1]-bounds[0])*i/numSearchPoints),
                       (bounds[2]+(bounds[3]-bounds[2])*i/numSearchPoints),
                       (bounds[4]+(bounds[5]-bounds[4])*i/numSearchPoints)};
    vtkIdType locator1Pt = locator1->FindClosestPoint(point);
    vtkIdType locator2Pt = locator2->FindClosestPoint(point);
    if(!ArePointsEquidistant(point, locator1Pt, locator2Pt, sgrid))
      {
      cerr << " from FindClosestPoint.\n";
      rval++;
      }
    int N = 1+i*250/numSearchPoints; // test different amounts of points to search for
    locator1->FindClosestNPoints(N, point, locator1List);
    locator2->FindClosestNPoints(N, point, locator2List);
    if(!ArePointsEquidistant(point, locator1Pt, locator1List->GetId(0), sgrid))
      {
      cerr << "for comparing FindClosestPoint and first result of FindClosestNPoints for locator1.\n";
      rval++;
      }
    if(!ArePointsEquidistant(point, locator2Pt, locator2List->GetId(0), sgrid))
      {
      cerr << "for comparing FindClosestPoint and first result of FindClosestNPoints for locator2.\n";
      rval++;
      }

    for(j=0;j<N;j++)
      {
      if(!ArePointsEquidistant(point, locator2List->GetId(j), locator1List->GetId(j), sgrid))
        {
        cerr << "for point " << j << " for ClosestNPoints search.\n";
        rval++;
        }
      }
    double radius = 10;
    locator1->FindPointsWithinRadius(radius, point, locator1List);
    locator2->FindPointsWithinRadius(radius, point, locator2List);
    if(!DoesListHaveProperPoints(point, locator1List, locator2List, sgrid) ||
       !DoesListHaveProperPoints(point, locator2List, locator1List, sgrid))
      {
      cerr << "Problem with FindPointsWithinRadius\n";
      rval++;
      }

    double dist2;
    locator1Pt = locator1->FindClosestPointWithinRadius(radius, point, dist2);
    locator2Pt = locator2->FindClosestPointWithinRadius(radius, point, dist2);
    if(locator1Pt < 0 || locator2Pt < 0)
      {
      if(locator1Pt >=0 || locator2Pt >= 0)
        {
        cerr << "Inconsistent results for FindClosestPointWithinRadius\n";
        rval++;
        }
      }
    else if(!ArePointsEquidistant(point, locator1Pt, locator2Pt, sgrid))
      {
      cerr << "Incorrect result for FindClosestPointWithinRadius.\n";
      rval++;
      }
    if(locator1Pt >= 0)
      {
      locator1List->Reset();
      locator1List->InsertNextId(locator1Pt);
      if(!DoesListHaveProperPoints(point, locator1List, locator2List, sgrid))
        {
        cerr << "Inconsistent results for FindClosestPointWithinRadius and FindPointsWithRadius\n";
        rval++;
        }
      }
    }

  locator1List->Delete();
  locator2List->Delete();

  sgrid->Delete();

  return rval; // returns 0 if all tests passes
}

// This test does a brute force test on the KdTree point locator
// to make sure that at least one of the point locators used
// above gives a correct result for FindClosestPoint().
int TestKdTreePointLocator()
{
  int rval = 0;
  vtkIdType num_points = 1000;
  vtkIdType num_test_points = 100;

  vtkIdType idA;
  vtkIdType closest_id = -1;
  vtkIdType point;
  vtkIdType test_point;

  double pointA[3];
  double pointB[3];

  vtkPoints * A = vtkPoints::New();
  A->SetDataTypeToDouble();
  A->SetNumberOfPoints( num_points );
  for ( point = 0; point < num_points; ++point )
    {
    pointA[0] = ((double) rand()) / RAND_MAX;
    pointA[1] = ((double) rand()) / RAND_MAX;
    pointA[2] = ((double) rand()) / RAND_MAX;
    A->SetPoint( point, pointA );
    }

  vtkKdTree * kd = vtkKdTree::New();
  kd->BuildLocatorFromPoints( A );

  for ( test_point = 0; test_point < num_test_points; ++test_point )
    {
    double min_dist2 = 10.0;
    pointB[0] = ((double) rand()) / RAND_MAX;
    pointB[1] = ((double) rand()) / RAND_MAX;
    pointB[2] = ((double) rand()) / RAND_MAX;
    for ( point = 0; point < num_points; ++point )
      {
      double dist2;
      double dx, dy, dz;
      A->GetPoint( point, pointA );
      dx = pointA[0] - pointB[0];
      dy = pointA[1] - pointB[1];
      dz = pointA[2] - pointB[2];
      dist2 = dx * dx + dy * dy + dz * dz;
      if ( dist2 < min_dist2 )
        {
        closest_id = point;
        min_dist2 = dist2;
        }
      }
    double ld2;
    idA = kd->FindClosestPoint( pointB, ld2 );
    float diff = static_cast<float>(ld2) - static_cast<float>(min_dist2);
    if(ld2 == 0)
      {
      ld2 = 1; // avoid divide by zero error below
      }
    if ( (idA != closest_id ) && ( diff/ld2 > .00001 ))
      {
      cerr << "KdTree found the closest point to be " << ld2
           << " away but a brute force method returned a closer distance of "
                << min_dist2 << endl;
      rval++;
      }
    }

  kd->Delete();
  A->Delete();

  return rval;
}

int TestPointLocators(int , char *[])
{
  vtkKdTreePointLocator* kdTreeLocator = vtkKdTreePointLocator::New();
  vtkPointLocator* uniformLocator = vtkPointLocator::New();

  cout << "Comparing vtkPointLocator to vtkKdTreePointLocator.\n";
  int rval = ComparePointLocators(uniformLocator, kdTreeLocator);

  vtkOctreePointLocator* octreeLocator = vtkOctreePointLocator::New();

  cout << "Comparing vtkOctreePointLocator to vtkKdTreePointLocator.\n";
  rval += ComparePointLocators(octreeLocator, kdTreeLocator);

  kdTreeLocator->Delete();
  uniformLocator->Delete();
  octreeLocator->Delete();

  rval += TestKdTreePointLocator();

  return rval;
}
