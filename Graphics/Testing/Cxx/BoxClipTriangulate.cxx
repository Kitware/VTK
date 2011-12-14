/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoxClipTriangulate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

// This test makes sure that vtkBoxClipDataSet correctly triangulates all cell
// types.

#include "vtkBoxClipDataSet.h"
#include "vtkCellArray.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTriangle.h"
#include "vtkUnstructuredGrid.h"

#include "vtkExtractEdges.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, var) \
  vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

#include <time.h>

#include <vector>

const int NumPoints = 13;
const double PointData[NumPoints*3] = {
  0.0, 0.0, 0.0,
  0.0, 1.0, 0.0,
  1.0, 0.0, 0.0,
  1.0, 1.0, 0.0,
  2.0, 0.0, 0.0,
  2.0, 1.0, 0.0,

  0.0, 0.0, 1.0,
  0.0, 1.0, 1.0,
  1.0, 0.0, 1.0,
  1.0, 1.0, 1.0,
  2.0, 0.0, 1.0,
  2.0, 1.0, 1.0,
  2.0, 0.5, 1.0
};


const vtkIdType NumTriStripCells = 1;
const vtkIdType TriStripCells[] = {
  6, 1, 0, 3, 2, 5, 4
};

const vtkIdType NumQuadCells = 2;
const vtkIdType QuadCells[] = {
  4, 0, 2, 3, 1,
  4, 2, 4, 5, 3
};

const vtkIdType NumPixelCells = 2;
const vtkIdType PixelCells[] = {
  4, 0, 2, 1, 3,
  4, 2, 4, 3, 5
};

const vtkIdType NumPolyCells = 3;
const vtkIdType PolyCells[] = {
  4, 0, 2, 3, 1,
  3, 2, 4, 5,
  5, 6, 8, 12, 9, 7
};


const vtkIdType NumHexCells = 2;
const vtkIdType HexCells[] = {
  8, 6, 8, 2, 0, 7, 9, 3, 1,
  8, 4, 2, 8, 10, 5, 3, 9, 11
};
const vtkIdType NumExpectedHexSurfacePolys = 20;

const vtkIdType NumVoxelCells = 2;
const vtkIdType VoxelCells[] = {
  8, 0, 2, 1, 3, 6, 8, 7, 9,
  8, 10, 8, 11, 9, 4, 2, 5, 3
};
const vtkIdType NumExpectedVoxelSurfacePolys = 20;

const vtkIdType NumWedgeCells = 4;
const vtkIdType WedgeCells[] = {
  6, 0, 1, 2, 6, 7, 8,
  6, 7, 8, 9, 1, 2, 3,
  6, 8, 11, 9, 2, 5, 3,
  6, 2, 5, 4, 8, 11, 10
};
const vtkIdType NumExpectedWedgeSurfacePolys = 20;

const vtkIdType NumPyramidCells = 2;
const vtkIdType PyramidCells[] = {
  5, 8, 9, 3, 2, 0,
  5, 2, 3, 9, 8, 12
};
const vtkIdType NumExpectedPyramidSurfacePolys = 8;

class BoxClipTriangulateFailed { };

//-----------------------------------------------------------------------------

static void CheckWinding(vtkUnstructuredGrid *data)
{
  data->Update();

  vtkPoints *points = data->GetPoints();

  vtkCellArray *cells = data->GetCells();
  cells->InitTraversal();

  vtkIdType npts, *pts;
  while (cells->GetNextCell(npts, pts))
    {
    if (npts != 4)
      {
      cout << "Weird.  I got something that is not a tetrahedra." << endl;
      continue;
      }

    double p0[3], p1[3], p2[3], p3[3];
    points->GetPoint(pts[0], p0);
    points->GetPoint(pts[1], p1);
    points->GetPoint(pts[2], p2);
    points->GetPoint(pts[3], p3);

    // If the winding is correct, the normal to triangle p0,p1,p2 should point
    // towards p3.
    double v0[3], v1[3];
    v0[0] = p1[0] - p0[0];    v0[1] = p1[1] - p0[1];    v0[2] = p1[2] - p0[2];
    v1[0] = p2[0] - p0[0];    v1[1] = p2[1] - p0[1];    v1[2] = p2[2] - p0[2];

    double n[3];
    vtkMath::Cross(v0, v1, n);

    double d[3];
    d[0] = p3[0] - p0[0];    d[1] = p3[1] - p0[1];    d[2] = p3[2] - p0[2];

    if (vtkMath::Dot(n, d) < 0)
      {
      cout << "Found a tetrahedra with bad winding." << endl;
      throw BoxClipTriangulateFailed();
      }
    }
}

//-----------------------------------------------------------------------------

static vtkSmartPointer<vtkUnstructuredGrid> BuildInput(int type,
                                                       vtkIdType numcells,
                                                       const vtkIdType *cells)
{
  vtkIdType i;

  VTK_CREATE(vtkUnstructuredGrid, input);

  // Randomly shuffle the points to possibly test various tessellations.
  // Make a map from original point orderings to new point orderings.
  std::vector<vtkIdType> idMap;
  std::vector<vtkIdType> idsLeft;

  for (i = 0; i < NumPoints; i++)
    {
    idsLeft.push_back(i);
    }

  while (!idsLeft.empty())
    {
    vtkIdType next
      = vtkMath::Round(vtkMath::Random(-0.49, idsLeft.size() - 0.51));
    std::vector<vtkIdType>::iterator nextp = idsLeft.begin() + next;
    idMap.push_back(*nextp);
    idsLeft.erase(nextp, nextp + 1);
    }

  // Build shuffled points.
  VTK_CREATE(vtkPoints, points);
  points->SetNumberOfPoints(NumPoints);
  for (i = 0; i < NumPoints; i++)
    {
    points->SetPoint(idMap[i], PointData + 3*i);
    }
  input->SetPoints(points);

  // Add the cells with indices properly mapped.
  VTK_CREATE(vtkIdList, ptIds);
  const vtkIdType *c = cells;
  for (i = 0; i < numcells; i++)
    {
    vtkIdType npts = *c;  c++;
    ptIds->Initialize();
    for (vtkIdType j = 0; j < npts; j++)
      {
      ptIds->InsertNextId(idMap[*c]);
      c++;
      }
    input->InsertNextCell(type, ptIds);
    }

  return input;
}

