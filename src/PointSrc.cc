#include <math.h>
#include "PointSrc.hh"
#include "vlMath.hh"

vlPointSource::vlPointSource(int numPts)
{
  this->NumPoints = (numPts > 0 ? numPts : 10);

  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->Radius = 0.5;
}

void vlPointSource::Execute()
{
  int i;
  float radius, theta, phi, x[3];
  vlFloatPoints *newPoints;
  vlCellArray *newVerts;
  vlMath math;
  int pts[1];

  this->Initialize();

  newPoints = new vlFloatPoints(this->NumPoints);
  newVerts = new vlCellArray(this->NumPoints);

  for (i=0; i<this->NumPoints; i++)
    {
    radius = this->Radius * math.Random();
    theta = 2.0*math.Pi() * math.Random()  - math.Pi();
    phi = math.Pi() * math.Random();
    x[0] = radius * cos(theta);
    x[1] = radius * sin(theta);
    x[2] = radius * cos(phi);
    pts[0] = newPoints->InsertNextPoint(x);
    newVerts->InsertNextCell(1,pts);
    }
//
// Update ourselves
//
  this->SetPoints(newPoints);
  this->SetVerts(newVerts);
}
