/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoxClipTetrahedra.cxx

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

// This test exercises several ways a plane may clip a tetrahedra.  One of the
// things tested is the "winding" of the tetrahedra.  There are two rotationally
// independent ways to specify a tetrahedra:
//
//        v3                    v3
//        /|\                   /|\                     //
//       / | \                 / | \                    //
//      /  |  \               /  |  \                   //
//   v2/_ _|_ _\v1         v1/_ _|_ _\v2                //
//     \   |   /             \   |   /                  //
//      \  |  /               \  |  /                   //
//       \ | /                 \ | /                    //
//        \|/                   \|/                     //
//         v0                    v0                     //
//
// I'm calling these rotationally independent vertex specifications windings for
// short.  VTK expects the winding on the left.  We will test to make sure the
// winding on the right does not occur.

#include "vtkActor.h"
#include "vtkBoxClipDataSet.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkMath.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkUnstructuredGrid.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, var) \
  vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

static double tetrahedraPoints[4*3] = {
  1.0, 0.0, 0.0,
  -1.0, 0.0, 0.0,
  0.0, 0.0, 1.0,
  0.0, 1.0, 0.5
};

// All possible cell connectivites with the correct winding.
static vtkIdType tetrahedra[12][4] = {
  { 0, 1, 2, 3 },
  { 2, 0, 1, 3 },
  { 1, 2, 0, 3 },

  { 0, 3, 1, 2 },
  { 1, 0, 3, 2 },
  { 3, 1, 0, 2 },

  { 0, 2, 3, 1 },
  { 3, 0, 2, 1 },
  { 2, 3, 0, 1 },

  { 1, 3, 2, 0 },
  { 2, 1, 3, 0 },
  { 3, 2, 1, 0 }
};

static double minusx[] = { -1.0, 0.0, 0.0 };
static double minusy[] = { 0.0, -1.0, 0.0 };
static double minusz[] = { 0.0, 0.0, -1.0 };
static double plusx[] = { 1.0, 0.0, 0.0 };
static double plusy[] = { 0.0, 1.0, 0.0 };
static double plusz[] = { 0.0, 0.0, 1.0 };

static int NUM_CLIP_BOXES = 8;

class BadWinding
{
public:
  BadWinding(vtkUnstructuredGrid *data) {
    this->Data = data;
    this->Data->Register(NULL);
  }
  ~BadWinding() {
    this->Data->UnRegister(NULL);
  }
  BadWinding(const BadWinding &bw) {
    this->Data = bw.Data;
    this->Data->Register(NULL);
  }
  void operator=(const BadWinding &bw) {
    this->Data->UnRegister(NULL);
    this->Data = bw.Data;
    this->Data->Register(NULL);
  }

  vtkUnstructuredGrid *Data;
};


static void CheckWinding(vtkUnstructuredGrid *data)
{
  vtkPoints *points = data->GetPoints();

  vtkCellArray *cells = data->GetCells();
  cells->InitTraversal();

  vtkIdType npts, *pts;
  while (cells->GetNextCell(npts, pts))
    {
    if (npts != 4)
      {
      std::cout << "Weird.  I got something that is not a tetrahedra." << std::endl;
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
      throw BadWinding(data);
      }
    }
}

static vtkSmartPointer<vtkUnstructuredGrid> MakeTetrahedron(int num)
{
  VTK_CREATE(vtkDoubleArray, pointArray);
  pointArray->SetArray(tetrahedraPoints, 4*3, 1);
  pointArray->SetNumberOfComponents(3);
  pointArray->SetNumberOfTuples(4);

  VTK_CREATE(vtkPoints, points);
  points->SetData(pointArray);

  VTK_CREATE(vtkUnstructuredGrid, ugrid);
  ugrid->SetPoints(points);
  ugrid->InsertNextCell(VTK_TETRA, 4, tetrahedra[num]);

  return ugrid;
}

static void PlaceRenderer(vtkRenderer *renderer, int boxnum, int tetnum,
                          int boxtype)
{
  renderer->SetViewport(tetnum/24.0 + 0.5*(boxtype%2),
                        1.0 - (  static_cast<double>(boxnum)/NUM_CLIP_BOXES
                               + static_cast<double>(boxtype/2 + 1)/(2*NUM_CLIP_BOXES) ),
                        (tetnum + 1)/24.0 + 0.5*(boxtype%2),
                        1.0 - (  static_cast<double>(boxnum)/NUM_CLIP_BOXES
                               + static_cast<double>(boxtype/2)/(2*NUM_CLIP_BOXES) ));
}

