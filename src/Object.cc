#include "Object.hh"

vlObject::vlObject()
{
  this->RefCount = 0;
  this->Debug = 0;
}

vlObject::~vlObject() 
{
  if (this->RefCount > 0)
    {
    cerr << this->GetClassName()
         <<": Trying to delete object with non-zero refCount\n";
    }
  if (this->Debug)
    {
    cerr << this->GetClassName() << " (" << this << "):"
         <<" Destructing!\n";
    }
}

void vlObject::Register(void *p)
{
  this->RefCount++;
  if ( this->Debug )
    {
    cerr << this->GetClassName() << " (" << this << "):"
         << " Registered by " << (void *)p << "\n";
    }
}

void vlObject::UnRegister(void *p)
{
  if ( this->Debug )
    {
    cerr << this->GetClassName() << " (" << this << "):"
         << " UnRegistered by " << (void *)p << "\n";
    }
  if (--this->RefCount <= 0) delete this;
}


void vlObject::PrintHeader(ostream& os)
{
  os << this->GetClassName() << " (" << this << ")\n";
  os << "    Debug state: " << this->Debug << "\n";
  os << "    Modified Time: " << this->GetMtime() << "\n";
  os << "    Reference Count: " << this->RefCount << "\n";}

void vlObject::PrintSelf(ostream& os)
{
  ;
}

void vlObject::PrintTrailer(ostream& os)
{
  os << "\n";
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
