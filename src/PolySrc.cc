
#include "PolySrc.hh"

void vlPolySource::Update()
{
  vlSource::Update();
}

void vlPolySource::PrintSelf(ostream& os, vlIndent indent)
{
  vlPolyData::PrintSelf(os,indent);
  vlSource::PrintSelf(os,indent);
}
