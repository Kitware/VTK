//
//  Scalars, abstract representation
//
#include "Scalars.hh"
#include "FScalars.hh"
#include "IdList.hh"

void vlScalars::GetScalars(vlIdList& ptId, vlFloatScalars& fp)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fp.InsertScalar(i,this->GetScalar(ptId[i]));
    }
}
void vlScalars::ComputeRange()
{
  int i;
  float s;

  if ( this->GetMtime() > this->ComputeTime )
    {
    this->Range[0] =  LARGE_FLOAT;
    this->Range[1] =  -LARGE_FLOAT;
    for (i=0; i<this->NumScalars(); i++)
      {
      s = this->GetScalar(i);
      if ( s < this->Range[0] ) this->Range[0] = s;
      if ( s > this->Range[1] ) this->Range[1] = s;
      }

    this->ComputeTime.Modified();
    }
}

float *vlScalars::GetRange()
{
  this->ComputeRange();
  return this->Range;
}
