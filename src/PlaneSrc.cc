//
// Methods for plane generator
//
#include "PlaneSrc.hh"

void vlPlaneSource::SetResolution(const int xR, const int yR)
{
  if ( xR != this->XRes || yR != this->YRes )
  {
    this->XRes = xR;
    this->YRes = yR;

    this->XRes = (this->XRes > 0 ? this->XRes : 1);
    this->YRes = (this->YRes > 0 ? this->YRes : 1);

    this->Modified();
  }
}

void vlPlaneSource::Execute()
{
  float x[3], tc[2], n[3], xinc, yinc;
  int pts[MAX_VERTS];
  int i, j;
  int numPts;
  int numPolys;
  vlFloatPoints *newPoints; 
  vlFloatNormals *newNormals;
  vlFloatTCoords *newTCoords;
  vlCellArray *newPolys;
//
// Set things up; allocate memory
//
  Initialize();

  numPts = (this->XRes+1) * (this->YRes+1);
  numPolys = this->XRes * this->YRes;

  newPoints = new vlFloatPoints;
  newPoints->Initialize(numPts);

  newNormals = new vlFloatNormals;
  newNormals->Initialize(numPts);

  newTCoords = new vlFloatTCoords;
  newTCoords->Initialize(numPts,2);

  newPolys = new vlCellArray;
  newPolys->Initialize(5*numPolys);
//
// Generate points and point data
//
  xinc = 1.0 / ((float)this->XRes);
  yinc = 1.0 / ((float)this->YRes);
  x[2] = 0.0; // z-value
  n[0] = 0.0; n[1] = 0.0; n[2] = 1.0;

  for (numPts=0, i=0; i<(this->YRes+1); i++)
    {
    x[1] = tc[1] = -0.5 + i*yinc;
    for (j=0; j<(this->XRes+1); j++)
      {
      x[0] = tc[0] = -0.5 + j*xinc;

      newPoints->InsertPoint(numPts,x);
      newTCoords->InsertTCoord(numPts,tc);
      newNormals->InsertNormal(numPts++,n);
      }
    }
//
// Generate polygons
//
  for (i=0; i<this->YRes; i++)
    {
    x[1] = tc[1] = i*yinc;
    for (j=0; j<this->XRes; j++)
      {
      pts[0] = j + i*(this->XRes+1);
      pts[1] = pts[0] + 1;
      pts[2] = pts[0] + this->XRes + 2;
      pts[3] = pts[0] + this->XRes + 1;
      newPolys->InsertNextCell(4,pts);
      }
    }
//
// Update ourselves
//
  this->SetPoints(newPoints);
  this->PointData.SetNormals(newNormals);
  this->PointData.SetTCoords(newTCoords);

  this->SetPolys(newPolys);
}

