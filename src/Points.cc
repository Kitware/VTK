//
//  3D Points, abstract representation
//
#include "Points.hh"
#include "FPoints.hh"
#include "IdList.hh"

vlPoints::vlPoints()
{
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = 0.0;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = 1.0;
}

void vlPoints::GetPoints(vlIdList& ptId, vlFloatPoints& fp)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fp.InsertPoint(i,this->GetPoint(ptId[i]));
    }
}
void vlPoints::ComputeBounds()
{
  int i, j;
  float *x;

  if ( this->GetMtime() > this->ComputeTime )
    {
    this->Bounds[0] = this->Bounds[2] = this->Bounds[4] =  LARGE_FLOAT;
    this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -LARGE_FLOAT;
    for (i=0; i<this->NumPoints(); i++)
      {
      x = this->GetPoint(i);
      for (j=0; j<3; j++)
        {
        if ( x[j] < this->Bounds[2*j] ) this->Bounds[2*j] = x[j];
        if ( x[j] > this->Bounds[2*j+1] ) this->Bounds[2*j+1] = x[j];
        }
      }

    this->ComputeTime.Modified();
    }
}

float *vlPoints::GetBounds()
{
  this->ComputeBounds();
  return this->Bounds;
}

void vlPoints::PrintSelf(ostream& os)
{
  float * bounds;

  os << "    Number Points: " << this->NumPoints() << "\n";
  bounds = this->GetBounds();
  os << "    Bounds: \n";
  os << "      Xmin,Xmax: (" << bounds[0] << ", " << bounds[1] << ")\n";
  os << "      Ymin,Ymax: (" << bounds[2] << ", " << bounds[3] << ")\n";
  os << "      Zmin,Zmax: (" << bounds[4] << ", " << bounds[5] << ")\n";

  vlObject::PrintSelf(os);
}