//-----------------------------------------------------------------------------

static void Check2DPrimitive(int type, vtkIdType numcells,
                             const vtkIdType *cells)
{
  vtkSmartPointer<vtkUnstructuredGrid> input
    = BuildInput(type, numcells, cells);

  VTK_CREATE(vtkBoxClipDataSet, clipper);
  clipper->SetInput(input);
  // Clip nothing.
  clipper->SetBoxClip(0.0, 2.0, 0.0, 1.0, 0.0, 1.0);
  clipper->Update();

  vtkUnstructuredGrid *output = clipper->GetOutput();

  if (output->GetNumberOfCells() < 1)
    {
    cout << "Output has no cells!" << endl;
    throw BoxClipTriangulateFailed();
    }

  // Check to make sure all the normals point in the z direction.
  vtkCellArray *outCells = output->GetCells();
  outCells->InitTraversal();
  vtkIdType npts, *pts;
  while (outCells->GetNextCell(npts, pts))
    {
    if (npts != 3)
      {
      cout << "Got a primitive that is not a triangle!" << endl;
      throw BoxClipTriangulateFailed();
      }

    double n[3];
    vtkTriangle::ComputeNormal(output->GetPoints(), npts, pts, n);
    if ((n[0] > 0.1) || (n[1] > 0.1) || (n[2] < 0.9))
      {
      cout << "Primitive is facing the wrong way!" << endl;
      throw BoxClipTriangulateFailed();
      }
    }
}

//-----------------------------------------------------------------------------

static void Check3DPrimitive(int type, vtkIdType numcells,
                             const vtkIdType *cells, vtkIdType numSurfacePolys)
{
  vtkSmartPointer<vtkUnstructuredGrid> input
    = BuildInput(type, numcells, cells);

  VTK_CREATE(vtkBoxClipDataSet, clipper);
  clipper->SetInput(input);
  // Clip nothing.
  clipper->SetBoxClip(0.0, 2.0, 0.0, 1.0, 0.0, 1.0);
  clipper->Update();

  vtkUnstructuredGrid *output = clipper->GetOutput();

  if (output->GetNumberOfCells() < 1)
    {
    cout << "Output has no cells!" << endl;
    throw BoxClipTriangulateFailed();
    }

#if 0
  VTK_CREATE(vtkExtractEdges, edges);
  edges->SetInput(output);
  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(0, edges->GetOutputPort(0));
  VTK_CREATE(vtkActor, actor);
  actor->SetMapper(mapper);
  VTK_CREATE(vtkRenderer, renderer);
  renderer->AddActor(actor);
  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->AddRenderer(renderer);
  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);
  renwin->Render();
  iren->Start();
#endif

  CheckWinding(output);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface);
  surface->SetInput(output);
  surface->Update();

  if (surface->GetOutput()->GetNumberOfCells() != numSurfacePolys)
    {
    cout << "Expected " << numSurfacePolys << " triangles on the surface, got "
         << surface->GetOutput()->GetNumberOfCells() << endl;
    throw BoxClipTriangulateFailed();
    }
}

//-----------------------------------------------------------------------------

int BoxClipTriangulate(int, char *[])
{
  long seed = time(NULL);
  cout << "Random seed = " << seed << endl;
  vtkMath::RandomSeed(seed);
  vtkMath::Random();
  vtkMath::Random();
  vtkMath::Random();

  try
    {
    cout << "Checking triangle strip." << endl;
    Check2DPrimitive(VTK_TRIANGLE_STRIP, NumTriStripCells, TriStripCells);

    cout << "Checking quadrilaterals." << endl;
    Check2DPrimitive(VTK_QUAD, NumQuadCells, QuadCells);

    cout << "Checking pixels." << endl;
    Check2DPrimitive(VTK_PIXEL, NumPixelCells, PixelCells);

    cout << "Checking polygons." << endl;
    Check2DPrimitive(VTK_POLYGON, NumPolyCells, PolyCells);

    cout << "Checking hexahedrons." << endl;
    Check3DPrimitive(VTK_HEXAHEDRON, NumHexCells, HexCells,
                     NumExpectedHexSurfacePolys);

    cout << "Checking voxels." << endl;
    Check3DPrimitive(VTK_VOXEL, NumVoxelCells, VoxelCells,
                     NumExpectedVoxelSurfacePolys);

    cout << "Checking wedges." << endl;
    Check3DPrimitive(VTK_WEDGE, NumWedgeCells, WedgeCells,
                     NumExpectedWedgeSurfacePolys);

    cout << "Checking pyramids." << endl;
    Check3DPrimitive(VTK_PYRAMID, NumPyramidCells, PyramidCells,
                     NumExpectedPyramidSurfacePolys);
    }
  catch (BoxClipTriangulateFailed)
    {
    return 1;
    }

  return 0;
}
