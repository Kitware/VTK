//
//  Scalar values, floating representation
//
//  Assumptions:
//
//
#include "FScalars.hh"

vlScalars *vlFloatScalars::MakeObject(int sze, int ext)
{
  return new vlFloatScalars(sze,ext);
}

vlFloatScalars& vlFloatScalars::operator=(const vlFloatScalars& fs)
{
  this->S = fs.S;
  return *this;
}
