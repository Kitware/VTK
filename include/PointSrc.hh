//
// Create cloud of points centered at specified point
//
#ifndef __vlPointSource_h
#define __vlPointSource_h

#include "PolySrc.hh"

class vlPointSource : public vlPolySource 
{
public:
  vlPointSource(int numPts=10);
  char *GetClassName() {return "vlPointSource";};

  vlSetClampMacro(NumPoints,int,1,LARGE_INTEGER);
  vlGetMacro(NumPoints,int);

  vlSetVector3Macro(Center,float);
  vlGetVectorMacro(Center,float);

  vlSetClampMacro(Radius,float,0.0,LARGE_FLOAT);
  vlGetMacro(Radius,float);

protected:
  void Execute();
  int NumPoints;
  float Center[3];
  float Radius;
};

#endif


