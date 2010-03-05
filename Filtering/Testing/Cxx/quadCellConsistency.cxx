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

#include "vtkLine.h"
#include "vtkQuadraticEdge.h"
#include "vtkCubicLine.h"
#include "vtkTriangle.h"
#include "vtkQuadraticTriangle.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkQuad.h"
#include "vtkQuadraticQuad.h"
#include "vtkBiQuadraticQuad.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkTetra.h"
#include "vtkQuadraticTetra.h"
#include "vtkHexahedron.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkPyramid.h"
#include "vtkQuadraticPyramid.h"
#include "vtkWedge.h"
#include "vtkQuadraticWedge.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkQuadraticLinearWedge.h"

void InitializeCell(vtkCell *cell)
{
  // Default initialize the cell ids to 0,1,2 ... n
  int n = cell->GetNumberOfPoints();
  for(int i=0; i<n; i++)
    {
    cell->GetPointIds()->SetId(i, i);
    }
}

// Check that corner points id match quad ones for each edges
int CompareCellEdges(vtkCell *linear, vtkCell *quadratic)
{
  int dif;
  int sum = 0;
  int nEdges = linear->GetNumberOfEdges();
  for(int edge = 0; edge < nEdges; edge++)
    {
    vtkCell *lEdge = linear->GetEdge(edge);
    vtkCell *qEdge = quadratic->GetEdge(edge);

    int n = lEdge->GetNumberOfPoints();
    // Check that the points of the linear cell match the one from the quadratic one
    for( int i=0; i<n; i++)
      {
      dif = lEdge->GetPointIds()->GetId(i) - qEdge->GetPointIds()->GetId(i);
      sum += dif;
      }
    }
  return sum;
}

// Check that corner points id match quad ones for each faces
int CompareCellFaces(vtkCell *linear, vtkCell *quadratic)
{
  int dif;
  int sum = 0;
  int nFaces = linear->GetNumberOfFaces();
  for(int face = 0; face < nFaces; face++)
    {
    vtkCell *lFace = linear->GetFace(face);
    vtkCell *qFace = quadratic->GetFace(face);

    int n = lFace->GetNumberOfPoints();
    // Check that linear Triangle match quad Tri
    if( lFace->GetCellType() == VTK_TRIANGLE )
      sum += (qFace->GetCellType() != VTK_QUADRATIC_TRIANGLE && qFace->GetCellType() != VTK_BIQUADRATIC_TRIANGLE);
    // Check that linear Quad match quad Quad
    if( lFace->GetCellType() == VTK_QUAD &&
      (qFace->GetCellType() != VTK_QUADRATIC_QUAD && qFace->GetCellType() != VTK_BIQUADRATIC_QUAD &&
      qFace->GetCellType() != VTK_QUADRATIC_LINEAR_QUAD)  )
      sum++;
    // Check that the points of the linear cell match the one from the quadratic one
    for( int i=0; i<n; i++)
      {
      dif = lFace->GetPointIds()->GetId(i) - qFace->GetPointIds()->GetId(i);
      sum += dif;
      }
    }
  return sum;
}

int quadCellConsistency(int, char *[])
{
  int ret = 0;
  // Line / vtkQuadraticEdge / CubicLine:
  vtkLine *edge = vtkLine::New();
  vtkQuadraticEdge *qedge = vtkQuadraticEdge::New();
  vtkCubicLine *culine = vtkCubicLine::New();

  InitializeCell(edge);
  InitializeCell(qedge);
  ret += CompareCellEdges(edge, qedge);
  ret += CompareCellFaces(edge, qedge);

  
  qedge->Delete();
  
  InitializeCell(culine);
  ret += CompareCellEdges(edge, culine);
  ret += CompareCellFaces(edge, culine);

  edge->Delete();
  culine->Delete();

  // Triangles:
  vtkTriangle *tri = vtkTriangle::New();
  vtkQuadraticTriangle *qtri = vtkQuadraticTriangle::New();
  vtkBiQuadraticTriangle *bitri = vtkBiQuadraticTriangle::New();

  InitializeCell(tri);
  InitializeCell(qtri);
  ret += CompareCellEdges(tri, qtri);
  ret += CompareCellFaces(tri, qtri);


  qtri->Delete();
  

  InitializeCell(bitri);
  ret += CompareCellEdges(tri, bitri);
  ret += CompareCellFaces(tri, bitri);

  tri->Delete();
  bitri->Delete();


  // Quad
  vtkQuad *quad = vtkQuad::New();
  vtkQuadraticQuad *qquad = vtkQuadraticQuad::New();
  vtkBiQuadraticQuad *biqquad = vtkBiQuadraticQuad::New();
  vtkQuadraticLinearQuad *qlquad = vtkQuadraticLinearQuad::New();

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

  quad->Delete();
  qquad->Delete();
  biqquad->Delete();
  qlquad->Delete();

  // Tetra
  vtkTetra *tetra = vtkTetra::New();
  vtkQuadraticTetra *qtetra = vtkQuadraticTetra::New();

  InitializeCell(tetra);
  InitializeCell(qtetra);
  ret += CompareCellEdges(tetra, qtetra);
  ret += CompareCellFaces(tetra, qtetra);

  tetra->Delete();
  qtetra->Delete();

  // Hexhedron
  vtkHexahedron *hex = vtkHexahedron::New();
  vtkQuadraticHexahedron *qhex = vtkQuadraticHexahedron::New();
  vtkTriQuadraticHexahedron *triqhex = vtkTriQuadraticHexahedron::New();
  vtkBiQuadraticQuadraticHexahedron *biqqhex = vtkBiQuadraticQuadraticHexahedron::New();

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

  hex->Delete();
  qhex->Delete();
  triqhex->Delete();
  biqqhex->Delete();

  // Pyramid
  vtkPyramid *pyr= vtkPyramid::New();
  vtkQuadraticPyramid *qpyr = vtkQuadraticPyramid::New();

  InitializeCell(pyr);
  InitializeCell(qpyr);
  ret += CompareCellEdges(pyr, qpyr);
  ret += CompareCellFaces(pyr, qpyr);

  pyr->Delete();
  qpyr->Delete();

  // Wedge cells
  vtkWedge *wedge = vtkWedge::New();
  vtkQuadraticWedge *qwedge = vtkQuadraticWedge::New();
  vtkBiQuadraticQuadraticWedge *biqwedge = vtkBiQuadraticQuadraticWedge::New();

  InitializeCell(wedge);
  InitializeCell(qwedge);
  InitializeCell(biqwedge);
  ret += CompareCellEdges(wedge, qwedge);
  ret += CompareCellFaces(wedge, qwedge);
  ret += CompareCellEdges(wedge, biqwedge);
  ret += CompareCellFaces(wedge, biqwedge);

  wedge->Delete();
  qwedge->Delete();
  biqwedge->Delete();

  return ret;
}
