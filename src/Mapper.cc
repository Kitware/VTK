//
// Methods for abstract class mapper
//
#include "Mapper.hh"

vlMapper::vlMapper()
{
  this->StartRender = 0;
  this->EndRender = 0;
  this->Lut = 0;
  this->ScalarsVisible = 1;
  this->ScalarRange[0] = 0.0; this->ScalarRange[1] = 1.0;
}

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



