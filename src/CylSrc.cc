//
// Methods for plane generator
//
#include <math.h>
#include "CylSrc.hh"

void vlCylinderSource::SetResolution(int res)
{
  if ( res != this->Resolution )
    {
    this->Resolution = res;
    this->Resolution = (this->Resolution < 3 ? 3 :
        (this->Resolution > MAX_RESOLUTION ? MAX_RESOLUTION : this->Resolution));
    this->Modified();
    }
}
int vlCylinderSource::GetResolution()
{
  return this->Resolution;
}

void vlCylinderSource::SetHeight(float h)
{
  if ( h != this->Height )
    {
    this->Height = h;
    this->Height = (this->Height > 0.0 ? this->Height : 1.0);
    this->Modified();
    }
}
float vlCylinderSource::GetHeight()
{
  return this->Height;
}

void vlCylinderSource::SetRadius(float h)
{
  if ( h != this->Radius )
    {
    this->Radius = h;
    this->Radius = (this->Radius > 0.0 ? this->Radius : 0.5);
    this->Modified();
    }
}
float vlCylinderSource::GetRadius()
{
  return this->Radius;
}

void vlCylinderSource::SetCapping(int flag)
{
  if ( flag != this->Capping )
    {
    this->Capping = flag;
    this->Modified();
    }
}
int vlCylinderSource::GetCapping()
{
  return this->Capping;
}

void vlCylinderSource::Execute()
{
  float angle= 2.0*3.141592654/this->Resolution;
  int numVerts, numPolys, numPts;
  float xbot[3], tcbot[2], nbot[3];
  float xtop[3], tctop[2], ntop[3];
  int i, idx;
  int pts[MAX_RESOLUTION];
  vlFloatPoints *newPoints; 
  vlFloatNormals *newNormals;
  vlFloatTCoords *newTCoords;
  vlCellArray *newPolys;
  vlPointData *newPtData;
//
// Set things up; allocate memory
//
  this->Initialize();

  if ( this->Capping )
    {
    numVerts = 4*this->Resolution;
    numPolys = this->Resolution + 2;
    }
  else 
    {
    numVerts = 2*this->Resolution;
    numPolys = this->Resolution;
    }

  newPoints = new vlFloatPoints;
  newPoints->Initialize(numPts);

  newNormals = new vlFloatNormals;
  newNormals->Initialize(numPts);

  newTCoords = new vlFloatTCoords;
  newTCoords->Initialize(numPts,2);

  newPtData = new vlPointData;

  newPolys = new vlCellArray;
  newPolys->Initialize(newPolys->EstimateSize(numPolys,this->Resolution));
//
// Generate points and point data for sides
//
  for (i=0; i<this->Resolution; i++)
    {
    // x coordinate
    xbot[0] = xtop[0] = nbot[0] = ntop[0] = this->Radius * cos((double)i*angle);
    tcbot[0] = tctop[0] = fabs(2.0*i/this->Resolution - 1.0);

    // y coordinate
    xbot[1] = 0.5 * this->Height;
    xtop[1] = -0.5 * this->Height;
    nbot[1] = ntop[1] = 0.0;
    tcbot[1] = 0.0;
    tctop[1] = 1.0;

    // z coordinate
    xbot[2] = xtop[2] = nbot[2] = ntop[2] = -this->Radius * sin((double)i*angle);

    idx = 2*i;
    newPoints->InsertPoint(idx,xbot);
    newPoints->InsertPoint(idx+1,xtop);
    newTCoords->InsertTCoord(idx,tcbot);
    newTCoords->InsertTCoord(idx+1,tctop);
    newNormals->InsertNormal(idx,nbot);
    newNormals->InsertNormal(idx+1,ntop);
    }
//
// Generate polygons for sides
//
  for (i=0; i<this->Resolution; i++)
    {
    pts[0] = 2*i;
    pts[1] = pts[0] + 1;
    pts[2] = (pts[1] + 2) % this->Resolution;
    pts[3] = pts[3] - 1;
    newPolys->InsertNextCell(4,pts);
    }
//
// Generate points and point data for top/bottom polygons
//
  if ( this->Capping )
    {
    for (i=0; i<this->Resolution; i++)
      {
      // x coordinate
      xbot[0] = xtop[0] = this->Radius * cos((double)i*angle);
      nbot[0] = ntop[0] = 0.0;
      tcbot[0] = tctop[0] = xbot[0];

      // y coordinate
      xbot[1] = 0.5 * this->Height;
      xtop[1] = -0.5 * this->Height;
      nbot[1] = -1.0;
      ntop[1] =  1.0;

      // z coordinate
      xbot[2] = xtop[2] = -this->Radius * sin((double)i*angle);
      tcbot[1] = tctop[1] = xbot[2];

      idx = 2*this->Resolution;
      newPoints->InsertPoint(idx+i,xbot);
      newTCoords->InsertTCoord(idx+i,tcbot);
      newNormals->InsertNormal(idx+i,nbot);

      idx = 3*this->Resolution;
      newPoints->InsertPoint(idx+i,xtop);
      newTCoords->InsertTCoord(idx+i,tctop);
      newNormals->InsertNormal(idx+i,ntop);
      }
//
// Generate polygons for top/bottom polygons
//
    for (i=0; i<this->Resolution; i++)
      {
      pts[i] = 2*this->Resolution + i;
      }
    newPolys->InsertNextCell(this->Resolution,pts);
    for (i=0; i<this->Resolution; i++)
      {
      pts[i] = 3*this->Resolution + i;
      }
    newPolys->InsertNextCell(this->Resolution,pts);

    } // if capping
//
// Update ourselves
//
  this->SetPoints(newPoints);
  this->SetPointData(newPtData);
  newPtData->SetNormals(newNormals);
  newPtData->SetTCoords(newTCoords);

  newPolys->Squeeze(); // since we've estimated size; reclaim some space
  this->SetPolys(newPolys);
}
