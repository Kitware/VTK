
#include "PolySrc.hh"

void vlPolySource::Update()
{
  vlSource::Update();
}

void vlPolySource::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlPolySource::GetClassName()))
    {
    this->PrintWatchOn(); // watch for multiple inheritance
    
    vlPolyData::PrintSelf(os,indent);
    vlSource::PrintSelf(os,indent);
    
    this->PrintWatchOff(); // stop worrying about it now
    }
}