static void TestBox(vtkRenderWindow *renwin, int boxnum,
                    double minx, double maxx,
                    double miny, double maxy,
                    double minz, double maxz)
{
  int i;

  // Add tests for axis oriented box and no clipped output.
  for (i = 0; i < 12; i++)
    {
    vtkSmartPointer<vtkUnstructuredGrid> input = MakeTetrahedron(i);
    CheckWinding(input);

    VTK_CREATE(vtkBoxClipDataSet, clipper);
    clipper->SetInputData(input);
    clipper->GenerateClippedOutputOff();
    clipper->SetBoxClip(minx, maxx, miny, maxy, minz, maxz);
    clipper->Update();
    CheckWinding(clipper->GetOutput());

    VTK_CREATE(vtkDataSetSurfaceFilter, surface);
    surface->SetInputConnection(0, clipper->GetOutputPort(0));

    VTK_CREATE(vtkPolyDataMapper, mapper);
    mapper->SetInputConnection(0, surface->GetOutputPort(0));

    VTK_CREATE(vtkActor, actor);
    actor->SetMapper(mapper);

    VTK_CREATE(vtkRenderer, renderer);
    renderer->AddActor(actor);
    renderer->SetBackground(0.0, 0.5, 0.5);
    PlaceRenderer(renderer, boxnum, i, 0);
    renderer->ResetCamera();
    renwin->AddRenderer(renderer);

    vtkCamera *camera = renderer->GetActiveCamera();
    camera->Azimuth(25.0);
    camera->Elevation(-25.0);
    }

  // Add tests for axis oriented box and clipped output.
  for (i = 0; i < 12; i++)
    {
    vtkSmartPointer<vtkUnstructuredGrid> input = MakeTetrahedron(i);
    CheckWinding(input);

    VTK_CREATE(vtkBoxClipDataSet, clipper);
    clipper->SetInputData(input);
    clipper->GenerateClippedOutputOn();
    clipper->SetBoxClip(minx, maxx, miny, maxy, minz, maxz);
    clipper->Update();
    CheckWinding(clipper->GetOutput());
    CheckWinding(clipper->GetClippedOutput());

    VTK_CREATE(vtkDataSetSurfaceFilter, surface1);
    surface1->SetInputConnection(0, clipper->GetOutputPort(0));

    VTK_CREATE(vtkPolyDataMapper, mapper1);
    mapper1->SetInputConnection(0, surface1->GetOutputPort(0));

    VTK_CREATE(vtkActor, actor1);
    actor1->SetMapper(mapper1);

    VTK_CREATE(vtkDataSetSurfaceFilter, surface2);
    surface2->SetInputConnection(clipper->GetOutputPort(1));

    VTK_CREATE(vtkPolyDataMapper, mapper2);
    mapper2->SetInputConnection(0, surface2->GetOutputPort(0));

    VTK_CREATE(vtkActor, actor2);
    actor2->SetMapper(mapper2);
    actor2->GetProperty()->SetColor(1.0, 0.5, 0.5);

    VTK_CREATE(vtkRenderer, renderer);
    renderer->AddActor(actor1);
    renderer->AddActor(actor2);
    renderer->SetBackground(0.0, 0.5, 0.5);
    PlaceRenderer(renderer, boxnum, i, 1);
    renderer->ResetCamera();
    renwin->AddRenderer(renderer);

    vtkCamera *camera = renderer->GetActiveCamera();
    camera->Azimuth(25.0);
    camera->Elevation(-25.0);
    }

  double minpoint[3], maxpoint[3];
  minpoint[0] = minx;  minpoint[1] = miny;  minpoint[2] = minz;
  maxpoint[0] = maxx;  maxpoint[1] = maxy;  maxpoint[2] = maxz;

  // Add tests for arbitrarily oriented box and no clipped output.
  for (i = 0; i < 12; i++)
    {
    vtkSmartPointer<vtkUnstructuredGrid> input = MakeTetrahedron(i);
    CheckWinding(input);

    VTK_CREATE(vtkBoxClipDataSet, clipper);
    clipper->SetInputData(input);
    clipper->GenerateClippedOutputOff();
    clipper->SetBoxClip(minusx, minpoint, minusy, minpoint, minusz, minpoint,
                        plusx, maxpoint, plusy, maxpoint, plusz, maxpoint);
    clipper->Update();
    CheckWinding(clipper->GetOutput());

    VTK_CREATE(vtkDataSetSurfaceFilter, surface);
    surface->SetInputConnection(0, clipper->GetOutputPort(0));

    VTK_CREATE(vtkPolyDataMapper, mapper);
    mapper->SetInputConnection(0, surface->GetOutputPort(0));

    VTK_CREATE(vtkActor, actor);
    actor->SetMapper(mapper);

    VTK_CREATE(vtkRenderer, renderer);
    renderer->AddActor(actor);
    renderer->SetBackground(0.0, 0.5, 0.5);
    PlaceRenderer(renderer, boxnum, i, 2);
    renderer->ResetCamera();
    renwin->AddRenderer(renderer);

    vtkCamera *camera = renderer->GetActiveCamera();
    camera->Azimuth(25.0);
    camera->Elevation(-25.0);
    }

  // Add tests for arbitrarily oriented box and clipped output.
  for (i = 0; i < 12; i++)
    {
    vtkSmartPointer<vtkUnstructuredGrid> input = MakeTetrahedron(i);
    CheckWinding(input);

    VTK_CREATE(vtkBoxClipDataSet, clipper);
    clipper->SetInputData(input);
    clipper->GenerateClippedOutputOn();
    clipper->SetBoxClip(minusx, minpoint, minusy, minpoint, minusz, minpoint,
                        plusx, maxpoint, plusy, maxpoint, plusz, maxpoint);
    clipper->Update();
    CheckWinding(clipper->GetOutput());
    CheckWinding(clipper->GetClippedOutput());

    VTK_CREATE(vtkDataSetSurfaceFilter, surface1);
    surface1->SetInputConnection(0, clipper->GetOutputPort(0));

    VTK_CREATE(vtkPolyDataMapper, mapper1);
    mapper1->SetInputConnection(0, surface1->GetOutputPort(0));

    VTK_CREATE(vtkActor, actor1);
    actor1->SetMapper(mapper1);

    VTK_CREATE(vtkDataSetSurfaceFilter, surface2);
    surface2->SetInputConnection(clipper->GetOutputPort(1));

    VTK_CREATE(vtkPolyDataMapper, mapper2);
    mapper2->SetInputConnection(0, surface2->GetOutputPort(0));

    VTK_CREATE(vtkActor, actor2);
    actor2->SetMapper(mapper2);
    actor2->GetProperty()->SetColor(1.0, 0.5, 0.5);

    VTK_CREATE(vtkRenderer, renderer);
    renderer->AddActor(actor1);
    renderer->AddActor(actor2);
    renderer->SetBackground(0.0, 0.5, 0.5);
    PlaceRenderer(renderer, boxnum, i, 3);
    renderer->ResetCamera();
    renwin->AddRenderer(renderer);

    vtkCamera *camera = renderer->GetActiveCamera();
    camera->Azimuth(25.0);
    camera->Elevation(-25.0);
    }
}


int BoxClipTetrahedra(int, char *[])
{
  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetSize(960, 640);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  try
    {
    TestBox(renwin, 0, 0.15, 2.0, -2.0, 2.0, -2.0, 2.0);
    TestBox(renwin, 1, -2.0, 0.15, -2.0, 2.0, -2.0, 2.0);
    TestBox(renwin, 2, -2.0, 2.0, -2.0, 2.0, -2.0, 0.4);
    TestBox(renwin, 3, -2.0, 2.0, -2.0, 2.0, 0.4, 2.0);
    TestBox(renwin, 4, -2.0, 2.0, -2.0, 2.0, -2.0, 0.5);
    TestBox(renwin, 5, -2.0, 2.0, -2.0, 2.0, 0.5, 2.0);
    TestBox(renwin, 6, -2.0, 0.0, -2.0, 2.0, -2.0, 2.0);
    TestBox(renwin, 7, 0.0, 2.0, -2.0, 2.0, -2.0, 2.0);
    }
  catch (BadWinding bw)
    {
    std::cout << "Encountered a bad winding.  Aborting test." << std::endl;
    return EXIT_FAILURE;
    }

  // Run the regression test.
  renwin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
