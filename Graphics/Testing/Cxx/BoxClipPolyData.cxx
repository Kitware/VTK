/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BoxClipPolyData.cxx

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

#include "vtkActor.h"
#include "vtkBoxClipDataSet.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCutter.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
#include "vtkPlane.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSphereSource.h"
#include "vtkUnstructuredGrid.h"

#include "vtkRegressionTestImage.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, var) \
  vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

const double minpoint1[] = { -1.00002, -0.50002, -0.50002 };
const double maxpoint1[] = { -0.0511337, 0.5, 0.5 };

const double minpoint2[] = { -3.0, -1.0, -1.0 };
const double maxpoint2[] = { -1.0, 1.0, 1.0 };

const double minpoint3[] = { -3.0, -1.0, 0.0 };
const double maxpoint3[] = { 0.0, 0.5, 1.0 };

const double minusx[] = { -1.0, 0.0, 0.0 };
const double minusy[] = { 0.0, -1.0, 0.0 };
const double minusz[] = { 0.0, 0.0, -1.0 };
const double plusx[] = { 1.0, 0.0, 0.0 };
const double plusy[] = { 0.0, 1.0, 0.0 };
const double plusz[] = { 0.0, 0.0, 1.0 };

const int numTriangles = 6;
const int numTrianglePoints = numTriangles*3*3;
static double trianglePointData[numTrianglePoints] = {
  -4.0, -1.0, 0.0,
  -2.0, -1.0, 0.0,
  -3.0, -0.5, 0.0,
  
  -2.0, -1.0, 0.0,
  -1.0e-17, -1.0, 0.0,
  -1.0, -0.5, 0.0,

  -3.0, 0.25, 0.0,
  -4.0, -0.25, 0.0,
  -2.0, -0.25, 0.0,

  -1.0, 0.25, 0.0,
  -2.0, -0.25, 0.0,
  1.0e-17, -0.25, 0.0,

  -2.0, 0.5, 0.0,
  -3.0, 1.0, 0.0,
  -4.0, 0.5, 0.0,

  1.0e-17, 0.5, 0.0,
  -1.0, 1.0, 0.0,
  -2.0, 0.5, 0.0
};

//-----------------------------------------------------------------------------

const int numPolySets = 5;

static void TestPolyData(vtkPolyData *data, int num, vtkRenderWindow *renwin,
                         const double minBoxPoint[3],
                         const double maxBoxPoint[3])
{
  // Set up test of normal box.
  VTK_CREATE(vtkBoxClipDataSet, clipper1);
  clipper1->SetInput(data);
  clipper1->GenerateClippedOutputOff();
  clipper1->SetBoxClip(minBoxPoint[0], maxBoxPoint[0],
                       minBoxPoint[1], maxBoxPoint[1],
                       minBoxPoint[2], maxBoxPoint[2]);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface1);
  surface1->SetInputConnection(0, clipper1->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper1);
  mapper1->SetInputConnection(0, surface1->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor1);
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetPointSize(3.0f);

  VTK_CREATE(vtkRenderer, renderer1);
  renderer1->AddActor(actor1);
  renderer1->SetBackground(0.0, 0.5, 0.5);
  renderer1->SetViewport(static_cast<double>(num)/numPolySets, 0.75,
                         static_cast<double>(num+1)/numPolySets, 1.0);
  renwin->AddRenderer(renderer1);

  // Set up test of normal box with generation of clipped output.
  VTK_CREATE(vtkBoxClipDataSet, clipper2);
  clipper2->SetInput(data);
  clipper2->GenerateClippedOutputOn();
  clipper2->SetBoxClip(minBoxPoint[0], maxBoxPoint[0],
                       minBoxPoint[1], maxBoxPoint[1],
                       minBoxPoint[2], maxBoxPoint[2]);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface2_1);
  surface2_1->SetInputConnection(0, clipper2->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper2_1);
  mapper2_1->SetInputConnection(0, surface2_1->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor2_1);
  actor2_1->SetMapper(mapper2_1);
  actor2_1->GetProperty()->SetPointSize(3.0f);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface2_2);
  surface2_2->SetInput(clipper2->GetClippedOutput());

  VTK_CREATE(vtkPolyDataMapper, mapper2_2);
  mapper2_2->SetInputConnection(0, surface2_2->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor2_2);
  actor2_2->SetMapper(mapper2_2);
  actor2_2->GetProperty()->SetColor(1.0, 0.5, 0.5);
  actor2_2->GetProperty()->SetPointSize(3.0f);

  VTK_CREATE(vtkRenderer, renderer2);
  renderer2->AddActor(actor2_1);
  renderer2->AddActor(actor2_2);
  renderer2->SetBackground(0.0, 0.5, 0.5);
  renderer2->SetViewport(static_cast<double>(num)/numPolySets, 0.5,
                         static_cast<double>(num+1)/numPolySets, 0.75);
  renwin->AddRenderer(renderer2);

  // Set up test of an oriented box.
  VTK_CREATE(vtkBoxClipDataSet, clipper3);
  clipper3->SetInput(data);
  clipper3->GenerateClippedOutputOff();
  clipper3->SetBoxClip(minusx, minBoxPoint,
                       minusy, minBoxPoint,
                       minusz, minBoxPoint,
                       plusx, maxBoxPoint,
                       plusy, maxBoxPoint,
                       plusz, maxBoxPoint);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface3);
  surface3->SetInputConnection(0, clipper3->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper3);
  mapper3->SetInputConnection(0, surface3->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor3);
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetPointSize(3.0f);

  VTK_CREATE(vtkRenderer, renderer3);
  renderer3->AddActor(actor3);
  renderer3->SetBackground(0.0, 0.5, 0.5);
  renderer3->SetViewport(static_cast<double>(num)/numPolySets, 0.25,
                         static_cast<double>(num+1)/numPolySets, 0.5);
  renwin->AddRenderer(renderer3);

  // Set up test of an oriented box with generation of clipped output.
  VTK_CREATE(vtkBoxClipDataSet, clipper4);
  clipper4->SetInput(data);
  clipper4->GenerateClippedOutputOn();
  clipper4->SetBoxClip(minusx, minBoxPoint,
                       minusy, minBoxPoint,
                       minusz, minBoxPoint,
                       plusx, maxBoxPoint,
                       plusy, maxBoxPoint,
                       plusz, maxBoxPoint);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface4_1);
  surface4_1->SetInputConnection(0, clipper4->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper4_1);
  mapper4_1->SetInputConnection(0, surface4_1->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor4_1);
  actor4_1->SetMapper(mapper4_1);
  actor4_1->GetProperty()->SetPointSize(3.0f);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface4_2);
  surface4_2->SetInput(clipper4->GetClippedOutput());

  VTK_CREATE(vtkPolyDataMapper, mapper4_2);
  mapper4_2->SetInputConnection(0, surface4_2->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor4_2);
  actor4_2->SetMapper(mapper4_2);
  actor4_2->GetProperty()->SetColor(1.0, 0.5, 0.5);
  actor4_2->GetProperty()->SetPointSize(3.0f);

  VTK_CREATE(vtkRenderer, renderer4);
  renderer4->AddActor(actor4_1);
  renderer4->AddActor(actor4_2);
  renderer4->SetBackground(0.0, 0.5, 0.5);
  renderer4->SetViewport(static_cast<double>(num)/numPolySets, 0.0,
                         static_cast<double>(num+1)/numPolySets, 0.25);
  renwin->AddRenderer(renderer4);
}

