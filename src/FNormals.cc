//
//  3D normals, floating representation
//
#include "FNormals.hh"

vlNormals *vlFloatNormals::MakeObject(int sze, int ext)
{
  return new vlFloatNormals(sze,ext);
}

vlFloatNormals& vlFloatNormals::operator=(const vlFloatNormals& fn)
{
  this->N = fn.N;
  return *this;
}

