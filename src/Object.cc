#include "Object.h"

vlObject::vlObject()
{
  RefCount = 0;
  Debug = 0;
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
  Debug = 1;
}

void vlObject::DebugOff()
{
  Debug = 0;
}

int vlObject::GetDebug()
{
  return Debug;
}
