//
// Methods for abstract class mapper
//
#include "Mapper.hh"

vlMapper::~vlMapper()
{
  if (this->Lut)
    {
    this->Lut->UnRegister((void *)this);
    }
}

void vlMapper::SetStartRender(void (*f)())
{
  if ( f != this->StartRender )
    {
    this->StartRender = f;
    this->Modified();
    }
}

void vlMapper::SetEndRender(void (*f)())
{
  if ( f != this->EndRender )
    {
    this->EndRender = f;
    this->Modified();
    }
}