//-----------------------------------------------------------------------------

int BoxClipPolyData(int argc, char *argv[])
{
  vtkIdType i;

  // The render window.
  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetSize(800, 640);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  // Test polygons on a sphere
  VTK_CREATE(vtkSphereSource, sphere);
  sphere->Update();

  TestPolyData(sphere->GetOutput(), 0, renwin, minpoint1, maxpoint1);

  // Test a triangle with points right on the box.
  VTK_CREATE(vtkDoubleArray, trianglePointsArray);
  trianglePointsArray->SetArray(trianglePointData, numTrianglePoints, 1);
  trianglePointsArray->SetNumberOfComponents(3);
  trianglePointsArray->SetNumberOfTuples(numTriangles*3);

  VTK_CREATE(vtkPoints, trianglePoints);
  trianglePoints->SetData(trianglePointsArray);

  VTK_CREATE(vtkDoubleArray, triangleNormals);
  triangleNormals->SetName("Normals");
  triangleNormals->SetNumberOfComponents(3);
  triangleNormals->SetNumberOfTuples(numTriangles);

  VTK_CREATE(vtkCellArray, triangleCells);
  triangleCells->Allocate(numTriangles*4);

  for (i = 0; i < numTriangles; i++)
    {
    triangleNormals->SetTuple3(i, 0.0, 0.0, 1.0);

    vtkIdType pts[3];
    pts[0] = i*3+0;  pts[1] = i*3+1;  pts[2] = i*3+2;
    triangleCells->InsertNextCell(3, pts);
    }

  VTK_CREATE(vtkPolyData, triangles);
  triangles->SetPoints(trianglePoints);
  triangles->SetPolys(triangleCells);
  triangles->GetCellData()->SetNormals(triangleNormals);

  TestPolyData(triangles, 1, renwin, minpoint2, maxpoint2);

  // Test triangles co-planer with a face of the bounding box.
  TestPolyData(triangles, 2, renwin, minpoint3, maxpoint3);

  // Test lines.
  VTK_CREATE(vtkPolyData, sphereNoNormals);
  sphereNoNormals->CopyStructure(sphere->GetOutput());

  VTK_CREATE(vtkPlane, plane);
  plane->SetOrigin(0.0, 0.0, 0.0);
  plane->SetNormal(0.0, 0.0, 1.0);

  VTK_CREATE(vtkCutter, cutter);
  cutter->SetInput(sphereNoNormals);
  cutter->SetCutFunction(plane);
  cutter->Update();

  TestPolyData(cutter->GetOutput(), 3, renwin, minpoint1, maxpoint1);

  // Test verts.
  VTK_CREATE(vtkPolyData, verts);
  vtkPoints *vertsPoints = sphereNoNormals->GetPoints();
  verts->SetPoints(vertsPoints);

  VTK_CREATE(vtkCellArray, vertsCells);
  vertsCells->Allocate(2*vertsPoints->GetNumberOfPoints());
  for (i = 0; i < vertsPoints->GetNumberOfPoints(); i++)
    {
    vertsCells->InsertNextCell(1, &i);
    }
  verts->SetVerts(vertsCells);

  TestPolyData(verts, 4, renwin, minpoint1, maxpoint1);

  // Run the regression test.
  renwin->Render();
  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    return 0;
    }

  return !retVal;
}
