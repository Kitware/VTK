//
//  Scalar values, floating representation
//
//  Assumptions:
//
//
#include "FScalars.hh"

vlFloatScalars& vlFloatScalars::operator=(const vlFloatScalars& fs)
{
  this->S = fs.S;
  return *this;
}


void vlFloatScalars::GetScalars(vlIdList& ptId, vlFloatScalars& fs)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fs += (*this)[ptId[i]];
    }
}
