//
// Methods for abstract class mapper
//
#include "Mapper.hh"

vlMapper::vlMapper()
{
  this->StartRender = 0;
  this->EndRender = 0;
  this->LookupTable = 0;
  this->ScalarsVisible = 1;
  this->ScalarRange[0] = 0.0; this->ScalarRange[1] = 1.0;
}

vlMapper::~vlMapper()
{
  if (this->LookupTable)
    {
    this->LookupTable->UnRegister((void *)this);
    }
}

void vlMapper::operator=(const vlMapper& m)
{
  if (this->LookupTable) this->LookupTable->UnRegister((void *)this);
  this->LookupTable = m.LookupTable;
  if (this->LookupTable) this->LookupTable->Register((void *)this);

  this->ScalarsVisible = m.ScalarsVisible;
  this->ScalarRange[0] = m.ScalarRange[0];
  this->ScalarRange[1] = m.ScalarRange[1];

  this->StartRender = m.StartRender;
  this->EndRender = m.EndRender;

  this->Modified();
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



