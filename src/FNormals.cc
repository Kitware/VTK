//
//  3D normals, floating representation
//
//  Assumptions:
//
//
#include "FNormals.h"

FloatNormals::FloatNormals ()
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

}


FloatNormals::FloatNormals (const FloatNormals& fn)
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

  n = fn.n;
}


FloatNormals::FloatNormals(const int sz, const int ext):n(3*sz,3*ext)
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

}


FloatNormals::~FloatNormals()
{
#ifdef DEBUG
  cout << "Destructor\n";
#endif

}

FloatNormals& FloatNormals::operator=(const FloatNormals& fn)
{
#ifdef DEBUG
  cout << "Assignment\n";
#endif

  n = fn.n;
  
  return *this;
}

void FloatNormals::operator+=(const FloatNormals& fn)
{
#ifdef DEBUG
  cout << "Add method\n";
#endif
  n += fn.n;
}

void FloatNormals::reset()
{
  n.reset();
}

int FloatNormals::numNormals()
{
  return (n.getMaxId()+1)/3;
}

void FloatNormals::getNormals(IdList& ptId, FloatNormals& fn)
{
  for (int i=0; i<ptId.numIds(); i++)
  {
    fn += (*this)[ptId[i]];
  }
}
