//
// Methods for cube generator
//
#include <math.h>
#include "CubeSrc.hh"
#include "FPoints.hh"
#include "FNormals.hh"

vlCubeSource::vlCubeSource(float xL, float yL, float zL)
{
  this->XLength = fabs(xL);
  this->YLength = fabs(yL);
  this->ZLength = fabs(zL);
}

void vlCubeSource::Execute()
{
  float x[3], n[3];
  int numPolys=6, numPts=24;
  int i, j, k;
  int pts[4];
  vlFloatPoints *newPoints; 
  vlFloatNormals *newNormals;
  vlCellArray *newPolys;
//
// Set things up; allocate memory
//
  this->Initialize();

  newPoints = new vlFloatPoints(numPts);
  newNormals = new vlFloatNormals(numPts);

  newPolys = new vlCellArray;
  newPolys->Initialize(newPolys->EstimateSize(numPolys,4));
//
// Generate points and normals
//
  numPts = 0;

  for (x[0]=(-this->XLength)/2.0, n[0]=(-1.0), n[1]=n[2]=0.0, i=0; i<2; 
  i++, x[0]+=this->XLength, n[0]+=2.0)
    {
    for (x[1]=(-this->YLength/2.0), j=0; j<2; j++, x[1]+=this->YLength)
      {
      for (x[2]=(-this->ZLength/2.0), k=0; k<2; k++, x[2]+=this->ZLength)
        {
            newPoints->InsertNextPoint(x);
            newNormals->InsertNextNormal(n);
        }
      }
    }
   pts[0] = 0; pts[1] = 1; pts[2] = 3; pts[3] = 2; 
   newPolys->InsertNextCell(4,pts);
   pts[0] += 4; pts[1] +=4; pts[2] +=4; pts[3] += 4; 
   newPolys->InsertNextCell(4,pts);

  for (x[1]=(-this->YLength)/2.0, n[1]=(-1.0), n[0]=n[2]=0.0, i=0; i<2; 
  i++, x[1]+=this->YLength, n[1]+=2.0)
    {
    for (x[0]=(-this->XLength/2.0), j=0; j<2; j++, x[0]+=this->XLength)
      {
      for (x[2]=(-this->ZLength/2.0), k=0; k<2; k++, x[2]+=this->ZLength)
        {
            newPoints->InsertNextPoint(x);
            newNormals->InsertNextNormal(n);
        }
      }
    }
  pts[0] += 4; pts[1] +=4; pts[2] +=4; pts[3] += 4; 
  newPolys->InsertNextCell(4,pts);
  pts[0] += 4; pts[1] +=4; pts[2] +=4; pts[3] += 4; 
  newPolys->InsertNextCell(4,pts);

  for (x[2]=(-this->ZLength)/2.0, n[2]=(-1.0), n[0]=n[1]=0.0, i=0; i<2; 
  i++, x[2]+=this->ZLength, n[2]+=2.0)
    {
    for (x[1]=(-this->YLength/2.0), j=0; j<2; j++, x[1]+=this->YLength)
      {
      for (x[0]=(-this->XLength/2.0), k=0; k<2; k++, x[0]+=this->XLength)
        {
            newPoints->InsertNextPoint(x);
            newNormals->InsertNextNormal(n);
        }
      }
    }
   pts[0] += 4; pts[1] +=4; pts[2] +=4; pts[3] += 4; 
   newPolys->InsertNextCell(4,pts);
   pts[0] += 4; pts[1] +=4; pts[2] +=4; pts[3] += 4; 
   newPolys->InsertNextCell(4,pts);
//
// Update ourselves
//
  this->SetPoints(newPoints);
  this->PointData.SetNormals(newNormals);

  newPolys->Squeeze(); // since we've estimated size; reclaim some space
  this->SetPolys(newPolys);
}
