//
//  3D TCoords, floating representation
//
#include "FTCoords.hh"

vlFloatTCoords& vlFloatTCoords::operator=(const vlFloatTCoords& ftc)
{
  if ( this->Debug ) cerr << "Assignment\n";

  this->TC = ftc.TC;
  this->Dimension = ftc.Dimension;
  
  return *this;
}

void vlFloatTCoords::GetTCoords(vlIdList& ptId, vlFloatTCoords& ftc)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    ftc += (*this)[ptId[i]];
    }
}
