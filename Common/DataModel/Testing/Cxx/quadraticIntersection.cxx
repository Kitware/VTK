/*=========================================================================

  Program:   Visualization Toolkit
  Module:    quadraticIntersection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
// .NAME
// .SECTION Description
// This program tests quadratic cell IntersectWithLine() methods.

#include <sstream>

#include "vtkDebugLeaks.h"

#include "vtkRegressionTestImage.h"
#include "vtkTriQuadraticPyramid.h"
#include <vtkBiQuadraticQuad.h>
#include <vtkBiQuadraticQuadraticHexahedron.h>
#include <vtkBiQuadraticQuadraticWedge.h>
#include <vtkBiQuadraticTriangle.h>
#include <vtkCamera.h>
#include <vtkCubicLine.h>
#include <vtkIdList.h>
#include <vtkMath.h>
#include <vtkMinimalStandardRandomSequence.h>
#include <vtkPoints.h>
#include <vtkQuadraticEdge.h>
#include <vtkQuadraticHexahedron.h>
#include <vtkQuadraticLinearQuad.h>
#include <vtkQuadraticLinearWedge.h>
#include <vtkQuadraticPyramid.h>
#include <vtkQuadraticQuad.h>
#include <vtkQuadraticTetra.h>
#include <vtkQuadraticTriangle.h>
#include <vtkQuadraticWedge.h>
#include <vtkSmartPointer.h>
#include <vtkTriQuadraticHexahedron.h>

#include <vtkActor.h>
#include <vtkCellArray.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkVersion.h>

void ViewportRange(int testNum, double* range)
{
  range[0] = 0.2 * (testNum % 5);
  range[1] = range[0] + 0.2;
  range[2] = (1. / 4.) * (testNum / 5);
  range[3] = range[2] + (1. / 4.);
}

void RandomCircle(
  vtkMinimalStandardRandomSequence* sequence, double radius, double* offset, double* value)
{
  double theta = 2. * vtkMath::Pi() * sequence->GetValue();
  sequence->Next();
  value[0] = radius * cos(theta) + offset[0];
  value[1] = radius * sin(theta) + offset[1];
}

void RandomSphere(
  vtkMinimalStandardRandomSequence* sequence, double radius, double* offset, double* value)
{
  double theta = 2. * vtkMath::Pi() * sequence->GetValue();
  sequence->Next();
  double phi = vtkMath::Pi() * sequence->GetValue();
  sequence->Next();
  value[0] = radius * cos(theta) * sin(phi) + offset[0];
  value[1] = radius * sin(theta) * sin(phi) + offset[1];
  value[2] = radius * cos(phi) + offset[2];
}

void IntersectWithCell(unsigned nTest, vtkMinimalStandardRandomSequence* sequence,
  bool threeDimensional, double radius, double* offset, vtkCell* cell,
  vtkSmartPointer<vtkRenderWindow> renderWindow)
{
  double p[2][3];
  p[0][2] = p[1][2] = 0.;
  double tol = 1.e-7;
  double t;
  double intersect[3];
  double pcoords[3];
  int subId;

  auto points = vtkSmartPointer<vtkPoints>::New();
  auto vertices = vtkSmartPointer<vtkCellArray>::New();

  for (unsigned i = 0; i < nTest; i++)
  {
    if (threeDimensional)
    {
      RandomSphere(sequence, radius, offset, p[0]);
      RandomSphere(sequence, radius, offset, p[1]);
    }
    else
    {
      RandomCircle(sequence, radius, offset, p[0]);
      RandomCircle(sequence, radius, offset, p[1]);
    }

    if (cell->IntersectWithLine(p[0], p[1], tol, t, intersect, pcoords, subId))
    {
      vtkIdType pid = points->InsertNextPoint(intersect);
      vertices->InsertNextCell(1, &pid);
    }
  }

  auto camera = vtkSmartPointer<vtkCamera>::New();
  camera->SetPosition(2, 2, 2);
  camera->SetFocalPoint(offset[0], offset[1], offset[2]);

  auto renderer = vtkSmartPointer<vtkRenderer>::New();
  renderer->SetActiveCamera(camera);
  renderWindow->AddRenderer(renderer);
  double dim[4];
  static int testNum = 0;
  ViewportRange(testNum++, dim);
  renderer->SetViewport(dim[0], dim[2], dim[1], dim[3]);

  auto point = vtkSmartPointer<vtkPolyData>::New();

  point->SetPoints(points);
  point->SetVerts(vertices);

  auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
  mapper->SetInputData(point);

  auto actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  renderer->AddActor(actor);
  renderer->ResetCamera();

  renderWindow->Render();
}

int TestIntersectWithLine(int argc, char* argv[])
{
  std::ostringstream strm;
  strm << "Test vtkCell::IntersectWithLine Start" << endl;

  auto renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
  renderWindow->SetMultiSamples(0);
  renderWindow->SetSize(800, 600);
  auto renderer = vtkSmartPointer<vtkRenderer>::New();
  renderWindow->AddRenderer(renderer);
  renderWindow->Render();

  auto renderWindowInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();

  renderWindowInteractor->SetRenderWindow(renderWindow);

  auto sequence = vtkSmartPointer<vtkMinimalStandardRandomSequence>::New();

  sequence->SetSeed(1);

  unsigned nTest = 1.e4;
  double radius = 1.5;
  double center[3] = { 0.5, 0.25, 0. };

  // vtkQuadraticEdge
  auto edge = vtkSmartPointer<vtkQuadraticEdge>::New();

  for (int i = 0; i < edge->GetNumberOfPoints(); ++i)
  {
    edge->GetPointIds()->SetId(i, i);
  }

  edge->GetPoints()->SetPoint(0, 0, 0, 0);
  edge->GetPoints()->SetPoint(1, 1, 0, 0);
  edge->GetPoints()->SetPoint(2, 0.5, 0.25, 0);

  IntersectWithCell(nTest, sequence, false, radius, center, edge, renderWindow);

  // vtkQuadraticTriangle
  auto tri = vtkSmartPointer<vtkQuadraticTriangle>::New();

  for (int i = 0; i < tri->GetNumberOfPoints(); ++i)
  {
    tri->GetPointIds()->SetId(i, i);
  }

  tri->GetPoints()->SetPoint(0, 0, 0, 0);
  tri->GetPoints()->SetPoint(1, 1, 0, 0);
  tri->GetPoints()->SetPoint(2, 0.5, 0.8, 0);
  tri->GetPoints()->SetPoint(3, 0.5, 0.0, 0);
  tri->GetPoints()->SetPoint(4, 0.75, 0.4, 0);
  tri->GetPoints()->SetPoint(5, 0.25, 0.4, 0);

  center[0] = 0.5;
  center[1] = 0.5;
  center[2] = 0.;
  // interestingly, triangles are invisible edge-on. Test in 3D
  IntersectWithCell(nTest, sequence, true, radius, center, tri, renderWindow);

  // vtkQuadraticQuad
  auto quad = vtkSmartPointer<vtkQuadraticQuad>::New();

  for (int i = 0; i < quad->GetNumberOfPoints(); ++i)
  {
    quad->GetPointIds()->SetId(i, i);
  }

  quad->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  quad->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  quad->GetPoints()->SetPoint(2, 1.0, 1.0, 0.0);
  quad->GetPoints()->SetPoint(3, 0.0, 1.0, 0.0);
  quad->GetPoints()->SetPoint(4, 0.5, 0.0, 0.0);
  quad->GetPoints()->SetPoint(5, 1.0, 0.5, 0.0);
  quad->GetPoints()->SetPoint(6, 0.5, 1.0, 0.0);
  quad->GetPoints()->SetPoint(7, 0.0, 0.5, 0.0);

  IntersectWithCell(nTest, sequence, true, radius, center, quad, renderWindow);

  // vtkQuadraticTetra
  auto tetra = vtkSmartPointer<vtkQuadraticTetra>::New();

  for (int i = 0; i < tetra->GetNumberOfPoints(); ++i)
  {
    tetra->GetPointIds()->SetId(i, i);
  }

  tetra->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  tetra->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  tetra->GetPoints()->SetPoint(2, 0.5, 0.8, 0.0);
  tetra->GetPoints()->SetPoint(3, 0.5, 0.4, 1.0);
  tetra->GetPoints()->SetPoint(4, 0.5, 0.0, 0.0);
  tetra->GetPoints()->SetPoint(5, 0.75, 0.4, 0.0);
  tetra->GetPoints()->SetPoint(6, 0.25, 0.4, 0.0);
  tetra->GetPoints()->SetPoint(7, 0.25, 0.2, 0.5);
  tetra->GetPoints()->SetPoint(8, 0.75, 0.2, 0.5);
  tetra->GetPoints()->SetPoint(9, 0.50, 0.6, 0.5);

  IntersectWithCell(nTest, sequence, true, radius, center, tetra, renderWindow);

  // vtkQuadraticHexahedron
  auto hex = vtkSmartPointer<vtkQuadraticHexahedron>::New();

  for (int i = 0; i < hex->GetNumberOfPoints(); ++i)
  {
    hex->GetPointIds()->SetId(i, i);
  }

  hex->GetPoints()->SetPoint(0, 0, 0, 0);
  hex->GetPoints()->SetPoint(1, 1, 0, 0);
  hex->GetPoints()->SetPoint(2, 1, 1, 0);
  hex->GetPoints()->SetPoint(3, 0, 1, 0);
  hex->GetPoints()->SetPoint(4, 0, 0, 1);
  hex->GetPoints()->SetPoint(5, 1, 0, 1);
  hex->GetPoints()->SetPoint(6, 1, 1, 1);
  hex->GetPoints()->SetPoint(7, 0, 1, 1);
  hex->GetPoints()->SetPoint(8, 0.5, 0, 0);
  hex->GetPoints()->SetPoint(9, 1, 0.5, 0);
  hex->GetPoints()->SetPoint(10, 0.5, 1, 0);
  hex->GetPoints()->SetPoint(11, 0, 0.5, 0);
  hex->GetPoints()->SetPoint(12, 0.5, 0, 1);
  hex->GetPoints()->SetPoint(13, 1, 0.5, 1);
  hex->GetPoints()->SetPoint(14, 0.5, 1, 1);
  hex->GetPoints()->SetPoint(15, 0, 0.5, 1);
  hex->GetPoints()->SetPoint(16, 0, 0, 0.5);
  hex->GetPoints()->SetPoint(17, 1, 0, 0.5);
  hex->GetPoints()->SetPoint(18, 1, 1, 0.5);
  hex->GetPoints()->SetPoint(19, 0, 1, 0.5);

  IntersectWithCell(nTest, sequence, true, radius, center, hex, renderWindow);

  // vtkQuadraticWedge
  auto wedge = vtkSmartPointer<vtkQuadraticWedge>::New();
  double* pcoords = wedge->GetParametricCoords();
  for (int i = 0; i < wedge->GetNumberOfPoints(); ++i)
  {
    wedge->GetPointIds()->SetId(i, i);
    wedge->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }

  IntersectWithCell(nTest, sequence, true, radius, center, wedge, renderWindow);

  // vtkQuadraticPyramid
  auto pyra = vtkSmartPointer<vtkQuadraticPyramid>::New();

  for (int i = 0; i < pyra->GetNumberOfPoints(); ++i)
  {
    pyra->GetPointIds()->SetId(i, i);
  }

  pyra->GetPoints()->SetPoint(0, 0, 0, 0);
  pyra->GetPoints()->SetPoint(1, 1, 0, 0);
  pyra->GetPoints()->SetPoint(2, 1, 1, 0);
  pyra->GetPoints()->SetPoint(3, 0, 1, 0);
  pyra->GetPoints()->SetPoint(4, 0, 0, 1);
  pyra->GetPoints()->SetPoint(5, 0.5, 0, 0);
  pyra->GetPoints()->SetPoint(6, 1, 0.5, 0);
  pyra->GetPoints()->SetPoint(7, 0.5, 1, 0);
  pyra->GetPoints()->SetPoint(8, 0, 0.5, 0);
  pyra->GetPoints()->SetPoint(9, 0, 0, 0.5);
  pyra->GetPoints()->SetPoint(10, 0.5, 0, 0.5);
  pyra->GetPoints()->SetPoint(11, 0.5, 0.5, 0.5);
  pyra->GetPoints()->SetPoint(12, 0, 0.5, 0.5);

  IntersectWithCell(nTest, sequence, true, radius, center, pyra, renderWindow);

  // vtkQuadraticLinearQuad
  auto quadlin = vtkSmartPointer<vtkQuadraticLinearQuad>::New();
  double* paramcoor = quadlin->GetParametricCoords();

  for (int i = 0; i < quadlin->GetNumberOfPoints(); i++)
  {
    quadlin->GetPointIds()->SetId(i, i);
  }

  for (int i = 0; i < quadlin->GetNumberOfPoints(); i++)
  {
    quadlin->GetPoints()->SetPoint(i, paramcoor[i * 3], paramcoor[i * 3 + 1], paramcoor[i * 3 + 2]);
  }

  IntersectWithCell(nTest, sequence, true, radius, center, quadlin, renderWindow);

  // vtkBiQuadraticQuad
  auto biquad = vtkSmartPointer<vtkBiQuadraticQuad>::New();
  paramcoor = biquad->GetParametricCoords();

  for (int i = 0; i < biquad->GetNumberOfPoints(); i++)
    biquad->GetPointIds()->SetId(i, i);

  for (int i = 0; i < biquad->GetNumberOfPoints(); i++)
  {
    biquad->GetPoints()->SetPoint(i, paramcoor[i * 3], paramcoor[i * 3 + 1], paramcoor[i * 3 + 2]);
  }

  IntersectWithCell(nTest, sequence, true, radius, center, biquad, renderWindow);

  // vtkQuadraticLinearWedge
  auto wedgelin = vtkSmartPointer<vtkQuadraticLinearWedge>::New();
  paramcoor = wedgelin->GetParametricCoords();

  for (int i = 0; i < wedgelin->GetNumberOfPoints(); i++)
  {
    wedgelin->GetPointIds()->SetId(i, i);
  }

  for (int i = 0; i < wedgelin->GetNumberOfPoints(); i++)
  {
    wedgelin->GetPoints()->SetPoint(
      i, paramcoor[i * 3], paramcoor[i * 3 + 1], paramcoor[i * 3 + 2]);
  }

  IntersectWithCell(nTest, sequence, true, radius, center, wedgelin, renderWindow);

  // vtkBiQuadraticQuadraticWedge
  auto biwedge = vtkSmartPointer<vtkBiQuadraticQuadraticWedge>::New();
  paramcoor = biwedge->GetParametricCoords();

  for (int i = 0; i < biwedge->GetNumberOfPoints(); i++)
  {
    biwedge->GetPointIds()->SetId(i, i);
  }

  for (int i = 0; i < biwedge->GetNumberOfPoints(); i++)
  {
    biwedge->GetPoints()->SetPoint(i, paramcoor[i * 3], paramcoor[i * 3 + 1], paramcoor[i * 3 + 2]);
  }

  IntersectWithCell(nTest, sequence, true, radius, center, biwedge, renderWindow);
  // vtkBiQuadraticQuadraticHexahedron
  auto bihex = vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron>::New();
  paramcoor = bihex->GetParametricCoords();

  for (int i = 0; i < bihex->GetNumberOfPoints(); i++)
  {
    bihex->GetPointIds()->SetId(i, i);
  }

  for (int i = 0; i < bihex->GetNumberOfPoints(); i++)
  {
    bihex->GetPoints()->SetPoint(i, paramcoor[i * 3], paramcoor[i * 3 + 1], paramcoor[i * 3 + 2]);
  }

  IntersectWithCell(nTest, sequence, true, radius, center, bihex, renderWindow);

  // vtkTriQuadraticHexahedron
  auto trihex = vtkSmartPointer<vtkTriQuadraticHexahedron>::New();
  paramcoor = trihex->GetParametricCoords();

  for (int i = 0; i < trihex->GetNumberOfPoints(); i++)
  {
    trihex->GetPointIds()->SetId(i, i);
  }

  for (int i = 0; i < trihex->GetNumberOfPoints(); i++)
  {
    trihex->GetPoints()->SetPoint(i, paramcoor[i * 3], paramcoor[i * 3 + 1], paramcoor[i * 3 + 2]);
  }

  IntersectWithCell(nTest, sequence, true, radius, center, trihex, renderWindow);

  // vtkTriQuadraticPyramid
  auto tqPyra = vtkSmartPointer<vtkTriQuadraticPyramid>::New();

  for (int i = 0; i < tqPyra->GetNumberOfPoints(); ++i)
  {
    tqPyra->GetPointIds()->SetId(i, i);
  }

  tqPyra->GetPoints()->SetPoint(0, 0, 0, 0);
  tqPyra->GetPoints()->SetPoint(1, 1, 0, 0);
  tqPyra->GetPoints()->SetPoint(2, 1, 1, 0);
  tqPyra->GetPoints()->SetPoint(3, 0, 1, 0);
  tqPyra->GetPoints()->SetPoint(4, 0, 0, 1);
  tqPyra->GetPoints()->SetPoint(5, 0.5, 0, 0);
  tqPyra->GetPoints()->SetPoint(6, 1, 0.5, 0);
  tqPyra->GetPoints()->SetPoint(7, 0.5, 1, 0);
  tqPyra->GetPoints()->SetPoint(8, 0, 0.5, 0);
  tqPyra->GetPoints()->SetPoint(9, 0, 0, 0.5);
  tqPyra->GetPoints()->SetPoint(10, 0.5, 0, 0.5);
  tqPyra->GetPoints()->SetPoint(11, 0.5, 0.5, 0.5);
  tqPyra->GetPoints()->SetPoint(12, 0, 0.5, 0.5);
  tqPyra->GetPoints()->SetPoint(13, 0.5, 0.5, 0);
  tqPyra->GetPoints()->SetPoint(14, 1.0 / 3.0, 0, 1.0 / 3.0);
  tqPyra->GetPoints()->SetPoint(15, 2.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0);
  tqPyra->GetPoints()->SetPoint(16, 1.0 / 3.0, 2.0 / 3.0, 1.0 / 3.0);
  tqPyra->GetPoints()->SetPoint(17, 0, 1.0 / 3.0, 1.0 / 3.0);
  tqPyra->GetPoints()->SetPoint(18, 0.4, 0.4, 0.2);

  IntersectWithCell(nTest, sequence, true, radius, center, tqPyra, renderWindow);

  // vtkBiQuadraticTriangle
  auto bitri = vtkSmartPointer<vtkBiQuadraticTriangle>::New();

  for (int i = 0; i < bitri->GetNumberOfPoints(); ++i)
  {
    bitri->GetPointIds()->SetId(i, i);
  }

  bitri->GetPoints()->SetPoint(0, 0, 0, 0);
  bitri->GetPoints()->SetPoint(1, 1, 0, 0);
  bitri->GetPoints()->SetPoint(2, 0.5, 0.8, 0);
  bitri->GetPoints()->SetPoint(3, 0.5, 0.0, 0);
  bitri->GetPoints()->SetPoint(4, 0.75, 0.4, 0);
  bitri->GetPoints()->SetPoint(5, 0.25, 0.4, 0);
  bitri->GetPoints()->SetPoint(6, 0.45, 0.24, 0);

  IntersectWithCell(nTest, sequence, true, radius, center, bitri, renderWindow);

  // vtkCubicLine
  auto culine = vtkSmartPointer<vtkCubicLine>::New();

  for (int i = 0; i < culine->GetNumberOfPoints(); ++i)
  {
    culine->GetPointIds()->SetId(i, i);
  }

  culine->GetPoints()->SetPoint(0, 0, 0, 0);
  culine->GetPoints()->SetPoint(1, 1, 0, 0);
  culine->GetPoints()->SetPoint(2, (1.0 / 3.0), -0.1, 0);
  culine->GetPoints()->SetPoint(3, (1.0 / 3.0), 0.1, 0);

  IntersectWithCell(nTest, sequence, false, radius, center, culine, renderWindow);

  strm << "Test vtkCell::IntersectWithLine End" << endl;

  renderWindowInteractor->Initialize();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    renderWindowInteractor->Start();
    retVal = vtkRegressionTester::PASSED;
  }
  return (retVal == vtkRegressionTester::PASSED) ? EXIT_SUCCESS : EXIT_FAILURE;
}

int quadraticIntersection(int argc, char* argv[])
{
  return TestIntersectWithLine(argc, argv);
}
