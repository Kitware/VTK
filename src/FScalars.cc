//
//  Scalar values, floating representation
//
//  Assumptions:
//
//
#include "FScalars.h"

FloatScalars::FloatScalars ()
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

}


FloatScalars::FloatScalars (const FloatScalars& fs)
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

  s = fs.s;
}


FloatScalars::FloatScalars(const int sz, const int ext=1000):s(sz,ext)
{
#ifdef DEBUG
  cout << "Constructor\n";
#endif

}


FloatScalars::~FloatScalars()
{
#ifdef DEBUG
  cout << "Destructor\n";
#endif

}

FloatScalars& FloatScalars::operator=(const FloatScalars& fs)
{
#ifdef DEBUG
  cout << "Assignment\n";
#endif

  s = fs.s;
  
  return *this;
}

void FloatScalars::operator+=(const FloatScalars& fs)
{
#ifdef DEBUG
  cout << "Add method\n";
#endif
  s += fs.s;

}

void FloatScalars::reset()
{
  s.reset();
}

int FloatScalars::numScalars()
{
  return (s.getMaxId()+1);
}

void FloatScalars::getScalars(IdList& ptId, FloatScalars& fs)
{
  for (int i=0; i<ptId.numIds(); i++)
  {
    fs += (*this)[ptId[i]];
  }
}
