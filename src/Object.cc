#include "Object.hh"

vlObject::vlObject()
{
  this->RefCount = 0;
  this->Debug = 0;
}

vlObject::~vlObject() 
{
  if (RefCount > 0)
    {
    cerr << "Trying to delete object with non-zero refCount\n";
    }
}

void vlObject::DebugOn()
{
  this->Debug = 1;
}

void vlObject::DebugOff()
{
  this->Debug = 0;
}

int vlObject::GetDebug()
{
  return this->Debug;
}
