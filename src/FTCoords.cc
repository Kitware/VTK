//
//  3D TCoords, floating representation
//
#include "FTCoords.h"

vlFloatTCoords::vlFloatTCoords ():Dim(2)
{
  if ( this->Debug ) cerr << "Constructor\n";
}

vlFloatTCoords::vlFloatTCoords (const vlFloatTCoords& ftc)
{
  if ( this->Debug ) cerr << "Constructor\n";

  this->TC = ftc.TC;
  this->Dim = ftc.Dim;
}

vlFloatTCoords::vlFloatTCoords(const int sz, const int d, const int ext):
Dim(d), TC(d*sz,d*ext)
{
  if ( this->Debug ) cerr << "Constructor\n";
}


vlFloatTCoords::~vlFloatTCoords()
{
  if ( this->Debug ) cerr << "Destructor\n";
}

vlFloatTCoords& vlFloatTCoords::operator=(const vlFloatTCoords& ftc)
{
  if ( this->Debug ) cerr << "Assignment\n";

  this->TC = ftc.TC;
  this->Dim = ftc.Dim;
  
  return *this;
}

void vlFloatTCoords::operator+=(const vlFloatTCoords& ftc)
{
  if ( this->Debug ) cerr << "Add method\n";
  this->TC += ftc.TC;
}

void vlFloatTCoords::Reset()
{
  this->TC.Reset();
}

int vlFloatTCoords::NumTCoords()
{
  return (this->TC.GetMaxId()+1)/this->Dim;
}

void vlFloatTCoords::GetTCoords(vlIdList& ptId, vlFloatTCoords& ftc)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    ftc += (*this)[ptId[i]];
    }
}
