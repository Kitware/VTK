//
//  3D vectors, floating representation
//
//  Assumptions:
//
//
#include "FVectors.hh"

vlFloatVectors& vlFloatVectors::operator=(const vlFloatVectors& fv)
{
  this->V = fv.V;
  return *this;
}

void vlFloatVectors::GetVectors(vlIdList& ptId, vlFloatVectors& fv)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fv += (*this)[ptId[i]];
    }
}
