//
//  3D TCoords, floating representation
//
//  Assumptions:
//
//
#include "FTCoords.h"

FloatTCoords::FloatTCoords ():dim(2)
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

}


FloatTCoords::FloatTCoords (const FloatTCoords& ftc)
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

  tc = ftc.tc;
  dim = ftc.dim;
}


FloatTCoords::FloatTCoords(const int sz, const int d, const int ext):
dim(d), tc(d*sz,d*ext)
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

}


FloatTCoords::~FloatTCoords()
{
#ifdef DEBUG
  cout << "Destructor\n";
#endif

}

FloatTCoords& FloatTCoords::operator=(const FloatTCoords& ftc)
{
#ifdef DEBUG
  cout << "Assignment\n";
#endif

  tc = ftc.tc;
  dim = ftc.dim;
  
  return *this;
}

void FloatTCoords::operator+=(const FloatTCoords& ftc)
{
#ifdef DEBUG
  cout << "Add method\n";
#endif
  tc += ftc.tc;
}

void FloatTCoords::reset()
{
  tc.reset();
}

int FloatTCoords::numTCoords()
{
  return (tc.getMaxId()+1)/dim;
}

void FloatTCoords::getTCoords(IdList& ptId, FloatTCoords& ftc)
{
  for (int i=0; i<ptId.numIds(); i++)
  {
    ftc += (*this)[ptId[i]];
  }
}
