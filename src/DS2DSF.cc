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

void vlDataSetToDataSetFilter::Update()
{
  vlPointData *pd;

  vlDataSetFilter::Update();
  // Following copies data from this filter to internal dataset
  pd = this->DataSet->GetPointData();
  *pd = this->PointData;
}

void vlDataSetToDataSetFilter::Initialize()
{
  if ( this->Input )
    {
    this->DataSet->UnRegister((void *)this);
    // copies input geometry to internal data set
    this->DataSet = this->Input->MakeObject(); 
    this->DataSet->Register((void *)this);
    }
  else
    {
    return;
    }
}

vlMapper *vlDataSetToDataSetFilter::MakeMapper()
{
//
// A little tricky because mappers must be of concrete type, but this class 
// deals at abstract level of DataSet.  Depending upon Input member of this 
// filter, mapper may change.  Hence need to anticipate change in Input and 
// create new mappers as necessary.
//
  vlMapper *mapper;

  vlDataSetToDataSetFilter::Update(); // compiler bug, had to hard code call
  mapper = this->DataSet->MakeMapper();
  if ( !this->Mapper || mapper != this->Mapper )
    {
    if (this->Mapper) this->Mapper->UnRegister((void *)this);
    this->Mapper = mapper;
    this->Mapper->Register((void *)this);
    }
  return this->Mapper;
}
