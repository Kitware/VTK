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
    this->DataSet = this->Input->MakeObject();
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

vlMapper *vlDataSetToDataSetFilter::MakeMapper(vlDataSet *ds)
{
//
// A little tricky because mappers must be of concrete type, but this class 
// deals at abstract level of DataSet.  Depending upon Input member of this 
// filter, mapper may change.  Hence need to anticipate change in Input and 
// create new mappers as necessary.
//
  vlMapper *mapper;

  this->Update(); // Create DataSet of proper type
  mapper = this->DataSet->MakeMapper(this->DataSet);
  if ( !this->Mapper || mapper != this->Mapper )
    {
    if (this->Mapper) this->Mapper->UnRegister((void *)this);
    this->Mapper = mapper;
    this->Mapper->Register((void *)this);
    }
  return this->Mapper;
}
