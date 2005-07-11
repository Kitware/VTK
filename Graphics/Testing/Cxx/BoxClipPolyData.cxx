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
#include "vtkDataSetSurfaceFilter.h"
#include "vtkDoubleArray.h"
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

static double minpoint1[] = { -1.00002, -0.50002, -0.50002 };
static double maxpoint1[] = { -0.0511337, 0.5, 0.5 };

static double minpoint2[] = { -1.0, -1.0, -1.0 };
static double maxpoint2[] = { 1.0, 1.0, 1.0 };

static double minusx[] = { -1.0, 0.0, 0.0 };
static double minusy[] = { 0.0, -1.0, 0.0 };
static double minusz[] = { 0.0, 0.0, -1.0 };
static double plusx[] = { 1.0, 0.0, 0.0 };
static double plusy[] = { 0.0, 1.0, 0.0 };
static double plusz[] = { 0.0, 0.0, 1.0 };

const int numTriangles = 6;
const int numTrianglePoints = numTriangles*3*3;
static double trianglePointData[numTrianglePoints] = {
  -2.0, -1.0, 0.0,
  0.0, -1.0, 0.0,
  -1.0, -0.5, 0.0,
  
  0.0, -1.0, 0.0,
  2.0, -1.0, 0.0,
  1.0, -0.5, 0.0,

  -1.0, 0.25, 0.0,
  -2.0, -0.25, 0.0,
  0.0, -0.25, 0.0,

  1.0, 0.25, 0.0,
  0.0, -0.25, 0.0,
  2.0, -0.25, 0.0,

  0.0, 0.5, 0.0,
  -1.0, 1.0, 0.0,
  -2.0, 0.5, 0.0,

  2.0, 0.5, 0.0,
  1.0, 1.0, 0.0,
  0.0, 0.5, 0.0
};

