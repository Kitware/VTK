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
#include "vtkQuadraticEdge.h"
#include "vtkQuadraticTriangle.h"
#include "vtkQuadraticQuad.h"
#include "vtkQuadraticHexahedron.h"
#include "vtkQuadraticTetra.h"
#include "vtkQuadraticWedge.h"
#include "vtkQuadraticPyramid.h"
#include "vtkPoints.h"

#include <vtksys/ios/sstream>

// New quadratic cells
#include "vtkBiQuadraticQuad.h"
#include "vtkQuadraticLinearQuad.h"
#include "vtkQuadraticLinearWedge.h"
#include "vtkBiQuadraticQuadraticWedge.h"
#include "vtkTriQuadraticHexahedron.h"
#include "vtkBiQuadraticQuadraticHexahedron.h"
#include "vtkBiQuadraticTriangle.h"
#include "vtkCubicLine.h"

static void  ComputeDataValues(vtkPoints *pts, double *edgeValues);

void  ComputeDataValues(vtkPoints *pts, double *edgeValues)
{
  double x[3];
  int numPts = pts->GetNumberOfPoints();
  for (int i=0; i<numPts; i++)
    {
    pts->GetPoint(i,x);
    double dem = (1.0 + x[0]);
    if(fabs(dem) < 1.e-08)
      {
      edgeValues[i] = 0;
      }
    else
      {
      edgeValues[i] = 1.0 / dem; //simple linear function for now
      }
    }
}

