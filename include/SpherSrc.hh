//
// Create sphere centered at origin. Resolution=0 means octahedron which 
// is recursively subdivided for each resolution increase.
//
#ifndef __vlSphereSource_h
#define __vlSphereSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION 8

class vlSphereSource : public vlPolySource 
{
public:
  vlSphereSource(int res=2);
  char *GetClassName() {return "vlSphereSource";};

  vlSetClampMacro(Radius,float,0.0,LARGE_FLOAT)
  vlGetMacro(Radius,float);

  vlSetClampMacro(Resolution,int,0,MAX_RESOLUTION)
  vlGetMacro(Resolution,int);

protected:
  void Execute();
  float Radius;
  int Resolution;

};

#endif


