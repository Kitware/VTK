/*=========================================================================

  Program:   Visualization Toolkit
  Module:    quadraticEvaluation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME
// .SECTION Description
// This program tests quadratic cell EvaluatePosition() and EvaluateLocation()
// methods.

#include "vtkDebugLeaks.h"

#include "vtkIdList.h"
#include "vtkPoints.h"
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticPyramid.h"
#include "vtkQuadraticQuad.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticTriangle.h"
#include "vtkQuadraticWedge.h"
#include "vtkSmartPointer.h"

#include <cmath>
#include <sstream>

// New quadratic cells
#include "vtkBiQuadraticQuad.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkCubicLine.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkTriQuadraticPyramid.h"

static void ComputeDataValues(vtkPoints* pts, double* edgeValues);

void ComputeDataValues(vtkPoints* pts, double* edgeValues)
{
  double x[3];
  int numPts = pts->GetNumberOfPoints();
  for (int i = 0; i < numPts; i++)
  {
    pts->GetPoint(i, x);
    double dem = (1.0 + x[0]);
    if (fabs(dem) < 1.e-08)
    {
      edgeValues[i] = 0;
    }
    else
    {
      edgeValues[i] = 1.0 / dem; // simple linear function for now
    }
  }
}

int TestQE(ostream& strm)
{
  // actual test
  double dist2;
  int subId;
  double* paramcoor;

  //-----------------------------------------------------------
  strm << "Test instantiation New() and NewInstance() Start" << endl;
  auto edge = vtkSmartPointer<vtkQuadraticEdge>::New();
  auto edge2 = vtk::TakeSmartPointer(edge->NewInstance());

  auto tri = vtkSmartPointer<vtkQuadraticTriangle>::New();
  auto tri2 = vtk::TakeSmartPointer(tri->NewInstance());

  auto quad = vtkSmartPointer<vtkQuadraticQuad>::New();
  auto quad2 = vtk::TakeSmartPointer(quad->NewInstance());

  auto tetra = vtkSmartPointer<vtkQuadraticTetra>::New();
  auto tetra2 = vtk::TakeSmartPointer(tetra->NewInstance());

  auto hex = vtkSmartPointer<vtkQuadraticHexahedron>::New();
  auto hex2 = vtk::TakeSmartPointer(hex->NewInstance());

  auto wedge = vtkSmartPointer<vtkQuadraticWedge>::New();
  auto wedge2 = vtk::TakeSmartPointer(wedge->NewInstance());

  auto pyra = vtkSmartPointer<vtkQuadraticPyramid>::New();
  auto pyra2 = vtk::TakeSmartPointer(pyra->NewInstance());

  // New quadratic cells

  auto quadlin = vtkSmartPointer<vtkQuadraticLinearQuad>::New();
  auto quadlin2 = vtk::TakeSmartPointer(quadlin->NewInstance());

  auto biquad = vtkSmartPointer<vtkBiQuadraticQuad>::New();
  auto biquad2 = vtk::TakeSmartPointer(biquad->NewInstance());

  auto wedgelin = vtkSmartPointer<vtkQuadraticLinearWedge>::New();
  auto wedgelin2 = vtk::TakeSmartPointer(wedgelin->NewInstance());

  auto biwedge = vtkSmartPointer<vtkBiQuadraticQuadraticWedge>::New();
  auto biwedge2 = vtk::TakeSmartPointer(biwedge->NewInstance());

  auto bihex = vtkSmartPointer<vtkBiQuadraticQuadraticHexahedron>::New();
  auto bihex2 = vtk::TakeSmartPointer(bihex->NewInstance());

  auto trihex = vtkSmartPointer<vtkTriQuadraticHexahedron>::New();
  auto trihex2 = vtk::TakeSmartPointer(trihex->NewInstance());

  auto tqPyra = vtkSmartPointer<vtkTriQuadraticPyramid>::New();
  auto tqPyra2 = vtk::TakeSmartPointer(tqPyra->NewInstance());

  auto bitri = vtkSmartPointer<vtkBiQuadraticTriangle>::New();
  auto bitri2 = vtk::TakeSmartPointer(bitri->NewInstance());

  auto culine = vtkSmartPointer<vtkCubicLine>::New();
  auto culine2 = vtk::TakeSmartPointer(culine->NewInstance());

  strm << "Test instantiation New() and NewInstance() End" << endl;

  //-------------------------------------------------------------
  strm << "Test vtkCell::EvaluatePosition Start" << endl;

  // vtkQuadraticEdge
  double edgePCoords[3], edgeWeights[3], edgePosition[3];
  double edgePoint[1][3] = { { 0.25, 0.125, 0.0 } };
  double edgeClosest[3];

  edge->GetPointIds()->SetId(0, 0);
  edge->GetPointIds()->SetId(1, 1);
  edge->GetPointIds()->SetId(2, 2);

  edge->GetPoints()->SetPoint(0, 0, 0, 0);
  edge->GetPoints()->SetPoint(1, 1, 0, .5);
  edge->GetPoints()->SetPoint(2, 0.5, 0.25, .2);

  edge->EvaluatePosition(edgePoint[0], edgeClosest, subId, edgePCoords, dist2, edgeWeights);

  // vtkQuadraticTriangle
  double triPCoords[3], triWeights[6], triPosition[3];
  double triPoint[1][3] = { { 0.5, 0.266667, 0.0 } };
  double triClosest[3];

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

  tri->EvaluatePosition(triPoint[0], triClosest, subId, triPCoords, dist2, triWeights);

  // vtkQuadraticQuad
  double quadPCoords[3], quadWeights[8], quadPosition[3];
  double quadPoint[1][3] = { { 0.25, 0.33, 0.0 } };
  double quadClosest[3];

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

  quad->EvaluatePosition(quadPoint[0], quadClosest, subId, quadPCoords, dist2, quadWeights);

  // vtkQuadraticTetra
  double tetraPCoords[3], tetraWeights[10], tetraPosition[3];
  double tetraPoint[1][3] = { { 0.5, 0.266667, 0.333333 } };
  double tetraClosest[3];

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

  tetra->EvaluatePosition(tetraPoint[0], tetraClosest, subId, tetraPCoords, dist2, tetraWeights);

  // vtkQuadraticHexahedron
  double hexPCoords[3], hexWeights[20], hexPosition[3];
  double hexPoint[1][3] = { { 0.25, 0.33333, 0.666667 } };
  double hexClosest[3];

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

  hex->EvaluatePosition(hexPoint[0], hexClosest, subId, hexPCoords, dist2, hexWeights);

  // vtkQuadraticWedge
  double wedgePCoords[3], wedgeWeights[20], wedgePosition[3];
  double wedgePoint[1][3] = { { 0.25, 0.33333, 0.666667 } };
  double wedgeClosest[3];

  double* pcoords = wedge->GetParametricCoords();
  for (int i = 0; i < wedge->GetNumberOfPoints(); ++i)
  {
    wedge->GetPointIds()->SetId(i, i);
    wedge->GetPoints()->SetPoint(
      i, *(pcoords + 3 * i), *(pcoords + 3 * i + 1), *(pcoords + 3 * i + 2));
  }

  wedge->EvaluatePosition(wedgePoint[0], wedgeClosest, subId, wedgePCoords, dist2, wedgeWeights);

  // vtkQuadraticPyramid
  double pyraPCoords[3], pyraWeights[13], pyraPosition[3];
  double pyraPoint[1][3] = { { 0.25, 0.33333, 0.666667 } };
  double pyraClosest[3];

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

  pyra->EvaluatePosition(pyraPoint[0], pyraClosest, subId, pyraPCoords, dist2, pyraWeights);

  // New quadratic cells

  // vtkQuadraticLinearQuad
  double quadlinPCoords[3], quadlinWeights[6], quadlinPosition[3];
  double quadlinPoint[1][3] = { { 0.25, 0.33, 0.0 } };
  double quadlinClosest[3];
  paramcoor = quadlin->GetParametricCoords();

  for (int i = 0; i < quadlin->GetNumberOfPoints(); i++)
  {
    quadlin->GetPointIds()->SetId(i, i);
  }

  for (int i = 0; i < quadlin->GetNumberOfPoints(); i++)
  {
    quadlin->GetPoints()->SetPoint(i, paramcoor[i * 3], paramcoor[i * 3 + 1], paramcoor[i * 3 + 2]);
  }

  quadlin->EvaluatePosition(
    quadlinPoint[0], quadlinClosest, subId, quadlinPCoords, dist2, quadlinWeights);

  // vtkBiQuadraticQuad
  double biquadPCoords[3], biquadWeights[9], biquadPosition[3];
  double biquadPoint[1][3] = { { 0.25, 0.33, 0.0 } };
  double biquadClosest[3];
  paramcoor = biquad->GetParametricCoords();

  for (int i = 0; i < biquad->GetNumberOfPoints(); i++)
  {
    biquad->GetPointIds()->SetId(i, i);
  }

  for (int i = 0; i < biquad->GetNumberOfPoints(); i++)
  {
    biquad->GetPoints()->SetPoint(i, paramcoor[i * 3], paramcoor[i * 3 + 1], paramcoor[i * 3 + 2]);
  }

  biquad->EvaluatePosition(
    biquadPoint[0], biquadClosest, subId, biquadPCoords, dist2, biquadWeights);

  // vtkQuadraticLinearWedge
  double wedgelinPCoords[3], wedgelinWeights[12], wedgelinPosition[3];
  double wedgelinPoint[1][3] = { { 0.25, 0.33333, 0.666667 } };
  double wedgelinClosest[3];
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

  wedgelin->EvaluatePosition(
    wedgelinPoint[0], wedgelinClosest, subId, wedgelinPCoords, dist2, wedgelinWeights);

  // vtkBiQuadraticQuadraticWedge
  double biwedgePCoords[3], biwedgeWeights[18], biwedgePosition[3];
  double biwedgePoint[1][3] = { { 0.25, 0.33333, 0.666667 } };
  double biwedgeClosest[3];
  paramcoor = biwedge->GetParametricCoords();

  for (int i = 0; i < biwedge->GetNumberOfPoints(); i++)
  {
    biwedge->GetPointIds()->SetId(i, i);
  }

  for (int i = 0; i < biwedge->GetNumberOfPoints(); i++)
  {
    biwedge->GetPoints()->SetPoint(i, paramcoor[i * 3], paramcoor[i * 3 + 1], paramcoor[i * 3 + 2]);
  }

  biwedge->EvaluatePosition(
    biwedgePoint[0], biwedgeClosest, subId, biwedgePCoords, dist2, biwedgeWeights);

  // vtkBiQuadraticQuadraticHexahedron
  double bihexPCoords[3], bihexWeights[24], bihexPosition[3];
  double bihexPoint[1][3] = { { 0.25, 0.33333, 0.666667 } };
  double bihexClosest[3];
  paramcoor = bihex->GetParametricCoords();

  for (int i = 0; i < bihex->GetNumberOfPoints(); i++)
  {
    bihex->GetPointIds()->SetId(i, i);
  }

  for (int i = 0; i < bihex->GetNumberOfPoints(); i++)
  {
    bihex->GetPoints()->SetPoint(i, paramcoor[i * 3], paramcoor[i * 3 + 1], paramcoor[i * 3 + 2]);
  }

  bihex->EvaluatePosition(bihexPoint[0], bihexClosest, subId, bihexPCoords, dist2, bihexWeights);

  // vtkTriQuadraticHexahedron
  double trihexPCoords[3], trihexWeights[27], trihexPosition[3];
  double trihexPoint[1][3] = { { 0.25, 0.33333, 0.666667 } };
  double trihexClosest[3];
  paramcoor = trihex->GetParametricCoords();

  for (int i = 0; i < trihex->GetNumberOfPoints(); i++)
  {
    trihex->GetPointIds()->SetId(i, i);
  }

  for (int i = 0; i < trihex->GetNumberOfPoints(); i++)
  {
    trihex->GetPoints()->SetPoint(i, paramcoor[i * 3], paramcoor[i * 3 + 1], paramcoor[i * 3 + 2]);
  }

  trihex->EvaluatePosition(
    trihexPoint[0], trihexClosest, subId, trihexPCoords, dist2, trihexWeights);

  // vtkTriQuadraticPyramid
  double tqPyraPCoords[3], tqPyraWeights[19], tqPyraPosition[3];
  double tqPyraPoint[1][3] = { { 0.25, 0.33333, 0.666667 } };
  double tqPyraClosest[3];

  for (int i = 0; i < 19; ++i)
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

  tqPyra->EvaluatePosition(
    tqPyraPoint[0], tqPyraClosest, subId, tqPyraPCoords, dist2, tqPyraWeights);

  // vtkBiQuadraticTriangle
  double bitriPCoords[3], bitriWeights[14], bitriPosition[3];
  double bitriPoint[1][3] = { { 0.5, 0.266667, 0.0 } };
  double bitriClosest[3];

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

  bitri->EvaluatePosition(bitriPoint[0], bitriClosest, subId, bitriPCoords, dist2, bitriWeights);

  // vtkCubicLine

  double culinePCoords[3], culineWeights[4];
  double culinePoint[1][3] = { { 0.25, 0.125, 0.0 } };
  double culineClosest[3];

  for (int i = 0; i < culine->GetNumberOfPoints(); ++i)
  {
    culine->GetPointIds()->SetId(i, i);
  }

  culine->GetPoints()->SetPoint(0, 0, 0, 0);
  culine->GetPoints()->SetPoint(1, 1, 0, 0);
  culine->GetPoints()->SetPoint(2, (1.0 / 3.0), -0.1, 0);
  culine->GetPoints()->SetPoint(3, (1.0 / 3.0), 0.1, 0);

  culine->EvaluatePosition(
    culinePoint[0], culineClosest, subId, culinePCoords, dist2, culineWeights);

  strm << "Test vtkCell::EvaluatePosition End" << endl;

  //-------------------------------------------------------------
  strm << "Test vtkCell::EvaluateLocation Start" << endl;
  // vtkQuadraticEdge
  edge->EvaluateLocation(subId, edgePCoords, edgePosition, edgeWeights);

  // vtkQuadraticTriangle
  tri->EvaluateLocation(subId, triPCoords, triPosition, triWeights);

  // vtkQuadraticQuad
  quad->EvaluateLocation(subId, quadPCoords, quadPosition, quadWeights);

  // vtkQuadraticTetra
  tetra->EvaluateLocation(subId, tetraPCoords, tetraPosition, tetraWeights);

  // vtkQuadraticHexahedron
  hex->EvaluateLocation(subId, hexPCoords, hexPosition, hexWeights);

  // vtkQuadraticWedge
  wedge->EvaluateLocation(subId, wedgePCoords, wedgePosition, wedgeWeights);

  // vtkQuadraticPyramid
  pyra->EvaluateLocation(subId, pyraPCoords, pyraPosition, pyraWeights);

  // New quadratic cells

  // vtkQuadraticLinearQuad
  quadlin->EvaluateLocation(subId, quadlinPCoords, quadlinPosition, quadlinWeights);

  // vtkBiQuadraticQuad
  biquad->EvaluateLocation(subId, biquadPCoords, biquadPosition, biquadWeights);

  // vtkQuadraticLinearWedge
  wedgelin->EvaluateLocation(subId, wedgelinPCoords, wedgelinPosition, wedgelinWeights);

  // vtkBiQuadraticQuadraticWedge
  biwedge->EvaluateLocation(subId, biwedgePCoords, biwedgePosition, biwedgeWeights);

  // vtkQuadraticLinearQuad
  bihex->EvaluateLocation(subId, bihexPCoords, bihexPosition, bihexWeights);

  // vtkTriQuadraticHexahedron
  trihex->EvaluateLocation(subId, trihexPCoords, trihexPosition, trihexWeights);

  // vtkTriQuadraticPyramid
  tqPyra->EvaluateLocation(subId, tqPyraPCoords, tqPyraPosition, tqPyraWeights);

  // vtkBiQuadraticTriangle
  bitri->EvaluateLocation(subId, bitriPCoords, bitriPosition, bitriWeights);

  strm << "Test vtkCell::EvaluateLocation End" << endl;

  //-------------------------------------------------------------
  strm << "Test vtkCell::CellDerivs Start" << endl;

  // vtkQuadraticEdge - temporarily commented out
  // double edgeValues[3], edgeDerivs[3];
  // ComputeDataValues(edge->Points,edgeValues);
  // edge->Derivatives(subId, edgePCoords, edgeValues, 1, edgeDerivs);

  // vtkQuadraticTriangle
  double triValues[6], triDerivs[3];
  ComputeDataValues(tri->Points, triValues);
  tri->Derivatives(subId, triPCoords, triValues, 1, triDerivs);

  // vtkQuadraticQuad
  double quadValues[8], quadDerivs[3];
  ComputeDataValues(quad->Points, quadValues);
  quad->Derivatives(subId, quadPCoords, quadValues, 1, quadDerivs);

  // vtkQuadraticTetra
  double tetraValues[10], tetraDerivs[3];
  ComputeDataValues(tetra->Points, tetraValues);
  tetra->Derivatives(subId, tetraPCoords, tetraValues, 1, tetraDerivs);

  // vtkQuadraticHexahedron
  double hexValues[20], hexDerivs[3];
  ComputeDataValues(hex->Points, hexValues);
  hex->Derivatives(subId, hexPCoords, hexValues, 1, hexDerivs);

  // vtkQuadraticWedge
  double wedgeValues[15], wedgeDerivs[3];
  ComputeDataValues(wedge->Points, wedgeValues);
  wedge->Derivatives(subId, wedgePCoords, wedgeValues, 1, wedgeDerivs);

  // vtkQuadraticPyramid
  double pyraValues[13], pyraDerivs[3];
  ComputeDataValues(pyra->Points, pyraValues);
  pyra->Derivatives(subId, pyraPCoords, pyraValues, 1, pyraDerivs);

  // New quadratic cells

  // vtkQuadraticLinearQuad
  double quadlinValues[6], quadlinDerivs[3];
  ComputeDataValues(quadlin->Points, quadlinValues);
  quadlin->Derivatives(subId, quadlinPCoords, quadlinValues, 1, quadlinDerivs);

  // vtkBiQuadraticQuad
  double biquadValues[9], biquadDerivs[3];
  ComputeDataValues(biquad->Points, biquadValues);
  biquad->Derivatives(subId, biquadPCoords, biquadValues, 1, biquadDerivs);

  // vtkQuadraticLinearWedge
  double wedgelinValues[12], wedgelinDerivs[3];
  ComputeDataValues(wedgelin->Points, wedgelinValues);
  wedgelin->Derivatives(subId, wedgelinPCoords, wedgelinValues, 1, wedgelinDerivs);

  // vtkBiQuadraticQuadraticWedge
  double biwedgeValues[18], biwedgeDerivs[3];
  ComputeDataValues(biwedge->Points, biwedgeValues);
  biwedge->Derivatives(subId, biwedgePCoords, biwedgeValues, 1, biwedgeDerivs);

  // vtkBiQuadraticQuadraticHexahedron
  double bihexValues[24], bihexDerivs[3];
  ComputeDataValues(bihex->Points, bihexValues);
  bihex->Derivatives(subId, bihexPCoords, bihexValues, 1, bihexDerivs);

  // vtkTriQuadraticHexahedron
  double trihexValues[27], trihexDerivs[3];
  ComputeDataValues(trihex->Points, trihexValues);
  trihex->Derivatives(subId, trihexPCoords, trihexValues, 1, trihexDerivs);

  // vtkTriQuadraticPyramid
  double tqPyraValues[19], tqPyraDerivs[3];
  ComputeDataValues(tqPyra->Points, tqPyraValues);
  tqPyra->Derivatives(subId, tqPyraPCoords, tqPyraValues, 1, tqPyraDerivs);

  // vtkBiQuadraticTriangle
  double bitriValues[7], bitriDerivs[3];
  ComputeDataValues(bitri->Points, bitriValues);
  bitri->Derivatives(subId, bitriPCoords, bitriValues, 1, bitriDerivs);

  strm << "Test vtkCell::CellDerivs End" << endl;

  return 0;
}

int quadraticEvaluation(int, char*[])
{
  std::ostringstream vtkmsg_with_warning_C4701;
  return TestQE(vtkmsg_with_warning_C4701);
}
