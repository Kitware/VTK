//
//  3D TCoords, floating representation
//
#include "FTCoords.hh"

vlTCoords *vlFloatTCoords::MakeObject(int sze, int d, int ext)
{
  return new vlFloatTCoords(sze,d,ext);
}

vlFloatTCoords& vlFloatTCoords::operator=(const vlFloatTCoords& ftc)
{
  if ( this->Debug ) cerr << "Assignment\n";

  this->TC = ftc.TC;
  this->Dimension = ftc.Dimension;
  
  return *this;
}

