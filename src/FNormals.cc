//
//  3D normals, floating representation
//
#include "FNormals.hh"

vlFloatNormals::vlFloatNormals ()
{
  if ( this->Debug ) cerr << "Constructor\n";
}

vlFloatNormals::vlFloatNormals (const vlFloatNormals& fn)
{
  if ( this->Debug ) cerr << "Constructor\n";
  this->N= fn.N;
}

vlFloatNormals::vlFloatNormals(const int sz, const int ext):N(3*sz,3*ext)
{
  if ( this->Debug ) cerr << "Constructor\n";
}

vlFloatNormals::~vlFloatNormals()
{
  if ( this->Debug ) cerr << "Destructor\n";
}

vlFloatNormals& vlFloatNormals::operator=(const vlFloatNormals& fn)
{
  if ( this->Debug ) cerr << "Assignment\n";

  this->N = fn.N;
  return *this;
}

void vlFloatNormals::operator+=(const vlFloatNormals& fn)
{
  if ( this->Debug ) cerr << "Add method\n";
  this->N += fn.N;
}

void vlFloatNormals::Reset()
{
  this->N.Reset();
}

int vlFloatNormals::NumNormals()
{
  return (this->N.GetMaxId()+1)/3;
}

void vlFloatNormals::GetNormals(vlIdList& ptId, vlFloatNormals& fn)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fn += (*this)[ptId[i]];
    }
}
