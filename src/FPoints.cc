//
//  3D points, floating representation
//
//
#include "FPoints.hh"

vlPoints *vlFloatPoints::MakeObject(int sze, int ext)
{
  return new vlFloatPoints(sze,ext);
}

vlFloatPoints& vlFloatPoints::operator=(const vlFloatPoints& fp)
{
  this->P = fp.P;
  return *this;
}

