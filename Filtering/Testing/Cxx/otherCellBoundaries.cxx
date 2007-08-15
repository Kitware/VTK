/*=========================================================================

  Program:   Visualization Toolkit
  Module:    otherCellBoundaries.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME 
// .SECTION Description
// this program tests the CellBoundary method for each cell type

#include "vtkDebugLeaks.h"
#include "vtkHexahedron.h"
#include "vtkIdList.h"
#include "vtkLine.h"
#include "vtkPixel.h"
#include "vtkPoints.h"
#include "vtkPolyLine.h"
#include "vtkPolyVertex.h"
#include "vtkPolygon.h"
#include "vtkQuad.h"
#include "vtkTetra.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"
#include "vtkWedge.h"
#include "vtkPentagonalPrism.h"
#include "vtkHexagonalPrism.h"

#include <vtksys/ios/sstream>

int TestOCB(ostream& strm)
{
  // actual test
  vtkIdList *ids = vtkIdList::New();
  int i, j, k;
  strm << "Test vtkCell::CellBoundary Start" << endl;

  //Vertex
  vtkVertex *vertex = vtkVertex::New();
  double vertexCoords[1][1];

  vertex->GetPointIds()->SetId(0,0);
  
  vertexCoords[0][0] = 0.0;

  for (j = 0; j < 1; j++)
    {
    vertex->CellBoundary (0, vertexCoords[j], ids);
    strm << "vtkVertex \t(" << vertexCoords[j][0] << ") \t= ";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }
  
  //Poly Vertex
  vtkPolyVertex *polyVertex = vtkPolyVertex::New();
  double polyVertexCoords[1][1];

  polyVertex->GetPointIds()->SetNumberOfIds(2);
  polyVertex->GetPointIds()->SetId(0,0);
  polyVertex->GetPointIds()->SetId(1,1);
  
  polyVertexCoords[0][0] = 0.0;

  for (k = 0; k < polyVertex->GetPointIds()->GetNumberOfIds(); k++)
    {
    for (j = 0; j < 1; j++)
      {
      polyVertex->CellBoundary (k, polyVertexCoords[j], ids);
      strm << "vtkPolyVertex \t(" << polyVertexCoords[j][0] << ") \t= ";
      for (i = 0; i < ids->GetNumberOfIds(); i++)
        {
        strm << ids->GetId(i) << ", ";
        }
      strm << endl;
      }
    }
  
  //Line
  vtkLine *line = vtkLine::New();
  double lineCoords[2][1];

  line->GetPointIds()->SetId(0,0);
  line->GetPointIds()->SetId(1,1);
  
  lineCoords[0][0] = .25;
  lineCoords[1][0] = .75;

  for (j = 0; j < 2; j++)
    {
    line->CellBoundary (0, lineCoords[j], ids);
    strm << "vtkLine \t(" << lineCoords[j][0] << ") = \t";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }
  
  //Poly Line
  vtkPolyLine *polyLine = vtkPolyLine::New();
  double polyLineCoords[3][1];

  polyLine->GetPointIds()->SetNumberOfIds(3);
  polyLine->GetPointIds()->SetId(0,0);
  polyLine->GetPointIds()->SetId(1,1);
  polyLine->GetPointIds()->SetId(2,2);
  
  polyLineCoords[0][0] = .25;
  polyLineCoords[1][0] = .75;

  for (k = 0; k < polyLine->GetPointIds()->GetNumberOfIds() - 1; k++)
    {
    for (j = 0; j < 2; j++)
      {
      polyLine->CellBoundary (k, polyLineCoords[j], ids);
      strm << "vtkPolyLine \t(" << polyLineCoords[j][0] << ") = \t";
      for (i = 0; i < ids->GetNumberOfIds(); i++)
        {
        strm << ids->GetId(i) << ", ";
        }
      strm << endl;
      }
    }
  
  //Triangle
  vtkTriangle *triangle = vtkTriangle::New();
  double triangleCoords[3][2];

  triangle->GetPointIds()->SetId(0,0);
  triangle->GetPointIds()->SetId(1,1);
  triangle->GetPointIds()->SetId(2,2);
  
  triangleCoords[0][0] = .5; triangleCoords[0][1] = 0.1;
  triangleCoords[1][0] = .9; triangleCoords[1][1] = 0.9;
  triangleCoords[2][0] = .1; triangleCoords[2][1] = 0.5;

  for (j = 0; j < 3; j++)
    {
    triangle->CellBoundary (0, triangleCoords[j], ids);
    strm << "vtkTriangle \t(" << triangleCoords[j][0] << ", " << triangleCoords[j][1] << ") = \t";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }
  
  //Triangle Strip
  vtkTriangleStrip *triangleStrip = vtkTriangleStrip::New();
  double triangleStripCoords[3][2];

  triangleStrip->GetPointIds()->SetNumberOfIds(4);
  triangleStrip->GetPointIds()->SetId(0,0);
  triangleStrip->GetPointIds()->SetId(1,1);
  triangleStrip->GetPointIds()->SetId(2,2);
  triangleStrip->GetPointIds()->SetId(3,3);
  
  triangleStripCoords[0][0] = .5; triangleStripCoords[0][1] = 0.1;
  triangleStripCoords[1][0] = .9; triangleStripCoords[1][1] = 0.9;
  triangleStripCoords[2][0] = .1; triangleStripCoords[2][1] = 0.5;

  for (k = 0; k < triangleStrip->GetPointIds()->GetNumberOfIds() - 2; k++)
    {
    for (j = 0; j < 3; j++)
      {
      triangleStrip->CellBoundary (k, triangleStripCoords[j], ids);
      strm << "vtkTriangleStrip \t(" << triangleStripCoords[j][0] << ", " << triangleStripCoords[j][1] << ") = \t";
      for (i = 0; i < ids->GetNumberOfIds(); i++)
        {
        strm << ids->GetId(i) << ", ";
        }
      strm << endl;
      }
    }
  
  //Quad
  vtkQuad *quad = vtkQuad::New();
  double quadCoords[4][2];

  quad->GetPointIds()->SetId(0,0);
  quad->GetPointIds()->SetId(1,1);
  quad->GetPointIds()->SetId(2,2);
  quad->GetPointIds()->SetId(3,3);
  
  quadCoords[0][0] = .5; quadCoords[0][1] = 0.1;
  quadCoords[1][0] = .9; quadCoords[1][1] = 0.5;
  quadCoords[2][0] = .5; quadCoords[2][1] = 0.9;
  quadCoords[3][0] = .1; quadCoords[3][1] = 0.5;

  for (j = 0; j < 4; j++)
    {
    quad->CellBoundary (0, quadCoords[j], ids);
    strm << "vtkQuad \t(" << quadCoords[j][0] << ", " << quadCoords[j][1] << ") = \t";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }

  //Pixel
  vtkPixel *pixel = vtkPixel::New();
  double pixelCoords[4][2];

  pixel->GetPointIds()->SetId(0,0);
  pixel->GetPointIds()->SetId(1,1);
  pixel->GetPointIds()->SetId(2,2);
  pixel->GetPointIds()->SetId(3,3);
  
  pixelCoords[0][0] = .5; pixelCoords[0][1] = 0.1;
  pixelCoords[1][0] = .9; pixelCoords[1][1] = 0.5;
  pixelCoords[2][0] = .5; pixelCoords[2][1] = 0.9;
  pixelCoords[3][0] = .1; pixelCoords[3][1] = 0.5;

  for (j = 0; j < 4; j++)
    {
    pixel->CellBoundary (0, pixelCoords[j], ids);
    strm << "vtkPixel \t(" << pixelCoords[j][0] << ", " << pixelCoords[j][1] << ") = \t";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }
  
  //Polygon
  vtkPolygon *polygon = vtkPolygon::New();
  double polygonCoords[4][2];

  polygon->GetPointIds()->SetNumberOfIds(4);
  polygon->GetPointIds()->SetId(0,0);
  polygon->GetPointIds()->SetId(1,1);
  polygon->GetPointIds()->SetId(2,2);
  polygon->GetPointIds()->SetId(3,3);

  polygon->GetPoints()->SetNumberOfPoints(4);
  polygon->GetPoints()->SetPoint(0, 0.0, 0.0, 0.0);
  polygon->GetPoints()->SetPoint(1, 1.0, 0.0, 0.0);
  polygon->GetPoints()->SetPoint(2, 1.0, 1.0, 0.0);
  polygon->GetPoints()->SetPoint(3, 0.0, 1.0, 0.0);
  
  polygonCoords[0][0] = .5; polygonCoords[0][1] = 0.1;
  polygonCoords[1][0] = .9; polygonCoords[1][1] = 0.5;
  polygonCoords[2][0] = .5; polygonCoords[2][1] = 0.9;
  polygonCoords[3][0] = .1; polygonCoords[3][1] = 0.5;

  for (j = 0; j < 4; j++)
    {
    polygon->CellBoundary (0, polygonCoords[j], ids);
    strm << "vtkPolygon \t(" << polygonCoords[j][0] << ", " << polygonCoords[j][1] << ") = \t";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }
  
  //Tetra
  vtkTetra *tetra = vtkTetra::New();
  double tetraCoords[4][3];

  tetra->GetPointIds()->SetNumberOfIds(4);
  tetra->GetPointIds()->SetId(0,0);
  tetra->GetPointIds()->SetId(1,1);
  tetra->GetPointIds()->SetId(2,2);
  tetra->GetPointIds()->SetId(3,3);

  tetraCoords[0][0] = .1; tetraCoords[0][1] = 0.2; tetraCoords[0][2] = 0.2;
  tetraCoords[1][0] = .2; tetraCoords[1][1] = 0.1; tetraCoords[1][2] = 0.2;
  tetraCoords[2][0] = .2; tetraCoords[2][1] = 0.2; tetraCoords[2][2] = 0.1;
  tetraCoords[3][0] = .3; tetraCoords[3][1] = 0.3; tetraCoords[3][2] = 0.3;

  for (j = 0; j < 4; j++)
    {
    tetra->CellBoundary (0, tetraCoords[j], ids);
    strm << "vtkTetra \t(" << tetraCoords[j][0] << ", " << tetraCoords[j][1] << ", " << tetraCoords[j][2] << ") = \t";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }
  
  //Voxel
  vtkVoxel *voxel = vtkVoxel::New();
  double voxelCoords[6][3];

  voxel->GetPointIds()->SetNumberOfIds(8);
  voxel->GetPointIds()->SetId(0,0);
  voxel->GetPointIds()->SetId(1,1);
  voxel->GetPointIds()->SetId(2,2);
  voxel->GetPointIds()->SetId(3,3);
  voxel->GetPointIds()->SetId(4,4);
  voxel->GetPointIds()->SetId(5,5);
  voxel->GetPointIds()->SetId(6,6);
  voxel->GetPointIds()->SetId(7,7);

  voxelCoords[0][0] = .5; voxelCoords[0][1] = 0.5; voxelCoords[0][2] = 0.1;
  voxelCoords[1][0] = .9; voxelCoords[1][1] = 0.9; voxelCoords[1][2] = 0.5;
  voxelCoords[2][0] = .5; voxelCoords[2][1] = 0.1; voxelCoords[2][2] = 0.5;
  voxelCoords[3][0] = .5; voxelCoords[3][1] = 0.5; voxelCoords[3][2] = 0.9;
  voxelCoords[4][0] = .1; voxelCoords[4][1] = 0.5; voxelCoords[4][2] = 0.5;
  voxelCoords[5][0] = .5; voxelCoords[5][1] = 0.9; voxelCoords[5][2] = 0.5;

  for (j = 0; j < 6; j++)
    {
    voxel->CellBoundary (0, voxelCoords[j], ids);
    strm << "vtkVoxel \t(" << voxelCoords[j][0] << ", " << voxelCoords[j][1] << ", " << voxelCoords[j][2] << ") = \t";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }

  //Wedge
  vtkWedge *wedge = vtkWedge::New();
  double wedgeCoords[6][3];

  wedge->GetPointIds()->SetNumberOfIds(6);
  wedge->GetPointIds()->SetId(0,0);
  wedge->GetPointIds()->SetId(1,1);
  wedge->GetPointIds()->SetId(2,2);
  wedge->GetPointIds()->SetId(3,3);
  wedge->GetPointIds()->SetId(4,4);
  wedge->GetPointIds()->SetId(5,5);

  wedgeCoords[0][0] = .5; wedgeCoords[0][1] = 0.5; wedgeCoords[0][2] = 0.1;
  wedgeCoords[1][0] = .9; wedgeCoords[1][1] = 0.9; wedgeCoords[1][2] = 0.5;
  wedgeCoords[2][0] = .5; wedgeCoords[2][1] = 0.1; wedgeCoords[2][2] = 0.5;
  wedgeCoords[3][0] = .5; wedgeCoords[3][1] = 0.5; wedgeCoords[3][2] = 0.9;
  wedgeCoords[4][0] = .1; wedgeCoords[4][1] = 0.5; wedgeCoords[4][2] = 0.5;
  wedgeCoords[5][0] = .5; wedgeCoords[5][1] = 0.9; wedgeCoords[5][2] = 0.5;

  for (j = 0; j < 6; j++)
    {
    wedge->CellBoundary (0, wedgeCoords[j], ids);
    strm << "vtkWedge \t(" << wedgeCoords[j][0] << ", " << wedgeCoords[j][1] << ", " << wedgeCoords[j][2] << ") = \t";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }

  //Hexahedron
  vtkHexahedron *hexahedron = vtkHexahedron::New();
  double hexahedronCoords[8][3];

  hexahedron->GetPointIds()->SetNumberOfIds(8);
  hexahedron->GetPointIds()->SetId(0,0);
  hexahedron->GetPointIds()->SetId(1,1);
  hexahedron->GetPointIds()->SetId(2,2);
  hexahedron->GetPointIds()->SetId(3,3);
  hexahedron->GetPointIds()->SetId(4,4);
  hexahedron->GetPointIds()->SetId(5,5);
  hexahedron->GetPointIds()->SetId(6,6);
  hexahedron->GetPointIds()->SetId(7,7);

  hexahedronCoords[0][0] = .5; hexahedronCoords[0][1] = 0.5; hexahedronCoords[0][2] = 0.1;
  hexahedronCoords[1][0] = .9; hexahedronCoords[1][1] = 0.9; hexahedronCoords[1][2] = 0.5;
  hexahedronCoords[2][0] = .5; hexahedronCoords[2][1] = 0.1; hexahedronCoords[2][2] = 0.5;
  hexahedronCoords[3][0] = .5; hexahedronCoords[3][1] = 0.5; hexahedronCoords[3][2] = 0.1;
  hexahedronCoords[4][0] = .5; hexahedronCoords[4][1] = 0.5; hexahedronCoords[4][2] = 0.9;
  hexahedronCoords[5][0] = .9; hexahedronCoords[5][1] = 0.9; hexahedronCoords[5][2] = 0.7;
  hexahedronCoords[6][0] = .5; hexahedronCoords[6][1] = 0.1; hexahedronCoords[6][2] = 0.7;
  hexahedronCoords[7][0] = .5; hexahedronCoords[7][1] = 0.5; hexahedronCoords[7][2] = 0.9;

  for (j = 0; j < 8; j++)
    {
    hexahedron->CellBoundary (0, hexahedronCoords[j], ids);
    strm << "vtkHexahedron \t(" << hexahedronCoords[j][0] << ", " << hexahedronCoords[j][1] << ", " << hexahedronCoords[j][2] << ") = \t";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }

  //Pentagonal Prism
  vtkPentagonalPrism *penta = vtkPentagonalPrism::New();
  double pentaCoords[10][3];

  penta->GetPointIds()->SetNumberOfIds(10);
  penta->GetPointIds()->SetId(0,0);
  penta->GetPointIds()->SetId(1,1);
  penta->GetPointIds()->SetId(2,2);
  penta->GetPointIds()->SetId(3,3);
  penta->GetPointIds()->SetId(4,4);
  penta->GetPointIds()->SetId(5,5);
  penta->GetPointIds()->SetId(6,6);
  penta->GetPointIds()->SetId(7,7);
  penta->GetPointIds()->SetId(8,8);
  penta->GetPointIds()->SetId(9,9);

  pentaCoords[0][0] = 0.25; pentaCoords[0][1] = 0.0; pentaCoords[0][2] = 0.0;
  pentaCoords[1][0] = 0.75; pentaCoords[1][1] = 0.0; pentaCoords[1][2] = 0.0;
  pentaCoords[2][0] = 1.0 ; pentaCoords[2][1] = 0.5; pentaCoords[2][2] = 0.0;
  pentaCoords[3][0] = 0.5 ; pentaCoords[3][1] = 1.0; pentaCoords[3][2] = 0.0;
  pentaCoords[4][0] = 0.0 ; pentaCoords[4][1] = 0.5; pentaCoords[4][2] = 0.0;
  pentaCoords[5][0] = 0.25; pentaCoords[5][1] = 0.0; pentaCoords[5][2] = 1.0;
  pentaCoords[6][0] = 0.75; pentaCoords[6][1] = 0.0; pentaCoords[6][2] = 1.0;
  pentaCoords[7][0] = 1.0 ; pentaCoords[7][1] = 0.5; pentaCoords[7][2] = 1.0;
  pentaCoords[8][0] = 0.5 ; pentaCoords[8][1] = 1.0; pentaCoords[8][2] = 1.0;
  pentaCoords[9][0] = 0.0 ; pentaCoords[9][1] = 0.5; pentaCoords[9][2] = 1.0;

  for (j = 0; j < 10; j++)
    {
    penta->CellBoundary (0, pentaCoords[j], ids);
    strm << "vtkPentagonalPrism \t(" << pentaCoords[j][0] << ", " << pentaCoords[j][1] << ", " << pentaCoords[j][2] << ") = \t";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }

  //Hexagonal Prism
  vtkHexagonalPrism *hexa = vtkHexagonalPrism::New();
  double hexaCoords[12][3];

  hexa->GetPointIds()->SetNumberOfIds(12);
  hexa->GetPointIds()->SetId(0,0);
  hexa->GetPointIds()->SetId(1,1);
  hexa->GetPointIds()->SetId(2,2);
  hexa->GetPointIds()->SetId(3,3);
  hexa->GetPointIds()->SetId(4,4);
  hexa->GetPointIds()->SetId(5,5);
  hexa->GetPointIds()->SetId(6,6);
  hexa->GetPointIds()->SetId(7,7);
  hexa->GetPointIds()->SetId(8,8);
  hexa->GetPointIds()->SetId(9,9);
  hexa->GetPointIds()->SetId(10,12);
  hexa->GetPointIds()->SetId(11,11);

  hexaCoords[0][0] = 0.5 ; hexaCoords[0][1] = 0.0; hexaCoords[0][2] = 0.3;
  hexaCoords[1][0] = 0.93 ; hexaCoords[1][1] = 0.25; hexaCoords[1][2] = 0.3;
  hexaCoords[2][0] = 0.93 ; hexaCoords[2][1] = 0.75; hexaCoords[2][2] = 0.3;
  hexaCoords[3][0] = 0.716 ; hexaCoords[3][1] = 0.875; hexaCoords[3][2] = 0.4;
  hexaCoords[4][0] = 0.55 ; hexaCoords[4][1] = 0.95; hexaCoords[4][2] = 0.3;
  hexaCoords[5][0] = 0.067 ; hexaCoords[5][1] = 0.6; hexaCoords[5][2] = 0.1;
  hexaCoords[6][0] = 0.05 ; hexaCoords[6][1] = 0.4; hexaCoords[6][2] = 0.7;
  hexaCoords[7][0] = 0.5 ; hexaCoords[7][1] = 0.6; hexaCoords[7][2] = 0.7;
  hexaCoords[8][0] = 0.93 ; hexaCoords[8][1] = 0.4; hexaCoords[8][2] = 0.7;
  hexaCoords[9][0] = 0.93 ; hexaCoords[9][1] = 0.9; hexaCoords[9][2] = 0.7;
  hexaCoords[10][0] = 0.06 ; hexaCoords[10][1] = 0.7; hexaCoords[10][2] = 0.7;
  hexaCoords[11][0] = 0.07 ; hexaCoords[11][1] = 0.3; hexaCoords[11][2] = 0.7;

  for (j = 0; j < 12; j++)
    {
    hexa->CellBoundary (0, hexaCoords[j], ids);
    strm << "vtkHexagonalPrism \t(" << hexaCoords[j][0] << ", " << hexaCoords[j][1] << ", " << hexaCoords[j][2] << ") = \t";
    for (i = 0; i < ids->GetNumberOfIds(); i++)
      {
      strm << ids->GetId(i) << ", ";
      }
    strm << endl;
    }

  ids->Delete();
  vertex->Delete();
  polyVertex->Delete();
  line->Delete();
  polyLine->Delete();
  triangle->Delete();
  triangleStrip->Delete();
  quad->Delete();
  pixel->Delete();
  polygon->Delete();
  tetra->Delete();
  voxel->Delete();
  wedge->Delete();
  hexahedron->Delete();
  penta->Delete();
  hexa->Delete();

 strm << "Test vtkCell::CellBoundary Complete" << endl;

 return 0;
}


int otherCellBoundaries(int, char *[])
{
  vtksys_ios::ostringstream vtkmsg_with_warning_C4701; 
  return TestOCB(vtkmsg_with_warning_C4701);
} 