int TestQE(ostream& strm)
{
  // actual test
  double dist2;
  int subId;
  int i;
  double *paramcoor;

  //-----------------------------------------------------------
  strm << "Test instantiation New() and NewInstance() Start" << endl;
  vtkQuadraticEdge *edge = vtkQuadraticEdge::New();
  vtkQuadraticEdge *edge2 = edge->NewInstance();

  vtkQuadraticTriangle *tri = vtkQuadraticTriangle::New();
  vtkQuadraticTriangle *tri2 = tri->NewInstance();

  vtkQuadraticQuad *quad = vtkQuadraticQuad::New();
  vtkQuadraticQuad *quad2 = quad->NewInstance();

  vtkQuadraticTetra *tetra = vtkQuadraticTetra::New();
  vtkQuadraticTetra *tetra2 = tetra->NewInstance();

  vtkQuadraticHexahedron *hex = vtkQuadraticHexahedron::New();
  vtkQuadraticHexahedron *hex2 = hex->NewInstance();

  vtkQuadraticWedge *wedge = vtkQuadraticWedge::New();
  vtkQuadraticWedge *wedge2 = wedge->NewInstance();

  vtkQuadraticPyramid *pyra = vtkQuadraticPyramid::New();
  vtkQuadraticHexahedron *pyra2 = hex->NewInstance();

  // New quadratic cells

  vtkQuadraticLinearQuad *quadlin = vtkQuadraticLinearQuad::New();
  vtkQuadraticLinearQuad *quadlin2 = quadlin->NewInstance();

  vtkBiQuadraticQuad *biquad = vtkBiQuadraticQuad::New();
  vtkBiQuadraticQuad *biquad2 = biquad->NewInstance();

  vtkQuadraticLinearWedge *wedgelin = vtkQuadraticLinearWedge::New();
  vtkQuadraticLinearWedge *wedgelin2 = wedgelin->NewInstance();

  vtkBiQuadraticQuadraticWedge *biwedge = vtkBiQuadraticQuadraticWedge::New();
  vtkBiQuadraticQuadraticWedge *biwedge2 = biwedge->NewInstance();


  vtkTriQuadraticHexahedron *trihex = vtkTriQuadraticHexahedron::New();
  vtkTriQuadraticHexahedron *trihex2 = trihex->NewInstance();


  vtkBiQuadraticQuadraticHexahedron *bihex = vtkBiQuadraticQuadraticHexahedron::New();
  vtkBiQuadraticQuadraticHexahedron *bihex2 = bihex->NewInstance();

  vtkBiQuadraticTriangle *bitri = vtkBiQuadraticTriangle::New();
  vtkBiQuadraticTriangle *bitri2 = bitri->NewInstance();

  vtkCubicLine *culine = vtkCubicLine::New();
  vtkCubicLine *culine2 = culine->NewInstance();


  edge2->Delete();
  tri2->Delete();
  quad2->Delete();
  quadlin2->Delete();
  biquad2->Delete();
  tetra2->Delete();
  hex2->Delete();
  trihex2->Delete();
  bihex2->Delete();
  wedge2->Delete();
  wedgelin2->Delete();
  biwedge2->Delete();
  pyra2->Delete();
  bitri2->Delete();
  culine2->Delete();

  strm << "Test instantiation New() and NewInstance() End" << endl;


  //-------------------------------------------------------------
  strm << "Test vtkCell::EvaluatePosition Start" << endl;

  // vtkQuadraticEdge
  double edgePCoords[3], edgeWeights[3], edgePosition[3];
  double edgePoint[1][3] = {{0.25, 0.125, 0.0}};
  double edgeClosest[3];

  edge->GetPointIds()->SetId(0,0);
  edge->GetPointIds()->SetId(1,1);
  edge->GetPointIds()->SetId(2,2);

  edge->GetPoints()->SetPoint(0, 0, 0, 0);
  edge->GetPoints()->SetPoint(1, 1, 0, .5);
  edge->GetPoints()->SetPoint(2, 0.5, 0.25, .2);

  edge->EvaluatePosition(edgePoint[0], edgeClosest, subId, edgePCoords,
                         dist2, edgeWeights);

  // vtkQuadraticTriangle
  double triPCoords[3], triWeights[6], triPosition[3];
  double triPoint[1][3] = {{0.5, 0.266667, 0.0}};
  double triClosest[3];

  tri->GetPointIds()->SetId(0,0);
  tri->GetPointIds()->SetId(1,1);
  tri->GetPointIds()->SetId(2,2);
  tri->GetPointIds()->SetId(3,3);
  tri->GetPointIds()->SetId(4,4);
  tri->GetPointIds()->SetId(5,5);

  tri->GetPoints()->SetPoint(0, 0, 0, 0);
  tri->GetPoints()->SetPoint(1, 1, 0, 0);
  tri->GetPoints()->SetPoint(2, 0.5, 0.8, 0);
  tri->GetPoints()->SetPoint(3, 0.5, 0.0, 0);
  tri->GetPoints()->SetPoint(4, 0.75, 0.4, 0);
  tri->GetPoints()->SetPoint(5, 0.25, 0.4, 0);

  tri->EvaluatePosition(triPoint[0], triClosest, subId, triPCoords,
                        dist2, triWeights);


  // vtkQuadraticQuad
  double quadPCoords[3], quadWeights[8], quadPosition[3];
  double quadPoint[1][3] = {{0.25, 0.33, 0.0}};
  double quadClosest[3];

  quad->GetPointIds()->SetId(0,0);
  quad->GetPointIds()->SetId(1,1);
  quad->GetPointIds()->SetId(2,2);
  quad->GetPointIds()->SetId(3,3);
  quad->GetPointIds()->SetId(4,4);
  quad->GetPointIds()->SetId(5,5);
  quad->GetPointIds()->SetId(6,6);
  quad->GetPointIds()->SetId(7,7);

  quad->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  quad->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  quad->GetPoints()->SetPoint(2, 1.0, 1.0, 0.0);
  quad->GetPoints()->SetPoint(3, 0.0, 1.0, 0.0);
  quad->GetPoints()->SetPoint(4, 0.5, 0.0, 0.0);
  quad->GetPoints()->SetPoint(5, 1.0, 0.5, 0.0);
  quad->GetPoints()->SetPoint(6, 0.5, 1.0, 0.0);
  quad->GetPoints()->SetPoint(7, 0.0, 0.5, 0.0);

  quad->EvaluatePosition(quadPoint[0], quadClosest, subId, quadPCoords,
                         dist2, quadWeights);

  // vtkQuadraticTetra
  double tetraPCoords[3], tetraWeights[10], tetraPosition[3];
  double tetraPoint[1][3] = {{0.5, 0.266667, 0.333333}};
  double tetraClosest[3];

  tetra->GetPointIds()->SetId(0,0);
  tetra->GetPointIds()->SetId(1,1);
  tetra->GetPointIds()->SetId(2,2);
  tetra->GetPointIds()->SetId(3,3);
  tetra->GetPointIds()->SetId(4,4);
  tetra->GetPointIds()->SetId(5,5);
  tetra->GetPointIds()->SetId(6,6);
  tetra->GetPointIds()->SetId(7,7);
  tetra->GetPointIds()->SetId(8,8);
  tetra->GetPointIds()->SetId(9,9);

  tetra->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  tetra->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  tetra->GetPoints()->SetPoint(2, 0.5, 0.8, 0.0);
  tetra->GetPoints()->SetPoint(3, 0.5, 0.4, 1.0);
  tetra->GetPoints()->SetPoint(4, 0.5, 0.0,  0.0);
  tetra->GetPoints()->SetPoint(5, 0.75, 0.4, 0.0);
  tetra->GetPoints()->SetPoint(6, 0.25, 0.4, 0.0);
  tetra->GetPoints()->SetPoint(7, 0.25, 0.2, 0.5);
  tetra->GetPoints()->SetPoint(8, 0.75, 0.2, 0.5);
  tetra->GetPoints()->SetPoint(9, 0.50, 0.6, 0.5);

  tetra->EvaluatePosition(tetraPoint[0], tetraClosest, subId, tetraPCoords,
                         dist2, tetraWeights);


  // vtkQuadraticHexahedron
  double hexPCoords[3], hexWeights[20], hexPosition[3];
  double hexPoint[1][3] = {{0.25, 0.33333, 0.666667}};
  double hexClosest[3];

  hex->GetPointIds()->SetId(0,0);
  hex->GetPointIds()->SetId(1,1);
  hex->GetPointIds()->SetId(2,2);
  hex->GetPointIds()->SetId(3,3);
  hex->GetPointIds()->SetId(4,4);
  hex->GetPointIds()->SetId(5,5);
  hex->GetPointIds()->SetId(6,6);
  hex->GetPointIds()->SetId(7,7);
  hex->GetPointIds()->SetId(8,8);
  hex->GetPointIds()->SetId(9,9);
  hex->GetPointIds()->SetId(10,10);
  hex->GetPointIds()->SetId(11,11);
  hex->GetPointIds()->SetId(12,12);
  hex->GetPointIds()->SetId(13,13);
  hex->GetPointIds()->SetId(14,14);
  hex->GetPointIds()->SetId(15,15);
  hex->GetPointIds()->SetId(16,16);
  hex->GetPointIds()->SetId(17,17);
  hex->GetPointIds()->SetId(18,18);
  hex->GetPointIds()->SetId(19,19);

  hex->GetPoints()->SetPoint( 0, 0, 0, 0  );
  hex->GetPoints()->SetPoint( 1, 1, 0, 0  );
  hex->GetPoints()->SetPoint( 2, 1, 1, 0  );
  hex->GetPoints()->SetPoint( 3, 0, 1, 0  );
  hex->GetPoints()->SetPoint( 4, 0, 0, 1  );
  hex->GetPoints()->SetPoint( 5, 1, 0, 1  );
  hex->GetPoints()->SetPoint( 6, 1, 1, 1  );
  hex->GetPoints()->SetPoint( 7, 0, 1, 1  );
  hex->GetPoints()->SetPoint( 8, 0.5, 0, 0);
  hex->GetPoints()->SetPoint( 9, 1, 0.5, 0);
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

  hex->EvaluatePosition(hexPoint[0], hexClosest, subId, hexPCoords,
                         dist2, hexWeights);

  // vtkQuadraticWedge
  double wedgePCoords[3], wedgeWeights[20], wedgePosition[3];
  double wedgePoint[1][3] = {{0.25, 0.33333, 0.666667}};
  double wedgeClosest[3];

  wedge->GetPointIds()->SetId(0,0);
  wedge->GetPointIds()->SetId(1,1);
  wedge->GetPointIds()->SetId(2,2);
  wedge->GetPointIds()->SetId(3,3);
  wedge->GetPointIds()->SetId(4,4);
  wedge->GetPointIds()->SetId(5,5);
  wedge->GetPointIds()->SetId(6,6);
  wedge->GetPointIds()->SetId(7,7);
  wedge->GetPointIds()->SetId(8,8);
  wedge->GetPointIds()->SetId(9,9);
  wedge->GetPointIds()->SetId(10,10);
  wedge->GetPointIds()->SetId(11,11);
  wedge->GetPointIds()->SetId(12,12);
  wedge->GetPointIds()->SetId(13,13);
  wedge->GetPointIds()->SetId(14,14);

  wedge->GetPoints()->SetPoint( 0, 0, 0, 0  );
  wedge->GetPoints()->SetPoint( 1, 1, 0, 0  );
  wedge->GetPoints()->SetPoint( 2, 0, 1, 0  );
  wedge->GetPoints()->SetPoint( 3, 0, 0, 1  );
  wedge->GetPoints()->SetPoint( 4, 1, 0, 1  );
  wedge->GetPoints()->SetPoint( 5, 0, 1, 1  );
  wedge->GetPoints()->SetPoint( 6, 0.5, 0, 0  );
  wedge->GetPoints()->SetPoint( 7, 0.5, 0.5, 0  );
  wedge->GetPoints()->SetPoint( 8, 0, 0.5, 0);
  wedge->GetPoints()->SetPoint( 9, 0.5, 0, 1);
  wedge->GetPoints()->SetPoint(10, 0.5, 0.5, 1);
  wedge->GetPoints()->SetPoint(11, 0, 0.5, 1);
  wedge->GetPoints()->SetPoint(12, 0, 0, 0.5);
  wedge->GetPoints()->SetPoint(13, 1, 0, 0.5);
  wedge->GetPoints()->SetPoint(14, 0, 1, 0.5);

  wedge->EvaluatePosition(wedgePoint[0], wedgeClosest, subId, wedgePCoords,
                         dist2, wedgeWeights);

  // vtkQuadraticPyramid
  double pyraPCoords[3], pyraWeights[20], pyraPosition[3];
  double pyraPoint[1][3] = {{0.25, 0.33333, 0.666667}};
  double pyraClosest[3];

  pyra->GetPointIds()->SetId(0,0);
  pyra->GetPointIds()->SetId(1,1);
  pyra->GetPointIds()->SetId(2,2);
  pyra->GetPointIds()->SetId(3,3);
  pyra->GetPointIds()->SetId(4,4);
  pyra->GetPointIds()->SetId(5,5);
  pyra->GetPointIds()->SetId(6,6);
  pyra->GetPointIds()->SetId(7,7);
  pyra->GetPointIds()->SetId(8,8);
  pyra->GetPointIds()->SetId(9,9);
  pyra->GetPointIds()->SetId(10,10);
  pyra->GetPointIds()->SetId(11,11);
  pyra->GetPointIds()->SetId(12,12);

  pyra->GetPoints()->SetPoint( 0, 0, 0, 0  );
  pyra->GetPoints()->SetPoint( 1, 1, 0, 0  );
  pyra->GetPoints()->SetPoint( 2, 1, 1, 0  );
  pyra->GetPoints()->SetPoint( 3, 0, 1, 0  );
  pyra->GetPoints()->SetPoint( 4, 0, 0, 1  );
  pyra->GetPoints()->SetPoint( 5, 0.5, 0, 0  );
  pyra->GetPoints()->SetPoint( 6, 1, 0.5, 0  );
  pyra->GetPoints()->SetPoint( 7, 0.5, 1, 0  );
  pyra->GetPoints()->SetPoint( 8, 0, 0.5, 0  );
  pyra->GetPoints()->SetPoint( 9, 0, 0, 0.5  );
  pyra->GetPoints()->SetPoint(10, 0.5, 0, 0.5  );
  pyra->GetPoints()->SetPoint(11, 0.5, 0.5, 0.5  );
  pyra->GetPoints()->SetPoint(12, 0, 0.5, 0.5  );

  pyra->EvaluatePosition(pyraPoint[0], pyraClosest, subId, pyraPCoords,
                         dist2, pyraWeights);

  // New quadratic cells

  // vtkQuadraticLinearQuad
  double quadlinPCoords[3], quadlinWeights[6], quadlinPosition[3];
  double quadlinPoint[1][3] = {{0.25, 0.33, 0.0}};
  double quadlinClosest[3];
  paramcoor = quadlin->GetParametricCoords();

  for(i = 0; i < quadlin->GetNumberOfPoints(); i++)
    quadlin->GetPointIds()->SetId(i,i);

  for(i = 0; i < quadlin->GetNumberOfPoints(); i++)
    quadlin->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  quadlin->EvaluatePosition(quadlinPoint[0], quadlinClosest, subId, quadlinPCoords,
                         dist2, quadlinWeights);

  // vtkBiQuadraticQuad
  double biquadPCoords[3], biquadWeights[9], biquadPosition[3];
  double biquadPoint[1][3] = {{0.25, 0.33, 0.0}};
  double biquadClosest[3];
  paramcoor = biquad->GetParametricCoords();

  for(i = 0; i < biquad->GetNumberOfPoints(); i++)
    biquad->GetPointIds()->SetId(i,i);

  for(i = 0; i < biquad->GetNumberOfPoints(); i++)
    biquad->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  biquad->EvaluatePosition(biquadPoint[0], biquadClosest, subId, biquadPCoords,
                         dist2, biquadWeights);

  // vtkQuadraticLinearWedge
  double wedgelinPCoords[3], wedgelinWeights[12], wedgelinPosition[3];
  double wedgelinPoint[1][3] = {{0.25, 0.33333, 0.666667}};
  double wedgelinClosest[3];
  paramcoor = wedgelin->GetParametricCoords();

  for(i = 0; i < wedgelin->GetNumberOfPoints(); i++)
    wedgelin->GetPointIds()->SetId(i,i);

  for(i = 0; i < wedgelin->GetNumberOfPoints(); i++)
    wedgelin->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  wedgelin->EvaluatePosition(wedgelinPoint[0], wedgelinClosest, subId, wedgelinPCoords,
                         dist2, wedgelinWeights);

  // vtkBiQuadraticQuadraticWedge
  double biwedgePCoords[3], biwedgeWeights[18], biwedgePosition[3];
  double biwedgePoint[1][3] = {{0.25, 0.33333, 0.666667}};
  double biwedgeClosest[3];
  paramcoor = biwedge->GetParametricCoords();

  for(i = 0; i < biwedge->GetNumberOfPoints(); i++)
    biwedge->GetPointIds()->SetId(i,i);

  for(i = 0; i < biwedge->GetNumberOfPoints(); i++)
    biwedge->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  biwedge->EvaluatePosition(biwedgePoint[0], biwedgeClosest, subId, biwedgePCoords,
                         dist2, biwedgeWeights);

  // vtkBiQuadraticQuadraticHexahedron
  double bihexPCoords[3], bihexWeights[24], bihexPosition[3];
  double bihexPoint[1][3] = {{0.25, 0.33333, 0.666667}};
  double bihexClosest[3];
  paramcoor = bihex->GetParametricCoords();

  for(i = 0; i < bihex->GetNumberOfPoints(); i++)
    bihex->GetPointIds()->SetId(i,i);

  for(i = 0; i < bihex->GetNumberOfPoints(); i++)
    bihex->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  bihex->EvaluatePosition(bihexPoint[0], bihexClosest, subId, bihexPCoords,
                         dist2, bihexWeights);

  // vtkTriQuadraticHexahedron
  double trihexPCoords[3], trihexWeights[27], trihexPosition[3];
  double trihexPoint[1][3] = {{0.25, 0.33333, 0.666667}};
  double trihexClosest[3];
  paramcoor = trihex->GetParametricCoords();

  for(i = 0; i < trihex->GetNumberOfPoints(); i++)
    trihex->GetPointIds()->SetId(i,i);

  for(i = 0; i < trihex->GetNumberOfPoints(); i++)
    trihex->GetPoints()->SetPoint(i, paramcoor[i*3], paramcoor[i*3 + 1], paramcoor[i*3 + 2]);

  trihex->EvaluatePosition(trihexPoint[0], trihexClosest, subId, trihexPCoords,
                         dist2, trihexWeights);



  // vtkBiQuadraticTriangle
  double bitriPCoords[3], bitriWeights[14], bitriPosition[3];
  double bitriPoint[1][3] = {{0.5, 0.266667, 0.0}};
  double bitriClosest[3];

  bitri->GetPointIds()->SetId(0,0);
  bitri->GetPointIds()->SetId(1,1);
  bitri->GetPointIds()->SetId(2,2);
  bitri->GetPointIds()->SetId(3,3);
  bitri->GetPointIds()->SetId(4,4);
  bitri->GetPointIds()->SetId(5,5);

  bitri->GetPoints()->SetPoint(0, 0, 0, 0);
  bitri->GetPoints()->SetPoint(1, 1, 0, 0);
  bitri->GetPoints()->SetPoint(2, 0.5, 0.8, 0);
  bitri->GetPoints()->SetPoint(3, 0.5, 0.0, 0);
  bitri->GetPoints()->SetPoint(4, 0.75, 0.4, 0);
  bitri->GetPoints()->SetPoint(5, 0.25, 0.4, 0);
  bitri->GetPoints()->SetPoint(6, 0.45, 0.24, 0);

  bitri->EvaluatePosition(bitriPoint[0], bitriClosest, subId, bitriPCoords,
                        dist2, bitriWeights);


  // vtkCubicLine

  double culinePCoords[3], culineWeights[4];
  double culinePoint[1][3] = {{0.25, 0.125, 0.0}};
  double culineClosest[3];

  culine->GetPointIds()->SetId(0,0);
  culine->GetPointIds()->SetId(1,1);
  culine->GetPointIds()->SetId(2,2);
  culine->GetPointIds()->SetId(3,3);


  culine->GetPoints()->SetPoint(0, 0, 0, 0);
  culine->GetPoints()->SetPoint(1, 1, 0, 0);
  culine->GetPoints()->SetPoint(2, (1.0/3.0), -0.1, 0);
  culine->GetPoints()->SetPoint(3, (1.0/3.0), 0.1, 0);


  culine->EvaluatePosition(culinePoint[0], culineClosest, subId, culinePCoords,
                        dist2, culineWeights);


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

  // vtkBiQuadraticTriangle
  bitri->EvaluateLocation(subId, bitriPCoords, bitriPosition, bitriWeights);

  strm << "Test vtkCell::EvaluateLocation End" << endl;

  //-------------------------------------------------------------
  strm << "Test vtkCell::CellDerivs Start" << endl;

  // vtkQuadraticEdge - temporarily commented out
  //double edgeValues[3], edgeDerivs[3];
  //ComputeDataValues(edge->Points,edgeValues);
  //edge->Derivatives(subId, edgePCoords, edgeValues, 1, edgeDerivs);

  // vtkQuadraticTriangle
  double triValues[6], triDerivs[3];
  ComputeDataValues(tri->Points,triValues);
  tri->Derivatives(subId, triPCoords, triValues, 1, triDerivs);

  // vtkQuadraticQuad
  double quadValues[8], quadDerivs[3];
  ComputeDataValues(quad->Points,quadValues);
  quad->Derivatives(subId, quadPCoords, quadValues, 1, quadDerivs);

  // vtkQuadraticTetra
  double tetraValues[10], tetraDerivs[3];
  ComputeDataValues(tetra->Points,tetraValues);
  tetra->Derivatives(subId, tetraPCoords, tetraValues, 1, tetraDerivs);

  // vtkQuadraticHexahedron
  double hexValues[20], hexDerivs[3];
  ComputeDataValues(hex->Points,hexValues);
  hex->Derivatives(subId, hexPCoords, hexValues, 1, hexDerivs);

  // vtkQuadraticWedge
  double wedgeValues[15], wedgeDerivs[3];
  ComputeDataValues(wedge->Points,wedgeValues);
  wedge->Derivatives(subId, wedgePCoords, wedgeValues, 1, wedgeDerivs);

  // vtkQuadraticPyramid
  double pyraValues[13], pyraDerivs[3];
  ComputeDataValues(pyra->Points,pyraValues);
  pyra->Derivatives(subId, pyraPCoords, pyraValues, 1, pyraDerivs);

  // New quadratic cells

  // vtkQuadraticLinearQuad
  double quadlinValues[6], quadlinDerivs[3];
  ComputeDataValues(quadlin->Points,quadlinValues);
  quadlin->Derivatives(subId, quadlinPCoords, quadlinValues, 1, quadlinDerivs);

  // vtkBiQuadraticQuad
  double biquadValues[9], biquadDerivs[3];
  ComputeDataValues(biquad->Points,biquadValues);
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

  // vtkBiQuadraticTriangle
  double bitriValues[7], bitriDerivs[3];
  ComputeDataValues(bitri->Points,bitriValues);
  bitri->Derivatives(subId, bitriPCoords, bitriValues, 1, bitriDerivs);


  strm << "Test vtkCell::CellDerivs End" << endl;

  edge->Delete();
  tri->Delete();
  tetra->Delete();
  wedge->Delete();
  wedgelin->Delete();
  biwedge->Delete();
  pyra->Delete();
  quad->Delete();
  quadlin->Delete();
  biquad->Delete();
  hex->Delete();
  bihex->Delete();
  trihex->Delete();
  bitri->Delete();
  culine->Delete();

  return 0;
}

int quadraticEvaluation(int,char *[])
{
  vtksys_ios::ostringstream vtkmsg_with_warning_C4701;
  return TestQE(vtkmsg_with_warning_C4701);
}
