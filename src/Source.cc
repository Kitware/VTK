#include "Source.h"

void vlSource::Execute()
{
  cerr << "Executing Source\n";
}

void vlSource::Update()
{
  // Make sure virtual getMtime method is called since subclasses will overload
  if ( Mtime > this->ExecuteTime )
    {
    if ( this->StartMethod ) (*this->StartMethod)();
    this->Execute();
    this->ExecuteTime.Modified();
    if ( this->EndMethod ) (*this->EndMethod)();
    }
}

void vlSource::SetStartMethod(void (*f)())
{
  if ( f != this->StartMethod )
    {
    this->StartMethod = f;
    this->Modified();
    }
}

void vlSource::SetEndMethod(void (*f)())
{
  if ( f != this->EndMethod )
    {
    this->EndMethod = f;
    this->Modified();
    }
}

