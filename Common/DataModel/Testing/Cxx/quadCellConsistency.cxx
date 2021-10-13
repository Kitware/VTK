/*=========================================================================

  Program:   Visualization Toolkit
  Module:    quadCellConsistency.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME
// .SECTION Description
// this program tests the consistency of face/edge ids between linear and quadratic cells

#include "vtkBiQuadraticQuad.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkCubicLine.h"
#include "vtkHexahedron.h"
#include "vtkLine.h"
#include "vtkPyramid.h"
#include "vtkQuad.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkQuadraticPyramid.h"
#include "vtkQuadraticQuad.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticTriangle.h"
#include "vtkQuadraticWedge.h"
#include "vtkTetra.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkTriQuadraticPyramid.h"
#include "vtkTriangle.h"
#include "vtkWedge.h"

void InitializeCell(vtkCell* cell)
{
  // Default initialize the cell ids to 0,1,2 ... n
  int n = cell->GetNumberOfPoints();
  for (int i = 0; i < n; i++)
  {
    cell->GetPointIds()->SetId(i, i);
  }
}

// Check that corner points id match quad ones for each edges
int CompareCellEdges(vtkCell* linear, vtkCell* quadratic)
{
  int dif;
  int sum = 0;
  int nEdges = linear->GetNumberOfEdges();
  for (int edge = 0; edge < nEdges; edge++)
  {
    vtkCell* lEdge = linear->GetEdge(edge);
    vtkCell* qEdge = quadratic->GetEdge(edge);

    int n = lEdge->GetNumberOfPoints();
    // Check that the points of the linear cell match the one from the quadratic one
    for (int i = 0; i < n; i++)
    {
      dif = lEdge->GetPointIds()->GetId(i) - qEdge->GetPointIds()->GetId(i);
      sum += dif;
    }
  }
  return sum;
}

// Check that corner points id match quad ones for each faces
int CompareCellFaces(vtkCell* linear, vtkCell* quadratic)
{
  int dif;
  int sum = 0;
  int nFaces = linear->GetNumberOfFaces();
  for (int face = 0; face < nFaces; face++)
  {
    vtkCell* lFace = linear->GetFace(face);
    vtkCell* qFace = quadratic->GetFace(face);

    int n = lFace->GetNumberOfPoints();
    // Check that linear Triangle match quad Tri
    if (lFace->GetCellType() == VTK_TRIANGLE)
      sum += (qFace->GetCellType() != VTK_QUADRATIC_TRIANGLE &&
        qFace->GetCellType() != VTK_BIQUADRATIC_TRIANGLE);
    // Check that linear Quad match quad Quad
    if (lFace->GetCellType() == VTK_QUAD &&
      (qFace->GetCellType() != VTK_QUADRATIC_QUAD && qFace->GetCellType() != VTK_BIQUADRATIC_QUAD &&
        qFace->GetCellType() != VTK_QUADRATIC_LINEAR_QUAD))
      sum++;
    // Check that the points of the linear cell match the one from the quadratic one
    for (int i = 0; i < n; i++)
    {
      dif = lFace->GetPointIds()->GetId(i) - qFace->GetPointIds()->GetId(i);
      sum += dif;
    }
  }
  return sum;
}

int quadCellConsistency(int, char*[])
{
  int ret = 0;
  // Line / vtkQuadraticEdge / CubicLine:
  auto edge = vtkSmartPointer<vtkLine>::New();
  auto qedge = vtkSmartPointer<vtkQuadraticEdge>::New();
  auto culine = vtkSmartPointer<vtkCubicLine>::New();

  InitializeCell(edge);
  InitializeCell(qedge);
  ret += CompareCellEdges(edge, qedge);
  ret += CompareCellFaces(edge, qedge);

  InitializeCell(culine);
  ret += CompareCellEdges(edge, culine);
  ret += CompareCellFaces(edge, culine);

  // Triangles:
  auto tri = vtkSmartPointer<vtkTriangle>::New();
  auto qtri = vtkSmartPointer<vtkQuadraticTriangle>::New();
  auto bitri = vtkSmartPointer<vtkBiQuadraticTriangle>::New();

  InitializeCell(tri);
  InitializeCell(qtri);
  ret += CompareCellEdges(tri, qtri);
  ret += CompareCellFaces(tri, qtri);

  InitializeCell(bitri);
  ret += CompareCellEdges(tri, bitri);
  ret += CompareCellFaces(tri, bitri);

  // Quad
  auto quad = vtkSmartPointer<vtkQuad>::New();
  auto qquad = vtkSmartPointer<vtkQuadraticQuad>::New();
  auto biqquad = vtkSmartPointer<vtkBiQuadraticQuad>::New();
  auto qlquad = vtkSmartPointer<vtkQuadraticLinearQuad>::New();

  InitializeCell(quad);
  InitializeCell(qquad);
  InitializeCell(biqquad);
  InitializeCell(qlquad);
  ret += CompareCellEdges(quad, qquad);
  ret += CompareCellFaces(quad, qquad);
  ret += CompareCellEdges(quad, biqquad);
  ret += CompareCellFaces(quad, biqquad);
  ret += CompareCellEdges(quad, qlquad);
  ret += CompareCellFaces(quad, qlquad);

  // Tetra
  auto tetra = vtkSmartPointer<vtkTetra>::New();
  auto qtetra = vtkSmartPointer<vtkQuadraticTetra>::New();

  InitializeCell(tetra);
  InitializeCell(qtetra);
  ret += CompareCellEdges(tetra, qtetra);
  ret += CompareCellFaces(tetra, qtetra);

  // Hexhedron
  auto hex = vtkSmartPointer<vtkHexahedron>::New();
  auto qhex = vtkSmartPointer<vtkQuadraticHexahedron>::New();
  auto triqhex = vtkSmartPointer<vtkTriQuadraticHexahedron>::New();
  auto biqqhex = vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron>::New();

  InitializeCell(hex);
  InitializeCell(qhex);
  InitializeCell(triqhex);
  InitializeCell(biqqhex);
  ret += CompareCellEdges(hex, qhex);
  ret += CompareCellFaces(hex, qhex);
  ret += CompareCellEdges(hex, triqhex);
  ret += CompareCellFaces(hex, triqhex);
  ret += CompareCellEdges(hex, biqqhex);
  ret += CompareCellFaces(hex, biqqhex);

  // Pyramid
  auto pyr = vtkSmartPointer<vtkPyramid>::New();
  auto qpyr = vtkSmartPointer<vtkQuadraticPyramid>::New();
  auto tqpyr = vtkSmartPointer<vtkTriQuadraticPyramid>::New();

  InitializeCell(pyr);
  InitializeCell(qpyr);
  InitializeCell(tqpyr);
  ret += CompareCellEdges(pyr, qpyr);
  ret += CompareCellFaces(pyr, qpyr);
  ret += CompareCellFaces(pyr, tqpyr);
  ret += CompareCellFaces(pyr, tqpyr);

  // Wedge cells
  auto wedge = vtkSmartPointer<vtkWedge>::New();
  auto qwedge = vtkSmartPointer<vtkQuadraticWedge>::New();
  auto biqwedge = vtkSmartPointer<vtkBiQuadraticQuadraticWedge>::New();

  InitializeCell(wedge);
  InitializeCell(qwedge);
  InitializeCell(biqwedge);
  ret += CompareCellEdges(wedge, qwedge);
  ret += CompareCellFaces(wedge, qwedge);
  ret += CompareCellEdges(wedge, biqwedge);
  ret += CompareCellFaces(wedge, biqwedge);

  return ret;
}
