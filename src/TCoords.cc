//
//  3D TCoords, abstract representation
//
#include "TCoords.hh"
#include "IdList.hh"
#include "FTCoords.hh"

void vlTCoords::GetTCoords(vlIdList& ptId, vlFloatTCoords& ftc)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    ftc.InsertTCoord(i,this->GetTCoord(ptId[i]));
    }
}
