//
//  3D normals, floating representation
//
#include "FNormals.hh"

vlFloatNormals& vlFloatNormals::operator=(const vlFloatNormals& fn)
{
  if ( this->Debug ) cerr << "Assignment\n";

  this->N = fn.N;
  return *this;
}

void vlFloatNormals::GetNormals(vlIdList& ptId, vlFloatNormals& fn)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fn += (*this)[ptId[i]];
    }
}
