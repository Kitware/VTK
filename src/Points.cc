//
//  3D Points, abstract representation
//
#include "Points.hh"
#include "FPoints.hh"
#include "IdList.hh"

void vlPoints::GetPoints(vlIdList& ptId, vlFloatPoints& fp)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fp.InsertPoint(i,this->GetPoint(ptId[i]));
    }
}
