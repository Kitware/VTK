//
// Methods for Line generator
//
#include <math.h>
#include "LineSrc.hh"

vlLineSource::vlLineSource(int res)
{
  this->Pt1[0] = -0.5;
  this->Pt1[1] =  0.0;
  this->Pt1[2] =  0.0;

  this->Pt2[0] =  0.5;
  this->Pt2[1] =  0.0;
  this->Pt2[2] =  0.0;

  this->Resolution = (res < 0 ? 1 : res);
}


void vlLineSource::SetResolution(int res)
{
  if ( res != this->Resolution )
    {
    this->Resolution = (res < 0 ? 1 : res);
    this->Modified();
    }
}
int vlLineSource::GetResolution()
{
  return this->Resolution;
}

void vlLineSource::SetPoint1(float* x)
{
  if ( x[0] != this->Pt1[0] || x[1] != this->Pt1[1] || x[2] != this->Pt1[2])
    {
    this->Pt1[0] = x[0]; this->Pt1[1] = x[1]; this->Pt1[2] = x[2]; 
    this->Modified();
    }
}
void vlLineSource::GetPoint1(float* &x)
{
  x[0] = this->Pt1[0];
  x[1] = this->Pt1[1];
  x[2] = this->Pt1[2];
}

void vlLineSource::SetPoint2(float* x)
{
  if ( x[0] != this->Pt2[0] || x[1] != this->Pt2[1] || x[2] != this->Pt2[2])
    {
    this->Pt2[0] = x[0]; this->Pt2[1] = x[1]; this->Pt2[2] = x[2]; 
    this->Modified();
    }
}
void vlLineSource::GetPoint2(float* &x)
{
  x[0] = this->Pt2[0];
  x[1] = this->Pt2[1];
  x[2] = this->Pt2[2];
}

void vlLineSource::Execute()
{
  int numLines=this->Resolution;
  int numPts=this->Resolution+1;
  float x[3], tc[2], v[3];
  int i, j;
  int pts[2];
  vlFloatPoints *newPoints; 
  vlFloatTCoords *newTCoords; 
  vlPointData *newPtData;
  vlCellArray *newLines;
//
// Set things up; allocate memory
//
  this->Initialize();

  newPoints = new vlFloatPoints;
  newPoints->Initialize(numPts);

  newTCoords = new vlFloatTCoords;
  newTCoords->Initialize(numPts,2);

  newPtData = new vlPointData;

  newLines = new vlCellArray;
  newLines->Initialize(newLines->EstimateSize(numLines,2));
//
// Generate lines
//
  for (i=0; i<3; i++) v[i] = this->Pt2[i] - this->Pt1[i];

  tc[1] = 0.0;
  for (i=0; i<numPts; i++) 
    {
    tc[0] = ((float)i/this->Resolution);
    for (j=0; j<3; j++) x[i] = this->Pt1[j] + tc[0]*v[j];
    newPoints->InsertPoint(i,x);
    newTCoords->InsertTCoord(i,tc);
    }
//
//  Generate line
//
  for (i=0; i < numLines; i++) 
    {
    pts[0] = i;
    pts[1] = i+1;
    newLines->InsertNextCell(2,pts);
    }
//
// Update ourselves
//
  this->SetPoints(newPoints);
  this->SetPointData(newPtData);
  newPtData->SetTCoords(newTCoords);
  this->SetLines(newLines);
}
