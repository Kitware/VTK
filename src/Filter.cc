//
//
//
#include "Filter.h"

void vlFilter::SetStartMethod(void (*f)())
{
  if ( f != this->StartMethod )
    {
    this->StartMethod = f;
    this->Modified();
    }
}

void vlFilter::SetEndMethod(void (*f)())
{
  if ( f != this->EndMethod )
    {
    this->EndMethod = f;
    this->Modified();
    }
}
