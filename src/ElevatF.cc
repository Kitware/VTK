//
// Methods for elevation filter
//
#include "ElevatF.hh"
#include "vlMath.hh"
#include "FScalars.hh"

vlElevationFilter::vlElevationFilter()
{
  this->LowPoint[0] = 0.0;
  this->LowPoint[1] = 0.0;
  this->LowPoint[2] = 0.0;
 
  this->HighPoint[0] = 0.0;
  this->HighPoint[1] = 0.0;
  this->HighPoint[2] = 1.0;

  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;
}

//
// Convert position along ray into scalar value.  Example use includes 
// coloring terrain by elevation.
//
void vlElevationFilter::Execute()
{
  int i, j, numPts;
  vlFloatScalars *newScalars;
  float l, *bounds, *x, s, v[3];
  float diffVector[3], diffScalar;
//
// Initialize
//
  this->Initialize();
  if ( ((numPts=this->Input->NumPoints()) < 1) )
    {
    cerr << "No input available for Elevation Filter\n";
    return;
    }
//
// Allocate
//
  newScalars = new vlFloatScalars(numPts);
//
// Set up 1D parametric system
//
  bounds = this->Input->GetBounds();

  for (i=0; i<3; i++) diffVector[i] = this->HighPoint[i] - this->LowPoint[i];
  if ( (l = vlDOT(diffVector,diffVector)) == 0.0)
    {
    cerr << this << ": Bad vector, using (0,0,1)\n";
    diffVector[0] = diffVector[1] = 0.0; diffVector[2] = 1.0;
    l = 1.0;
    }
//
// Compute parametric coordinate and map into scalar range
//
  diffScalar = this->ScalarRange[1] - this->ScalarRange[0];
  for (i=0; i<numPts; i++)
    {
    x = this->Input->GetPoint(i);
    for (j=0; j<3; j++) v[j] = x[j] - this->LowPoint[j];
    s = vlDOT(v,diffVector) / l;
    s = (s < 0.0 ? 0.0 : s > 1.0 ? 1.0 : s);
    newScalars->InsertScalar(i,this->ScalarRange[0]+s*diffScalar);
    }
//
// Update self
//
  this->PointData.SetScalars(newScalars);
}

