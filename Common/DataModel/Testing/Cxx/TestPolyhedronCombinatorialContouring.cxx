/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyhedron5.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkClipDataSet.h"
#include "vtkContourFilter.h"
#include "vtkDoubleArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkXMLUnstructuredGridReader.h"

using namespace std;

/* This is the layout of a cube with points on each edge
   In the test below we're going to test all combinations of
   edge points being present. As there are 12 edge points, the
   number of combinations is 2^12, i.e. 4096 cases. This can be
   calculated in ~15 seconds

Point indices:                       Face indices:

       7--14--6                           *------*
       |      |                           |      |
       19    18                           |   2  |
       |      |                           |      |
7--19--3--10--2--18--6             *------*------*------*
|      |      |      |             |      |      |      |
15    11      9     13             |   3  |   4  |   1  |
|      |      |      |             |      |      |      |
4--16--0---8--1--17--5             *------*------*------*
       |      |                           |      |
       16    17                           |   0  |
       |      |                           |      |
       4--12--5                           *------*
       |      |                           |      |
       15    13                           |   5  |
       |      |                           |      |
       7--14--6                           *------*

*/

#define CORNERS 8
#define EDGES 12
#define FACES 6
#define NPOINTS (CORNERS + EDGES + FACES)

const int Faces[FACES][8] = {
  { 0, 8, 1, 17, 5, 12, 4, 16 },
  { 1, 9, 2, 18, 6, 13, 5, 17 },
  { 2, 10, 3, 19, 7, 14, 6, 18 },
  { 3, 11, 0, 16, 4, 15, 7, 19 },
  { 0, 8, 1, 9, 2, 10, 3, 11 },
  { 4, 12, 5, 13, 6, 14, 7, 15 },
};

const double Points[CORNERS + EDGES + FACES][3] = {
  // first the corner points
  // lower plane
  { 0, 0, 0 },
  { 0, 2, 0 },
  { 2, 2, 0 },
  { 2, 0, 0 },

  // upper plane
  { 0, 0, 2 },
  { 0, 2, 2 },
  { 2, 2, 2 },
  { 2, 0, 2 },

  // then the edge points
  // lower plane
  { 0, 1, 0 },
  { 1, 2, 0 },
  { 2, 1, 0 },
  { 1, 0, 0 },

  // upper plane
  { 0, 1, 2 },
  { 1, 2, 2 },
  { 2, 1, 2 },
  { 1, 0, 2 },

  // intermediate plane
  // make the polyhedron concave by offsetting
  // these points towards the cube center
  { 0.25, 0.25, 1 },
  { 0.25, 1.75, 1 },
  { 1.75, 1.75, 1 },
  { 1.75, 0.25, 1 },

  // face centers (not used for now)
  { 1, 1, 0 },
  { 0, 1, 1 },
  { 1, 2, 1 },
  { 2, 1, 1 },
  { 1, 0, 1 },
  { 1, 1, 2 },
};

void BuildCaseGrid(int aCase, vtkUnstructuredGrid* grid, vtkIdList* faceStream)
{
  faceStream->Reset();

  for (int i = 0; i < FACES; ++i)
  {
    const int* face = Faces[i];
    int nFacePoints = 0;

    for (int j = 0; j < 8; ++j)
    {
      int idx = 1 << face[j];
      if ((aCase & idx) != 0)
      {
        ++nFacePoints;
      }
    }
    faceStream->InsertNextId(nFacePoints);

    for (int j = 0; j < 8; ++j)
    {
      int idx = 1 << face[j];
      if ((aCase & idx) != 0)
      {
        faceStream->InsertNextId(face[j]);
      }
    }
  }

  grid->InsertNextCell(VTK_POLYHEDRON, FACES, faceStream->GetPointer(0));
}

void BuildPoints(vtkPoints* pts)
{
  for (int i = 0; i < NPOINTS; ++i)
  {
    const double* pt = Points[i];
    pts->InsertNextPoint(pt);
  }
}

int TestPolyhedronCombinatorialContouring(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkPoints> pts;
  BuildPoints(pts);

  vtkNew<vtkUnstructuredGrid> g;
  g->Allocate(1);
  vtkNew<vtkIdList> ptIds;

  vtkNew<vtkXMLPolyDataWriter> pw;

  vtkNew<vtkDoubleArray> data;
  data->SetName("AirVolumeFraction");
  data->Allocate(NPOINTS);
  data->SetNumberOfTuples(NPOINTS);

  // assign 0 to even points and 1 to odd points, then contour at 0.5
  // TODO: also vary the data systematically?
  // (although that would give rise to sum(i=1..20, binomial(20,i)*2^(6+i)) = O(10^7) cases
  // and 4,096 cases run in about 15 seconds)
  // FOR NOW: don't vary the data
  for (int i = 0; i < NPOINTS; ++i)
  {
    data->SetTuple1(i, i % 2);
  }

  g->GetPointData()->AddArray(data);

  // there are 8 constant points (the corner points)
  int corners = (1 << CORNERS) - 1;

  // there are 12 variable points (the edge points)
  int nCases = 1 << EDGES;
  for (int i = 1; i < nCases; ++i)
  {
    int aCase = corners + (i << CORNERS);
    g->Reset();
    g->SetPoints(pts);

    BuildCaseGrid(aCase, g, ptIds);

    vtkNew<vtkContourFilter> cf;
    cf->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    cf->SetValue(0, 0.5);
    cf->SetInputData(g);
    cf->Update();

    vtkPolyData* result = cf->GetOutput();
    if (!result || result->GetNumberOfCells() < 1)
    {
      cerr << "Case " << aCase << " has no contour" << endl;
      return EXIT_FAILURE;
    }

    vtkNew<vtkClipDataSet> clipLess, clipMore;
    clipLess->SetInsideOut(0);
    clipMore->SetInsideOut(1);

    clipLess->SetInputData(g);
    clipMore->SetInputData(g);

    clipLess->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    clipLess->SetValue(0.5);

    clipMore->SetInputArrayToProcess(
      0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "AirVolumeFraction");
    clipMore->SetValue(0.5);

    clipLess->Update();
    vtkUnstructuredGrid* less = clipLess->GetOutput();
    if (!less || less->GetNumberOfCells() < 1)
    {
      cerr << "Case " << aCase << " has no 'less' clip result" << endl;
      return EXIT_FAILURE;
    }

    clipMore->Update();
    vtkUnstructuredGrid* more = clipMore->GetOutput();
    if (!more || more->GetNumberOfCells() < 1)
    {
      cerr << "Case " << aCase << " has no 'more' clip result" << endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
