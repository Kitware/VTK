//
// Abstract class for objects that filter DataSets
//
#include "DataSetF.hh"

void vlDataSetFilter::SetInput(vlDataSet *in)
{
  if (in != this->Input )
    {
    this->Input = in;
    this->Input->Register((void *)this);
    this->Modified();
    }
}
vlDataSet* vlDataSetFilter::GetInput()
{
  return this->Input;
}

vlDataSetFilter::~vlDataSetFilter()
{
  if ( this->Input != 0 )
    {
    this->Input->UnRegister((void *)this);
    }
}

void vlDataSetFilter::Execute()
{
  cerr << "Executing DataSetFilter\n";
}

void vlDataSetFilter::Update()
{
  // make sure input is available
  if ( !this->Input )
    {
    cerr << "No input available for DataSetFilter\n";
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Updating = 0;

  if (this->Input->Mtime > this->Mtime )
    {
    if ( this->StartMethod ) (*this->StartMethod)();
    this->Execute();
    this->Modified();
    if ( this->EndMethod ) (*this->EndMethod)();
    }
}


