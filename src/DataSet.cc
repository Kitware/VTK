//
// DataSet methods
//
#include "DataSet.h"

vlDataSet::vlDataSet ()
{
  this->PointData = 0;
}
vlDataSet::vlDataSet (const vlDataSet& ds)
{
  this->PointData = ds.PointData;
  this->PointData->Register((void *)this);
}

vlDataSet::~vlDataSet()
{
  vlDataSet::Initialize();
}

void vlDataSet::Initialize()
{
  if ( this->PointData )
    {
    this->PointData->UnRegister((void *)this);
    this->PointData = 0;
    }
};
void vlDataSet::SetPointData (vlPointData* pd)
{
  if ( this->PointData != pd )
    {
    if ( this->PointData ) this->PointData->UnRegister((void *)this);
    this->PointData = pd;
    this->Modified();
    }
}
vlPointData *vlDataSet::GetPointData()
{
  return this->PointData;
}

