//
//  Scalar values, floating representation
//
//  Assumptions:
//
//
#include "FScalars.hh"

vlFloatScalars::vlFloatScalars ()
{
  if ( this->Debug ) cerr << "Constructor\n";
}

vlFloatScalars::vlFloatScalars (const vlFloatScalars& fs)
{
  if ( this->Debug ) cerr << "Constructor\n";

  this->S = fs.S;
}


vlFloatScalars::vlFloatScalars(const int sz, const int ext=1000):S(sz,ext)
{
  if ( this->Debug ) cerr << "Constructor\n";
}


vlFloatScalars::~vlFloatScalars()
{
  if ( this->Debug ) cerr << "Destructor\n";
}

vlFloatScalars& vlFloatScalars::operator=(const vlFloatScalars& fs)
{
  if ( this->Debug ) cerr << "Assignment\n";

  this->S = fs.S;
  return *this;
}

void vlFloatScalars::operator+=(const vlFloatScalars& fs)
{
  if ( this->Debug ) cerr << "Add method\n";
  this->S += fs.S;
}

void vlFloatScalars::Reset()
{
  this->S.Reset();
}

int vlFloatScalars::NumScalars()
{
  return (this->S.GetMaxId()+1);
}

void vlFloatScalars::GetScalars(vlIdList& ptId, vlFloatScalars& fs)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fs += (*this)[ptId[i]];
    }
}
