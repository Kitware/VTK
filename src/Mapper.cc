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
void vlMapper::SetLookupTable(vlLookupTable *lt) 
{
  if ( lt != this->Lut )
    {
    this->Lut = lt;
    this->Lut->Register((void *)this);
    this->Modified();
    }
}

void vlMapper::SetScalarsVisible(int flag)
{
  if ( flag != this->ScalarsVisible )
    {
    this->ScalarsVisible = flag;
    this->Modified();
    }
}

int vlMapper::GetScalarsVisible()
{
  return this->ScalarsVisible;
}



