//
// DataSet methods
//
#include "DataSet.h"

DataSet::DataSet ()
{
  pointData = 0;
}
DataSet::DataSet (const DataSet& ds)
{
  pointData = ds.pointData;
  pointData->Register((void *)this);
}

DataSet::~DataSet()
{
  DataSet::Initialize();
}

void DataSet::Initialize()
{
  if ( pointData )
  {
    pointData->UnRegister((void *)this);
    pointData = 0;
  }
};
