//
// Methods for Cone generator
//
#include <math.h>
#include "ConeSrc.hh"

vlConeSource::vlConeSource(int res)
{
  this->Resolution = res;
  this->Height = 1.0;
  this->Radius = 0.5;
  this->Capping = 1;
}

void vlConeSource::Execute()
{
  float angle= 2.0*3.141592654/this->Resolution;
  int numLines, numPolys, numPts;
  float x[3], xbot;
  int i;
  int pts[MAX_RESOLUTION];
  vlFloatPoints *newPoints; 
  vlCellArray *newLines=0;
  vlCellArray *newPolys=0;
//
// Set things up; allocate memory
//
  this->Initialize();

  switch ( this->Resolution )
  {
  case 0:
    numPts = 2;
    numLines =  1;
    newLines = new vlCellArray;
    newLines->Initialize(newLines->EstimateSize(numLines,numPts));
  
  case 1: case 2:
    numPts = 2*this->Resolution + 1;
    numPolys = this->Resolution;
    newPolys = new vlCellArray;
    newPolys->Initialize(newPolys->EstimateSize(numPolys,3));
    break;

  default:
    numPts = this->Resolution + 1;
    numPolys = this->Resolution + 1;
    newPolys = new vlCellArray;
    newPolys->Initialize(newPolys->EstimateSize(numPolys,this->Resolution));
    break;
  }
  newPoints = new vlFloatPoints;
  newPoints->Initialize(numPts);
//
// Create cone
//
  x[0] = this->Height / 2.0; // zero-centered
  x[1] = 0.0;
  x[2] = 0.0;
  pts[0] = newPoints->InsertNextPoint(x);

  xbot = -this->Height / 2.0;

  switch (this->Resolution) 
  {
  case 0:
    x[0] = xbot;
    x[1] = 0.0;
    x[2] = 0.0;
    pts[1] = newPoints->InsertNextPoint(x);
    newLines->InsertNextCell(2,pts);
    break;

  case 2:  // fall through this case to use the code in case 1
    x[0] = xbot;
    x[1] = 0.0;
    x[2] = -this->Radius;
    pts[1] = newPoints->InsertNextPoint(x);
    x[0] = xbot;
    x[1] = 0.0;
    x[2] = this->Radius;
    pts[2] = newPoints->InsertNextPoint(x);
 
    newPolys->InsertNextCell(3,pts);

  case 1:
    x[0] = xbot;
    x[1] = -this->Radius;
    x[2] = 0.0;
    pts[1] = newPoints->InsertNextPoint(x);
    x[0] = xbot;
    x[1] = this->Radius;
    x[2] = 0.0;
    pts[2] = newPoints->InsertNextPoint(x);

    newPolys->InsertNextCell(3,pts);

    break;

  default: // General case: create Resolution triangles and single cap

    for (i=0; i<this->Resolution; i++) 
      {
      x[0] = xbot;
      x[1] = this->Radius * cos ((double)i*angle);
      x[2] = this->Radius * sin ((double)i*angle);
      pts[1] = newPoints->InsertNextPoint(x);
      pts[2] = (pts[1] % this->Resolution) + 1;
      newPolys->InsertNextCell(3,pts);
      }
//
// If capping, create last polygon
//
    if ( this->Capping )
      {
      for (i=0; i<this->Resolution; i++) pts[i] = i+1;
      newPolys->InsertNextCell(this->Resolution,pts);
      }
  } //switch
//
// Update ourselves
//
  this->SetPoints(newPoints);

  if ( newPolys )
    {
    newPolys->Squeeze(); // we may have estimated size; reclaim some space
    this->SetPolys(newPolys);
    }
  else
    {
    this->SetLines(newLines);
    }
}
