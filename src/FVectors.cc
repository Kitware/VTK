//
//  3D vectors, floating representation
//
#include "FVectors.hh"

vlVectors *vlFloatVectors::MakeObject(int sze, int ext)
{
  return new vlFloatVectors(sze,ext);
}

vlFloatVectors& vlFloatVectors::operator=(const vlFloatVectors& fv)
{
  this->V = fv.V;
  return *this;
}

