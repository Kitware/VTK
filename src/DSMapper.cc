//
// Methods for polygon mapper
//
#include "DSMapper.hh"

vlDataSetMapper::vlDataSetMapper()
{
  this->Input = 0;
  this->Mapper = 0;
}

vlDataSetMapper::~vlDataSetMapper()
{
  if ( this->Input )
    {
    this->Input->UnRegister((void *)this);
    }

  if ( this->Mapper )
    {
    this->Mapper->UnRegister((void *)this);
    }
}

void vlDataSetMapper::SetInput(vlDataSet *in)
{
  if (in != this->Input )
    {
    this->Input = in;
    this->Input->Register((void *)this);
    this->Modified();
    }
}
vlDataSet* vlDataSetMapper::GetInput()
{
  return this->Input;
}

//
// Receives from Actor -> maps data to primitives
//
void vlDataSetMapper::Render(vlRenderer *ren)
{
  vlMapper *mapper;
//
// make sure that we've been properly initialized
//
  if ( !this->Input )
    {
    cerr << this->GetClassName() << ": No input!\n";
    return;
    }
//
// Now can create appropriate mapper
//
  if ( !(mapper = this->Input->MakeMapper()) )
    {
    cerr << this->GetClassName() << ": Cannot map type: " 
         << this->Input->GetClassName() <<"\n";
    return;
    }
  if ( mapper != this->Mapper ) 
    {
    *mapper = *this; // Update lookup table, etc.
    if (this->Mapper) this->Mapper->UnRegister((void *)this);
    this->Mapper = mapper;
    this->Mapper->Register((void *)this);
    }
 
  this->Mapper->Render(ren);
}

