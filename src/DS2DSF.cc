#include "DS2DSF.hh"
#include "PolyData.hh"

vlDataSetToDataSetFilter::vlDataSetToDataSetFilter()
{
  // prevents dangling reference to DataSet
  this->DataSet = new vlPolyData;
  this->DataSet->Register((void *)this);
}

vlDataSetToDataSetFilter::~vlDataSetToDataSetFilter()
{
  this->DataSet->UnRegister((void *)this);
}

void vlDataSetToDataSetFilter::Initialize()
{
  if ( this->Input )
    {
    this->DataSet->UnRegister((void *)this);
    this->DataSet = this->Input->CopySelf();
    this->DataSet->Register((void *)this);
    }
  else
    {
    return;
    }
}

void vlDataSetToDataSetFilter::Update()
{
  vlDataSetFilter::Update();
}

