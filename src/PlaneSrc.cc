//
// Methods for plane generator
//
#include "PlaneSrc.h"

void PlaneSource::setResolution(const int xR, const int yR)
{
  if ( xR != xRes || yR != yRes )
  {
    xRes = xR;
    yRes = yR;

    xRes = (xRes > 0 ? xRes : 1);
    yRes = (yRes > 0 ? yRes : 1);

    modified();
  }
}

void PlaneSource::execute()
{
  float x[3], tc[2], n[3], xinc, yinc;
  int pts[MAX_VERTS];
  int i, j;
  int numPts;
  int numPolys;
  FloatPoints *newPoints; 
  FloatNormals *newNormals;
  FloatTCoords *newTCoords;
  CellArray *newPolys;
  PointData *newPtData;
//
// Set things up; allocate memory
//
  Initialize();

  numPts = (xRes+1) * (yRes+1);
  numPolys = xRes * yRes;

  newPoints = new FloatPoints;
  newPoints->Initialize(numPts);

  newNormals = new FloatNormals;
  newNormals->Initialize(numPts);

  newTCoords = new FloatTCoords;
  newTCoords->Initialize(numPts);

  newPtData = new PointData;

  newPolys = new CellArray;
  newPolys->Initialize(5*numPolys);
//
// Generate points and point data
//
  xinc = 1.0 / ((float)xRes);
  yinc = 1.0 / ((float)yRes);
  x[2] = 0.0; // z-value
  n[0] = 0.0; n[1] = 0.0; n[2] = 1.0;

  for (numPts=0, i=0; i<(yRes+1); i++)
  {
    x[1] = tc[1] = i*yinc;
    for (j=0; j<(xRes+1); j++)
    {
      x[0] = tc[0] = j*xinc;

      newPoints->insertPoint(numPts,x);
      newTCoords->insertTCoord(numPts,tc);
      newNormals->insertNormal(numPts++,n);

    }
  }
//
// Generate polygons
//
  for (i=0; i<yRes; i++)
  {
    x[1] = tc[1] = i*yinc;
    for (j=0; j<xRes; j++)
    {
      pts[0] = j + i*(xRes+1);
      pts[1] = pts[0] + 1;
      pts[2] = pts[0] + xRes + 2;
      pts[3] = pts[0] + xRes + 1;
      newPolys->insertNextCell(4,pts);
    }
  }
//
// Update ourselves
//
  setPoints(newPoints);
  setPointData(newPtData);
  newPtData->setNormals(newNormals);
  newPtData->setTCoords(newTCoords);

  setPolys(newPolys);
}

