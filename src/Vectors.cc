//
//  3D vectors, abstract representation
//
#include "Vectors.hh"
#include "FVectors.hh"
#include "IdList.hh"

void vlVectors::GetVectors(vlIdList& ptId, vlFloatVectors& fp)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fp.InsertVector(i,this->GetVector(ptId[i]));
    }
}
