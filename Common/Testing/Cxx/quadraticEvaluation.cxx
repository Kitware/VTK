/*=========================================================================

  Program:   Visualization Toolkit
  Module:    quadraticEvaluation.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
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

void  ComputeDataValues(vtkPoints *pts, float *edgeValues)
{
  float x[3];
  int numPts = pts->GetNumberOfPoints();
  for (int i=0; i<numPts; i++)
    {
    pts->GetPoint(i,x);
    edgeValues[i] = 1.0 / (1.0 + x[0]); //simple linear function for now
    }
}

int Test(ostream& strm)
{
  // actual test
  float dist2;
  int subId;

  //-----------------------------------------------------------
  strm << "Test instantiation New() and MakeObject() Start" << endl;
  vtkQuadraticEdge *edge = vtkQuadraticEdge::New();
  vtkQuadraticEdge *edge2 = static_cast<vtkQuadraticEdge*>(edge->MakeObject());
  
  vtkQuadraticTriangle *tri = vtkQuadraticTriangle::New();
  vtkQuadraticTriangle *tri2 = static_cast<vtkQuadraticTriangle*>(tri->MakeObject());
  
  vtkQuadraticQuad *quad = vtkQuadraticQuad::New();
  vtkQuadraticQuad *quad2 = static_cast<vtkQuadraticQuad*>(quad->MakeObject());
  
  vtkQuadraticTetra *tetra = vtkQuadraticTetra::New();
  vtkQuadraticTetra *tetra2 = static_cast<vtkQuadraticTetra*>(tetra->MakeObject());
  
  vtkQuadraticHexahedron *hex = vtkQuadraticHexahedron::New();
  vtkQuadraticHexahedron *hex2 = static_cast<vtkQuadraticHexahedron*>(hex->MakeObject());

  edge2->Delete();
  tri2->Delete();
  quad2->Delete();
  tetra2->Delete();
  hex2->Delete();

  strm << "Test instantiation New() and MakeObject() End" << endl;
  

  //-------------------------------------------------------------
  strm << "Test vtkCell::EvaluatePosition Start" << endl;

  // vtkQuadraticEdge
  float edgePCoords[3], edgeWeights[3], edgePosition[3];
  float edgePoint[1][3] = {{0.25, 0.125, 0.0}};
  float edgeClosest[3];

  edge->GetPointIds()->SetId(0,0);
  edge->GetPointIds()->SetId(1,1);
  edge->GetPointIds()->SetId(2,2);

  edge->GetPoints()->SetPoint(0, 0, 0, 0);
  edge->GetPoints()->SetPoint(1, 1, 0, 0);
  edge->GetPoints()->SetPoint(2, 0.5, 0.25, 0);

  edge->EvaluatePosition(edgePoint[0], edgeClosest, subId, edgePCoords, 
                         dist2, edgeWeights);

  // vtkQuadraticTriangle
  float triPCoords[3], triWeights[6], triPosition[3];
  float triPoint[1][3] = {{0.5, 0.266667, 0.0}};
  float triClosest[3];

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
  float quadPCoords[3], quadWeights[8], quadPosition[3];
  float quadPoint[1][3] = {{0.25, 0.33, 0.0}};
  float quadClosest[3];
  
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
  float tetraPCoords[3], tetraWeights[10], tetraPosition[3];
  float tetraPoint[1][3] = {{0.5, 0.266667, 0.333333}};
  float tetraClosest[3];
  
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
  float hexPCoords[3], hexWeights[20], hexPosition[3];
  float hexPoint[1][3] = {{0.25, 0.33333, 0.666667}};
  float hexClosest[3];
  
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
  hexPCoords[0] = 0.25;
  hexPCoords[1] = 0.33;
  hexPCoords[2] = 0.75;
  hex->EvaluateLocation(subId, hexPCoords, hexPosition, hexWeights);
  
  strm << "Test vtkCell::EvaluateLocation End" << endl;

  //-------------------------------------------------------------
  strm << "Test vtkCell::CellDerivs Start" << endl;

  // vtkQuadraticEdge
  float edgeValues[3], edgeDerivs[3];
  ComputeDataValues(edge->Points,edgeValues);
  edge->Derivatives(subId, edgePCoords, edgeValues, 1, edgeDerivs);
  
  // vtkQuadraticTriangle
  float triValues[6], triDerivs[3];
  ComputeDataValues(tri->Points,triValues);
  tri->Derivatives(subId, triPCoords, triValues, 1, triDerivs);
  
  // vtkQuadraticQuad
  float quadValues[8], quadDerivs[3];
  ComputeDataValues(quad->Points,quadValues);
  quad->Derivatives(subId, quadPCoords, quadValues, 1, quadDerivs);
  
  // vtkQuadraticTetra
  float tetraValues[10], tetraDerivs[3];
  ComputeDataValues(tetra->Points,tetraValues);
  tetra->Derivatives(subId, tetraPCoords, tetraValues, 1, tetraDerivs);
  
  // vtkQuadraticHexahedron
  float hexValues[20], hexDerivs[3];
  ComputeDataValues(hex->Points,hexValues);
  hex->Derivatives(subId, hexPCoords, hexValues, 1, hexDerivs);
  
  strm << "Test vtkCell::CellDerivs End" << endl;

  edge->Delete();
  tri->Delete();
  quad->Delete();
  tetra->Delete();
  hex->Delete();

  return 0;
}

int main()
{
  vtkDebugLeaks::PromptUserOff();

  ostrstream vtkmsg; 
  return Test(vtkmsg);
} 
