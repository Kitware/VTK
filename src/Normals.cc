//
//  3D normals, abstract representation
//
#include "Normals.hh"
#include "IdList.hh"
#include "FNormals.hh"

void vlNormals::GetNormals(vlIdList& ptId, vlFloatNormals& fp)
{
  for (int i=0; i<ptId.NumIds(); i++)
    {
    fp.InsertNormal(i,this->GetNormal(ptId[i]));
    }
}
