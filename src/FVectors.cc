//
//  3D vectors, floating representation
//
//  Assumptions:
//
//
#include "FVectors.h"

FloatVectors::FloatVectors ()
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

}


FloatVectors::FloatVectors (const FloatVectors& fv)
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

  v = fv.v;
}


FloatVectors::FloatVectors(const int sz, const int ext=1000):v(3*sz,3*ext)
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

}


FloatVectors::~FloatVectors()
{
#ifdef DEBUG
  cout << "Destructor\n";
#endif

}

FloatVectors& FloatVectors::operator=(const FloatVectors& fv)
{
#ifdef DEBUG
  cout << "Assignment\n";
#endif

  v = fv.v;
  
  return *this;
}

void FloatVectors::operator+=(const FloatVectors& fv)
{
#ifdef DEBUG
  cout << "Add method\n";
#endif
  v += fv.v;
}

void FloatVectors::reset()
{
  v.reset();
}

int FloatVectors::numVectors()
{
  return (v.getMaxId()+1)/3;
}

void FloatVectors::getVectors(IdList& ptId, FloatVectors& fv)
{
  for (int i=0; i<ptId.numIds(); i++)
  {
    fv += (*this)[ptId[i]];
  }
}
