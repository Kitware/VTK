//
//  Scalars, abstract representation
//
#include "Scalars.hh"
#include "FScalars.hh"
#include "IdList.hh"

void vlScalars::GetScalars(vlIdList& ptId, vlFloatScalars& fp)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fp.InsertScalar(i,this->GetScalar(ptId[i]));
    }
}
