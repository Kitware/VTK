/*==========================================================================

  Program: 
  Module:    otherCellBoundaries.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  ==========================================================================*/

// .NAME 
// .SECTION Description
// this program tests the CellBoundary method for each cell type

#include "vtkDebugLeaks.h"

#include "vtkIdList.h"
#include "vtkVertex.h"
#include "vtkVoxel.h"
#include "vtkPolyVertex.h"
#include "vtkTetra.h"
#include "vtkHexahedron.h"
#include "vtkLine.h"
#include "vtkPolyLine.h"
#include "vtkPolygon.h"
#include "vtkPixel.h"
#include "vtkQuad.h"
#include "vtkTriangle.h"
#include "vtkTriangleStrip.h"

void Test(ostream& strm)
{
  // actual test
  vtkIdList *ids = vtkIdList::New();
  int i, j, k;
  strm << "Test vtkCell::CellBoundary Start" << endl;

  vtkVertex *vertex = vtkVertex::New();
  float vertexCoords[1][1];

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
  
  vtkPolyVertex *polyVertex = vtkPolyVertex::New();
  float polyVertexCoords[1][1];

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
  
  vtkLine *line = vtkLine::New();
  float lineCoords[2][1];

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
  
  vtkPolyLine *polyLine = vtkPolyLine::New();
  float polyLineCoords[3][1];

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
  
  vtkTriangle *triangle = vtkTriangle::New();
  float triangleCoords[3][2];

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
  
  vtkTriangleStrip *triangleStrip = vtkTriangleStrip::New();
  float triangleStripCoords[3][2];

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
  
  vtkQuad *quad = vtkQuad::New();
  float quadCoords[4][2];

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

  vtkPixel *pixel = vtkPixel::New();
  float pixelCoords[4][2];

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
  
  vtkPolygon *polygon = vtkPolygon::New();
  float polygonCoords[4][2];

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
  
  vtkTetra *tetra = vtkTetra::New();
  float tetraCoords[4][3];

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
  
  vtkVoxel *voxel = vtkVoxel::New();
  float voxelCoords[6][3];

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

  vtkHexahedron *hexahedron = vtkHexahedron::New();
  float hexahedronCoords[6][3];

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
  hexahedronCoords[3][0] = .5; hexahedronCoords[3][1] = 0.5; hexahedronCoords[3][2] = 0.9;
  hexahedronCoords[4][0] = .1; hexahedronCoords[4][1] = 0.5; hexahedronCoords[4][2] = 0.5;
  hexahedronCoords[5][0] = .5; hexahedronCoords[5][1] = 0.9; hexahedronCoords[5][2] = 0.5;

  for (j = 0; j < 6; j++)
    {
    hexahedron->CellBoundary (0, hexahedronCoords[j], ids);
    strm << "vtkHexahedron \t(" << hexahedronCoords[j][0] << ", " << hexahedronCoords[j][1] << ", " << hexahedronCoords[j][2] << ") = \t";
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
  hexahedron->Delete();

 strm << "Test vtkCell::CellBoundary Complete" << endl;
}


int main(int argc, char* argv[])
{
  vtkDebugLeaks::PromptUserOff();

  Test(cout);

  return 0;
} 