int BoxClipPolyData(int argc, char *argv[])
{
  int i;

  // The render window.
  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetSize(800, 400);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  // Input data sets.
  VTK_CREATE(vtkSphereSource, sphere);

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

  // Set up test of normal box.
  VTK_CREATE(vtkBoxClipDataSet, clipper1);
  clipper1->SetInputConnection(0, sphere->GetOutputPort(0));
  clipper1->GenerateClippedOutputOff();
  clipper1->SetBoxClip(minpoint1[0], maxpoint1[0],
                       minpoint1[1], maxpoint1[1],
                       minpoint1[2], maxpoint1[2]);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface1);
  surface1->SetInputConnection(0, clipper1->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper1);
  mapper1->SetInputConnection(0, surface1->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor1);
  actor1->SetMapper(mapper1);

  VTK_CREATE(vtkRenderer, renderer1);
  renderer1->AddActor(actor1);
  renderer1->SetBackground(0.0, 0.5, 0.5);
  renderer1->SetViewport(0.0, 0.0, 0.25, 0.5);
  renwin->AddRenderer(renderer1);

  // Set up test of normal box with generation of clipped output.
  VTK_CREATE(vtkBoxClipDataSet, clipper2);
  clipper2->SetInputConnection(0, sphere->GetOutputPort(0));
  clipper2->GenerateClippedOutputOn();
  clipper2->SetBoxClip(minpoint1[0], maxpoint1[0],
                       minpoint1[1], maxpoint1[1],
                       minpoint1[2], maxpoint1[2]);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface2_1);
  surface2_1->SetInputConnection(0, clipper2->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper2_1);
  mapper2_1->SetInputConnection(0, surface2_1->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor2_1);
  actor2_1->SetMapper(mapper2_1);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface2_2);
  surface2_2->SetInput(clipper2->GetClippedOutput());

  VTK_CREATE(vtkPolyDataMapper, mapper2_2);
  mapper2_2->SetInputConnection(0, surface2_2->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor2_2);
  actor2_2->SetMapper(mapper2_2);
  actor2_2->GetProperty()->SetColor(1.0, 0.5, 0.5);

  VTK_CREATE(vtkRenderer, renderer2);
  renderer2->AddActor(actor2_1);
  renderer2->AddActor(actor2_2);
  renderer2->SetBackground(0.0, 0.5, 0.5);
  renderer2->SetViewport(0.25, 0.0, 0.5, 0.5);
  renwin->AddRenderer(renderer2);

  // Set up test of an oriented box.
  VTK_CREATE(vtkBoxClipDataSet, clipper3);
  clipper3->SetInputConnection(0, sphere->GetOutputPort(0));
  clipper3->GenerateClippedOutputOff();
  clipper3->SetBoxClip(minusx, minpoint1,
                       minusy, minpoint1,
                       minusz, minpoint1,
                       plusx, maxpoint1,
                       plusy, maxpoint1,
                       plusz, maxpoint1);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface3);
  surface3->SetInputConnection(0, clipper3->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper3);
  mapper3->SetInputConnection(0, surface3->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor3);
  actor3->SetMapper(mapper3);

  VTK_CREATE(vtkRenderer, renderer3);
  renderer3->AddActor(actor3);
  renderer3->SetBackground(0.0, 0.5, 0.5);
  renderer3->SetViewport(0.5, 0.0, 0.75, 0.5);
  renwin->AddRenderer(renderer3);

  // Set up test of an oriented box with generation of clipped output.
  VTK_CREATE(vtkBoxClipDataSet, clipper4);
  clipper4->SetInputConnection(0, sphere->GetOutputPort(0));
  clipper4->GenerateClippedOutputOn();
  clipper4->SetBoxClip(minusx, minpoint1,
                       minusy, minpoint1,
                       minusz, minpoint1,
                       plusx, maxpoint1,
                       plusy, maxpoint1,
                       plusz, maxpoint1);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface4_1);
  surface4_1->SetInputConnection(0, clipper4->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper4_1);
  mapper4_1->SetInputConnection(0, surface4_1->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor4_1);
  actor4_1->SetMapper(mapper4_1);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface4_2);
  surface4_2->SetInput(clipper4->GetClippedOutput());

  VTK_CREATE(vtkPolyDataMapper, mapper4_2);
  mapper4_2->SetInputConnection(0, surface4_2->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor4_2);
  actor4_2->SetMapper(mapper4_2);
  actor4_2->GetProperty()->SetColor(1.0, 0.5, 0.5);

  VTK_CREATE(vtkRenderer, renderer4);
  renderer4->AddActor(actor4_1);
  renderer4->AddActor(actor4_2);
  renderer4->SetBackground(0.0, 0.5, 0.5);
  renderer4->SetViewport(0.75, 0.0, 1.0, 0.5);
  renwin->AddRenderer(renderer4);

  // Test triangle cut cleanly at a vertex with a regular box.
  VTK_CREATE(vtkBoxClipDataSet, clipper5);
  clipper5->SetInput(triangles);
  clipper5->GenerateClippedOutputOff();
  clipper5->SetBoxClip(minpoint2[0], maxpoint2[0],
                       minpoint2[1], maxpoint2[1],
                       minpoint2[2], maxpoint2[2]);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface5);
  surface5->SetInputConnection(0, clipper5->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper5);
  mapper5->SetInputConnection(0, surface5->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor5);
  actor5->SetMapper(mapper5);

  VTK_CREATE(vtkRenderer, renderer5);
  renderer5->AddActor(actor5);
  renderer5->SetBackground(0.0, 0.5, 0.5);
  renderer5->SetViewport(0.0, 0.5, 0.25, 1.0);
  renwin->AddRenderer(renderer5);

  // Test triangle cut cleanly at vertex with a regular box and clipped output
  // on.
  VTK_CREATE(vtkBoxClipDataSet, clipper6);
  clipper6->SetInput(triangles);
  clipper6->GenerateClippedOutputOn();
  clipper6->SetBoxClip(minpoint2[0], maxpoint2[0],
                       minpoint2[1], maxpoint2[1],
                       minpoint2[2], maxpoint2[2]);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface6_1);
  surface6_1->SetInputConnection(0, clipper6->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper6_1);
  mapper6_1->SetInputConnection(0, surface6_1->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor6_1);
  actor6_1->SetMapper(mapper6_1);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface6_2);
  surface6_2->SetInput(clipper6->GetClippedOutput());

  VTK_CREATE(vtkPolyDataMapper, mapper6_2);
  mapper6_2->SetInputConnection(0, surface6_2->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor6_2);
  actor6_2->SetMapper(mapper6_2);
  actor6_2->GetProperty()->SetColor(1.0, 0.5, 0.5);

  VTK_CREATE(vtkRenderer, renderer6);
  renderer6->AddActor(actor6_1);
  renderer6->AddActor(actor6_2);
  renderer6->SetBackground(0.0, 0.5, 0.5);
  renderer6->SetViewport(0.25, 0.5, 0.5, 1.0);
  renwin->AddRenderer(renderer6);

  // Test triangle cut cleanly at a vertex with an oriented box.
  VTK_CREATE(vtkBoxClipDataSet, clipper7);
  clipper7->SetInput(triangles);
  clipper7->GenerateClippedOutputOff();
  clipper7->SetBoxClip(minusx, minpoint2,
                       minusy, minpoint2,
                       minusz, minpoint2,
                       plusx, maxpoint2,
                       plusy, maxpoint2,
                       plusz, maxpoint2);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface7);
  surface7->SetInputConnection(0, clipper7->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper7);
  mapper7->SetInputConnection(0, surface7->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor7);
  actor7->SetMapper(mapper7);

  VTK_CREATE(vtkRenderer, renderer7);
  renderer7->AddActor(actor7);
  renderer7->SetBackground(0.0, 0.5, 0.5);
  renderer7->SetViewport(0.5, 0.5, 0.75, 1.0);
  renwin->AddRenderer(renderer7);

  // Test triangle cut cleanly at vertex with an oriented box and clipped output
  // on.
  VTK_CREATE(vtkBoxClipDataSet, clipper8);
  clipper8->SetInput(triangles);
  clipper8->GenerateClippedOutputOn();
  clipper8->SetBoxClip(minusx, minpoint2,
                       minusy, minpoint2,
                       minusz, minpoint2,
                       plusx, maxpoint2,
                       plusy, maxpoint2,
                       plusz, maxpoint2);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface8_1);
  surface8_1->SetInputConnection(0, clipper8->GetOutputPort(0));

  VTK_CREATE(vtkPolyDataMapper, mapper8_1);
  mapper8_1->SetInputConnection(0, surface8_1->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor8_1);
  actor8_1->SetMapper(mapper8_1);

  VTK_CREATE(vtkDataSetSurfaceFilter, surface8_2);
  surface8_2->SetInput(clipper8->GetClippedOutput());

  VTK_CREATE(vtkPolyDataMapper, mapper8_2);
  mapper8_2->SetInputConnection(0, surface8_2->GetOutputPort(0));

  VTK_CREATE(vtkActor, actor8_2);
  actor8_2->SetMapper(mapper8_2);
  actor8_2->GetProperty()->SetColor(1.0, 0.5, 0.5);

  VTK_CREATE(vtkRenderer, renderer8);
  renderer8->AddActor(actor8_1);
  renderer8->AddActor(actor8_2);
  renderer8->SetBackground(0.0, 0.5, 0.5);
  renderer8->SetViewport(0.75, 0.5, 1.0, 1.0);
  renwin->AddRenderer(renderer8);

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
