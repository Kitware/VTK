//
// Abstract class for objects that filter DataSets
//
#include "DataSetF.hh"

vlDataSetFilter::vlDataSetFilter()
{
  this->Input = 0;
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

  if (this->Input->GetMtime() > this->Mtime || this->Mtime > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)();
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)();
    }
}


