/*==========================================================================

  Program: 
  Module:    otherCellPosition.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  ==========================================================================*/

// .NAME 
// .SECTION Description
// this program tests the Cell Position and Location Boundary methods for each cell type

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
  int j;
  int n;
  float dist2;
  int subId;

  strm << "Test vtkCell::EvaluatePosition Start" << endl;

  vtkVertex *vertex = vtkVertex::New();
  float vertexCoords[3], vertexWeights[2];
  float vertexPoint[2][3] = {{10.0, 20.0, 30.0}, {0, 0, 0}};
  float vertexClosest[3];

  vertex->GetPointIds()->SetId(0,0);
  vertex->GetPoints()->SetPoint (0, 10.0, 20.0, 30.0);

  n = sizeof(vertexPoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    vertex->EvaluatePosition (&vertexPoint[j][0], &vertexClosest[0], subId, &vertexCoords[0], dist2, &vertexWeights[0]);
    strm << "vtkVertex (" << vertexPoint[j][0] << ", " << vertexPoint[j][1] << ", " << vertexPoint[j][2] << ")" << endl;
    strm << "\tclosest: " << vertexClosest[0] << ", " << vertexClosest[1] << ", " << vertexClosest[2] << endl;
    strm << "\tcoords: " << vertexCoords[0] << endl;
    strm << "\tweights: " << vertexWeights[0] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    strm << endl;
    }
  
  vtkPolyVertex *polyVertex = vtkPolyVertex::New();
  float polyVertexCoords[1], polyVertexWeights[1];
  float polyVertexPoint[3][3] = {{10.0, 20.0, 30.0}, {30.0, 20.0, 10.0}, {0, 0, 0}};
  float polyVertexClosest[3];

  polyVertex->GetPointIds()->SetNumberOfIds(2);
  polyVertex->GetPointIds()->SetId(0,0);
  polyVertex->GetPointIds()->SetId(1,1);
  
  polyVertex->GetPoints()->SetNumberOfPoints(2);
  polyVertex->GetPoints()->SetPoint (0, 10.0, 20.0, 30.0);
  polyVertex->GetPoints()->SetPoint (1, 30.0, 20.0, 10.0);

  n = sizeof(polyVertexPoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    polyVertex->EvaluatePosition (&polyVertexPoint[j][0], &polyVertexClosest[0], subId, &polyVertexCoords[0], dist2, &polyVertexWeights[0]);
    strm << "vtkPolyVertex (" << polyVertexPoint[j][0] << ", " << polyVertexPoint[j][1] << ", " << polyVertexPoint[j][2] << ")" << endl;
    strm << "\tclosest: " << polyVertexClosest[0] << ", " << polyVertexClosest[1] << ", " << polyVertexClosest[2] << endl;
    strm << "\tcoords: " << polyVertexCoords[0] << endl;
    strm << "\tweights: " << polyVertexWeights[0] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    strm << endl;
    }

  vtkLine *line = vtkLine::New();
  float lineCoords[3], lineWeights[2];
  float linePoint[3][3] = {{10.0, 20.0, 30.0}, {30.0, 20.0, 10.0}, {0, 0, 0}};
  float lineClosest[3];

  line->GetPointIds()->SetId(0,0);
  line->GetPointIds()->SetId(1,1);
  line->GetPoints()->SetPoint (0, 10.0, 20.0, 30.0);
  line->GetPoints()->SetPoint (1, 30.0, 20.0, 10.0);
  
  n = sizeof(linePoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    line->EvaluatePosition (&linePoint[j][0], &lineClosest[0], subId, &lineCoords[0], dist2, &lineWeights[0]);
    strm << "vtkLine (" << linePoint[j][0] << ", " << linePoint[j][1] << ", " << linePoint[j][2] << ")" << endl;
    strm << "\tclosest: " << lineClosest[0] << ", " << lineClosest[1] << ", " << lineClosest[2] << endl;
    strm << "\tcoords: " << lineCoords[0] << endl;
    strm << "\tweights: " << lineWeights[0] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    strm << endl;
    }

  vtkPolyLine *polyLine = vtkPolyLine::New();
  float polyLineCoords[3], polyLineWeights[3];
  float polyLinePoint[4][3] = {{10.0, 20.0, 30.0}, {10.0, 30.0, 30.0}, {10.0, 30.0, 40.0}, {0, 0, 0}};
  float polyLineClosest[3];

  polyLine->GetPointIds()->SetNumberOfIds(3);
  polyLine->GetPointIds()->SetId(0,0);
  polyLine->GetPointIds()->SetId(1,1);
  polyLine->GetPointIds()->SetId(2,2);

  polyLine->GetPoints()->SetNumberOfPoints(3);
  polyLine->GetPoints()->SetPoint (0, 10.0, 20.0, 30.0);
  polyLine->GetPoints()->SetPoint (1, 10.0, 30.0, 30.0);
  polyLine->GetPoints()->SetPoint (2, 10.0, 30.0, 40.0);
  
  n = sizeof(polyLinePoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    polyLine->EvaluatePosition (&polyLinePoint[j][0], &polyLineClosest[0], subId, &polyLineCoords[0], dist2, &polyLineWeights[0]);
    strm << "vtkPolyLine (" << polyLinePoint[j][0] << ", " << polyLinePoint[j][1] << ", " << polyLinePoint[j][2] << ")" << endl;
    strm << "\tclosest: " << polyLineClosest[0] << ", " << polyLineClosest[1] << ", " << polyLineClosest[2] << endl;
    strm << "\tcoords: " << polyLineCoords[0] << endl;
    strm << "\tweights: " << polyLineWeights[0] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    strm << endl;
    }

  vtkTriangle *triangle = vtkTriangle::New();
  float triangleCoords[3], triangleWeights[3], trianglePosition[3];
  float trianglePoint[4][3] = {{10.0, 10.0, 10.0}, {12.0, 10.0, 10.0}, {11.0, 12.0, 12.0}, {11, 11, 11}};
  float triangleClosest[3];

  triangle->GetPointIds()->SetId(0,0);
  triangle->GetPointIds()->SetId(1,1);
  triangle->GetPointIds()->SetId(2,2);

  triangle->GetPoints()->SetPoint (0, 10.0, 10.0, 10.0);
  triangle->GetPoints()->SetPoint (1, 12.0, 10.0, 10.0);
  triangle->GetPoints()->SetPoint (2, 11.0, 12.0, 12.0);
  
  n = sizeof(trianglePoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    triangle->EvaluatePosition (&trianglePoint[j][0], &triangleClosest[0], subId, &triangleCoords[0], dist2, &triangleWeights[0]);
    strm << "vtkTriangle (" << trianglePoint[j][0] << ", " << trianglePoint[j][1] << ", " << trianglePoint[j][2] << ")" << endl;
    strm << "\tclosest: " << triangleClosest[0] << ", " << triangleClosest[1] << ", " << triangleClosest[2] << endl;
    strm << "\tcoords: " << triangleCoords[0] << ", " << triangleCoords[1] << ", " << triangleCoords[2] << endl;
    strm << "\tweights: " << triangleWeights[0] << ", " << triangleWeights[1] << ", " << triangleWeights[2] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    triangle->EvaluateLocation (subId, triangleCoords, trianglePosition, triangleWeights);
    strm << "\tposition: " << trianglePosition[0] << ", " << trianglePosition[1] << ", " << trianglePosition[2] << endl;
    strm << endl;
    }

  
  vtkTriangleStrip *triangleStrip = vtkTriangleStrip::New();
  float triangleStripCoords[3], triangleStripWeights[4], triangleStripPosition[3];
  float triangleStripPoint[5][3] = {{10.0, 10.0, 10.0}, {12.0, 10.0, 10.0}, {11.0, 12.0, 10.0}, {13, 10, 10}, {11, 11, 10}};
  float triangleStripClosest[3];

  triangleStrip->GetPointIds()->SetNumberOfIds(4);
  triangleStrip->GetPointIds()->SetId(0,0);
  triangleStrip->GetPointIds()->SetId(1,1);
  triangleStrip->GetPointIds()->SetId(2,2);
  triangleStrip->GetPointIds()->SetId(3,3);
  
  triangleStrip->GetPoints()->SetNumberOfPoints(4);
  triangleStrip->GetPoints()->SetPoint (0, 10.0, 10.0, 10.0);
  triangleStrip->GetPoints()->SetPoint (1, 12.0, 10.0, 10.0);
  triangleStrip->GetPoints()->SetPoint (2, 11.0, 12.0, 10.0);
  triangleStrip->GetPoints()->SetPoint (3, 13.0, 10.0, 10.0);

  n = sizeof(triangleStripPoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    triangleStrip->EvaluatePosition (&triangleStripPoint[j][0], &triangleStripClosest[0], subId, &triangleStripCoords[0], dist2, &triangleStripWeights[0]);
    strm << "vtkTriangleStrip (" << triangleStripPoint[j][0] << ", " << triangleStripPoint[j][1] << ", " << triangleStripPoint[j][2] << ")" << endl;
    strm << "\tclosest: " << triangleStripClosest[0] << ", " << triangleStripClosest[1] << ", " << triangleStripClosest[2] << endl;
    strm << "\tcoords: " << triangleStripCoords[0] << ", " << triangleStripCoords[1] << ", " << triangleStripCoords[2] << endl;
    strm << "\tweights: " << triangleStripWeights[0] << ", " << triangleStripWeights[1] << ", " << triangleStripWeights[2] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    triangleStrip->EvaluateLocation (subId, triangleStripCoords, triangleStripPosition, triangleStripWeights);
    strm << "\tposition: " << triangleStripPosition[0] << ", " << triangleStripPosition[1] << ", " << triangleStripPosition[2] << endl;
    strm << endl;
    }

  vtkQuad *quad = vtkQuad::New();
  float quadCoords[2], quadWeights[4], quadPosition[3];
  float quadPoint[5][3] = {{10.0, 10.0, 10.0}, {12.0, 10.0, 10.0}, {12.0, 12.0, 10.0}, {10, 12, 10}, {11, 11, 10.1}};
  float quadClosest[3];

  quad->GetPointIds()->SetId(0,0);
  quad->GetPointIds()->SetId(1,1);
  quad->GetPointIds()->SetId(2,2);
  quad->GetPointIds()->SetId(3,3);
  
  quad->GetPoints()->SetPoint (0, 10.0, 10.0, 10.0);
  quad->GetPoints()->SetPoint (1, 12.0, 10.0, 10.0);
  quad->GetPoints()->SetPoint (2, 12.0, 12.0, 10.0);
  quad->GetPoints()->SetPoint (3, 10.0, 12.0, 10.0);

  n = sizeof(quadPoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    quad->EvaluatePosition (&quadPoint[j][0], &quadClosest[0], subId, &quadCoords[0], dist2, &quadWeights[0]);
    strm << "vtkQuad (" << quadPoint[j][0] << ", " << quadPoint[j][1] << ", " << quadPoint[j][2] << ")" << endl;
    strm << "\tclosest: " << quadClosest[0] << ", " << quadClosest[1] << ", " << quadClosest[2] << endl;
    strm << "\tcoords: " << quadCoords[0] << ", " << quadCoords[1] << endl;
    strm << "\tweights: " << quadWeights[0] << ", " << quadWeights[1] << ", " << quadWeights[2] << ", " << quadWeights[3] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    quad->EvaluateLocation (subId, quadCoords, quadPosition, quadWeights);
    strm << "\tposition: " << quadPosition[0] << ", " << quadPosition[1] << ", " << quadPosition[2] << endl;
    strm << endl;
    }

  vtkPixel *pixel = vtkPixel::New();
  float pixelCoords[3], pixelWeights[4], pixelPosition[3];
  float pixelPoint[5][3] = {{10.0, 10.0, 10.0}, {12.0, 10.0, 10.0}, {12.0, 12.0, 10.0}, {10, 12, 10}, {11, 11, 10.1}};
  float pixelClosest[3];

  pixel->GetPointIds()->SetId(0,0);
  pixel->GetPointIds()->SetId(1,1);
  pixel->GetPointIds()->SetId(2,3);
  pixel->GetPointIds()->SetId(3,2);
  
  pixel->GetPoints()->SetPoint (0, 10.0, 10.0, 10.0);
  pixel->GetPoints()->SetPoint (1, 12.0, 10.0, 10.0);
  pixel->GetPoints()->SetPoint (3, 12.0, 12.0, 10.0);
  pixel->GetPoints()->SetPoint (2, 10.0, 12.0, 10.0);

  n = sizeof(pixelPoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    pixel->EvaluatePosition (&pixelPoint[j][0], &pixelClosest[0], subId, &pixelCoords[0], dist2, &pixelWeights[0]);
    strm << "vtkPixel (" << pixelPoint[j][0] << ", " << pixelPoint[j][1] << ", " << pixelPoint[j][2] << ")" << endl;
    strm << "\tclosest: " << pixelClosest[0] << ", " << pixelClosest[1] << ", " << pixelClosest[2] << endl;
    strm << "\tcoords: " << pixelCoords[0] << ", " << pixelCoords[1] << endl;
    strm << "\tweights: " << pixelWeights[0] << ", " << pixelWeights[1] << ", " << pixelWeights[2] << ", " << pixelWeights[3] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    pixel->EvaluateLocation (subId, pixelCoords, pixelPosition, pixelWeights);
    strm << "\tposition: " << pixelPosition[0] << ", " << pixelPosition[1] << ", " << pixelPosition[2] << endl;
    strm << endl;
    }

  vtkPolygon *polygon = vtkPolygon::New();
  float polygonCoords[2], polygonWeights[4], polygonPosition[3];
  float polygonPoint[5][3] = {{0.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {1.0, 1.0, 0.0}, {0, 1, 0}, {.5, .5, 0}};
  float polygonClosest[3];

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
  
  n = sizeof(polygonPoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    polygon->EvaluatePosition (&polygonPoint[j][0], &polygonClosest[0], subId, &polygonCoords[0], dist2, &polygonWeights[0]);
    strm << "vtkPolygon (" << polygonPoint[j][0] << ", " << polygonPoint[j][1] << ", " << polygonPoint[j][2] << ")" << endl;
    strm << "\tclosest: " << polygonClosest[0] << ", " << polygonClosest[1] << ", " << polygonClosest[2] << endl;
    strm << "\tcoords: " << polygonCoords[0] << ", " << polygonCoords[1] << endl;
    strm << "\tweights: " << polygonWeights[0] << ", " << polygonWeights[1] << ", " << polygonWeights[2] << ", " << polygonWeights[3] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    polygon->EvaluateLocation (subId, polygonCoords, polygonPosition, polygonWeights);
    strm << "\tposition: " << polygonPosition[0] << ", " << polygonPosition[1] << ", " << polygonPosition[2] << endl;
    strm << endl;
    }

  
  vtkTetra *tetra = vtkTetra::New();
  float tetraCoords[3], tetraWeights[4], tetraPosition[3];
  float tetraPoint[5][3] = {{10, 10, 10}, {12, 10, 10}, {11, 12, 10}, {11, 11, 12}, {11, 11, 11}};
  float tetraClosest[3];

  tetra->GetPointIds()->SetNumberOfIds(4);
  tetra->GetPointIds()->SetId(0,0);
  tetra->GetPointIds()->SetId(1,1);
  tetra->GetPointIds()->SetId(2,2);
  tetra->GetPointIds()->SetId(3,3);
  tetra->GetPoints()->SetPoint(0, 10.0, 10.0, 10.0);
  tetra->GetPoints()->SetPoint(1, 12.0, 10.0, 10.0);
  tetra->GetPoints()->SetPoint(2, 11.0, 12.0, 10.0);
  tetra->GetPoints()->SetPoint(3, 11.0, 11.0, 12.0);

  n = sizeof(tetraPoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    tetra->EvaluatePosition (&tetraPoint[j][0], &tetraClosest[0], subId, &tetraCoords[0], dist2, &tetraWeights[0]);
    strm << "vtkTetra (" << tetraPoint[j][0] << ", " << tetraPoint[j][1] << ", " << tetraPoint[j][2] << ")" << endl;
    strm << "\tclosest: " << tetraClosest[0] << ", " << tetraClosest[1] << ", " << tetraClosest[2] << endl;
    strm << "\tcoords: " << tetraCoords[0] << ", " << tetraCoords[1] << ", " << tetraCoords[2] << endl;
    strm << "\tweights: " << tetraWeights[0] << ", " << tetraWeights[1] << ", " << tetraWeights[2] << ", " << tetraWeights[3] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    tetra->EvaluateLocation (subId, tetraCoords, tetraPosition, tetraWeights);
    strm << "\tposition: " << tetraPosition[0] << ", " << tetraPosition[1] << ", " << tetraPosition[2] << endl;
    strm << endl;
    }

  vtkVoxel *voxel = vtkVoxel::New();
  float voxelCoords[3], voxelWeights[8], voxelPosition[3];
  float voxelPoint[9][3] = {{10, 10, 10}, {12, 10, 10}, {12, 12, 10}, {10, 12, 10},
			    {10, 10, 12}, {12, 10, 12}, {12, 12, 12}, {10, 12, 12},
			    {11, 11, 11}};
  float voxelClosest[3];

  voxel->GetPointIds()->SetNumberOfIds(8);
  voxel->GetPointIds()->SetId(0,0);
  voxel->GetPointIds()->SetId(1,1);
  voxel->GetPointIds()->SetId(2,3);
  voxel->GetPointIds()->SetId(3,2);
  voxel->GetPointIds()->SetId(4,4);
  voxel->GetPointIds()->SetId(5,5);
  voxel->GetPointIds()->SetId(6,7);
  voxel->GetPointIds()->SetId(7,6);

  voxel->GetPoints()->SetPoint(0, 10, 10, 10);
  voxel->GetPoints()->SetPoint(1, 12, 10, 10);
  voxel->GetPoints()->SetPoint(3, 12, 12, 10);
  voxel->GetPoints()->SetPoint(2, 10, 12, 10);
  voxel->GetPoints()->SetPoint(4, 10, 10, 12);
  voxel->GetPoints()->SetPoint(5, 12, 10, 12);
  voxel->GetPoints()->SetPoint(7, 12, 12, 12);
  voxel->GetPoints()->SetPoint(6, 10, 12, 12);

  n = sizeof(voxelPoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    voxel->EvaluatePosition (&voxelPoint[j][0], &voxelClosest[0], subId, &voxelCoords[0], dist2, &voxelWeights[0]);
    strm << "vtkVoxel (" << voxelPoint[j][0] << ", " << voxelPoint[j][1] << ", " << voxelPoint[j][2] << ")" << endl;
    strm << "\tclosest: " << voxelClosest[0] << ", " << voxelClosest[1] << ", " << voxelClosest[2] << endl;
    strm << "\tcoords: " << voxelCoords[0] << ", " << voxelCoords[1] << ", " << voxelCoords[2] << endl;
    strm << "\tweights: " << voxelWeights[0] << ", " << voxelWeights[1] << ", " << voxelWeights[2] << ", " << voxelWeights[3] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    voxel->EvaluateLocation (subId, voxelCoords, voxelPosition, voxelWeights);
    strm << "\tposition: " << voxelPosition[0] << ", " << voxelPosition[1] << ", " << voxelPosition[2] << endl;
    strm << endl;
    }

  vtkHexahedron *hexahedron = vtkHexahedron::New();
  float hexahedronCoords[3], hexahedronWeights[8], hexahedronPosition[3];
  float hexahedronPoint[9][3] = {{10, 10, 10}, {12, 10, 10}, {12, 12, 10}, {10, 12, 10},
				 {10, 10, 12}, {12, 10, 12}, {12, 12, 12}, {10, 12, 12},
				 {11, 11, 11}};
  float hexahedronClosest[3];

  hexahedron->GetPointIds()->SetNumberOfIds(8);
  hexahedron->GetPointIds()->SetId(0,0);
  hexahedron->GetPointIds()->SetId(1,1);
  hexahedron->GetPointIds()->SetId(2,2);
  hexahedron->GetPointIds()->SetId(3,3);
  hexahedron->GetPointIds()->SetId(4,4);
  hexahedron->GetPointIds()->SetId(5,5);
  hexahedron->GetPointIds()->SetId(6,6);
  hexahedron->GetPointIds()->SetId(7,7);

  hexahedron->GetPoints()->SetPoint(0, 10, 10, 10);
  hexahedron->GetPoints()->SetPoint(1, 12, 10, 10);
  hexahedron->GetPoints()->SetPoint(2, 12, 12, 10);
  hexahedron->GetPoints()->SetPoint(3, 10, 12, 10);
  hexahedron->GetPoints()->SetPoint(4, 10, 10, 12);
  hexahedron->GetPoints()->SetPoint(5, 12, 10, 12);
  hexahedron->GetPoints()->SetPoint(6, 12, 12, 12);
  hexahedron->GetPoints()->SetPoint(7, 10, 12, 12);

  n = sizeof(hexahedronPoint) / (3 * sizeof(float));
  for (j = 0; j < n; j++)
    {
    hexahedron->EvaluatePosition (&hexahedronPoint[j][0], &hexahedronClosest[0], subId, &hexahedronCoords[0], dist2, &hexahedronWeights[0]);
    strm << "vtkHexahedron (" << hexahedronPoint[j][0] << ", " << hexahedronPoint[j][1] << ", " << hexahedronPoint[j][2] << ")" << endl;
    strm << "\tclosest: " << hexahedronClosest[0] << ", " << hexahedronClosest[1] << ", " << hexahedronClosest[2] << endl;
    strm << "\tcoords: " << hexahedronCoords[0] << ", " << hexahedronCoords[1] << ", " << hexahedronCoords[2] << endl;
    strm << "\tweights: " << hexahedronWeights[0] << ", " << hexahedronWeights[1] << ", " << hexahedronWeights[2] << ", " << hexahedronWeights[3] << endl;
    strm << "\tsubid: " << subId << endl;
    strm << "\tdist2: " << dist2 << endl;
    hexahedron->EvaluateLocation (subId, hexahedronCoords, hexahedronPosition, hexahedronWeights);
    strm << "\tposition: " << hexahedronPosition[0] << ", " << hexahedronPosition[1] << ", " << hexahedronPosition[2] << endl;
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

  strm << "Test vtkCell::CellPosition Complete" << endl;
}

int main(int argc, char* argv[])
{
  vtkDebugLeaks::PromptUserOff();

  Test(cout);

  return 0;
} 
