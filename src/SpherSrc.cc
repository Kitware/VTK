//
// Methods for Sphere generator
//
#include <math.h>
#include "SpherSrc.hh"

vlSphereSource::vlSphereSource(int res)
{
  res = (res < 0 ? 0 : res);
  this->Resolution = res;
  this->Radius = 0.5;
}

void vlSphereSource::Execute()
{

}
