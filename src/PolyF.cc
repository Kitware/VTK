#include "PolyF.hh"

vlPolyFilter::~vlPolyFilter()
{
  if ( this->Input != 0 )
    {
    this->Input->UnRegister((void *)this);
    }
}

void vlPolyFilter::Execute()
{
  cerr << "Executing PolyFilter\n";
}

void vlPolyFilter::Update()
{
  // make sure input is available
  if ( !this->Input )
    {
    cerr << "No input available for PolyFilter\n";
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Updating = 0;

  if (this->Input->GetMtime() > this->GetMtime() || this->GetMtime() > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)();
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)();
    }
}
