//
//  3D vectors, abstract representation
//
#include "Vectors.hh"
#include "FVectors.hh"
#include "IdList.hh"
#include "vlMath.hh"

void vlVectors::GetVectors(vlIdList& ptId, vlFloatVectors& fp)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fp.InsertVector(i,this->GetVector(ptId[i]));
    }
}
void vlVectors::ComputeMaxNorm()
{
  int i;
  float *v, norm;
  vlMath math;

  if ( this->GetMtime() > this->ComputeTime )
    {
    this->MaxNorm = 0.0;
    for (i=0; i<this->NumVectors(); i++)
      {
      v = this->GetVector(i);
      norm = math.Norm(v);
      if ( norm > this->MaxNorm ) this->MaxNorm = norm;
      }

    this->ComputeTime.Modified();
    }
}

float vlVectors::GetMaxNorm()
{
  this->ComputeMaxNorm();
  return this->MaxNorm;
}
