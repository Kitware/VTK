//
//  3D vectors, floating representation
//
//  Assumptions:
//
//
#include "FVectors.h"

vlFloatVectors::vlFloatVectors ()
{
  if ( this->Debug ) cerr << "Constructor\n";
}

vlFloatVectors::vlFloatVectors (const vlFloatVectors& fv)
{
  if ( this->Debug ) cerr << "Constructor\n";

  this->V = fv.V;
}

vlFloatVectors::vlFloatVectors(const int sz, const int ext=1000):V(3*sz,3*ext)
{
  if ( this->Debug ) cerr << "Constructor\n";
}

vlFloatVectors::~vlFloatVectors()
{
  if ( this->Debug ) cerr << "Destructor\n";
}

vlFloatVectors& vlFloatVectors::operator=(const vlFloatVectors& fv)
{
  if ( this->Debug ) cerr << "Assignment\n";

  this->V = fv.V;
  return *this;
}

void vlFloatVectors::operator+=(const vlFloatVectors& fv)
{
  if ( this->Debug ) cerr << "Add method\n";
  this->V += fv.V;
}

void vlFloatVectors::Reset()
{
  this->V.Reset();
}

int vlFloatVectors::NumVectors()
{
  return (this->V.GetMaxId()+1)/3;
}

void vlFloatVectors::GetVectors(vlIdList& ptId, vlFloatVectors& fv)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fv += (*this)[ptId[i]];
    }
}
