#include "PolyF.hh"

vlPolyFilter::~vlPolyFilter()
{
  if ( this->Input != NULL )
    {
    this->Input->UnRegister(this);
    }
}

void vlPolyFilter::Update()
{
  // make sure input is available
  if ( !this->Input )
    {
    vlErrorMacro(<< "No input!\n");
    return;
    }

  // prevent chasing our tail
  if (this->Updating) return;

  this->Updating = 1;
  this->Input->Update();
  this->Updating = 0;

  if (this->Input->GetMTime() > this->GetMTime() || this->GetMTime() > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)();
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)();
    }
}

void vlPolyFilter::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlPolyFilter::GetClassName()))
    {
    vlFilter::PrintSelf(os,indent);

    if ( this->Input )
      {
      os << indent << "Input: (" << this->Input << ")\n";
      }
    else
      {
      os << indent << "Input: (none)\n";
      }
   }
}
