//
// Created cube centered at origin
//
#ifndef __vlCubeSource_h
#define __vlCubeSource_h

#include "PolySrc.hh"

class vlCubeSource : public vlPolySource 
{
public:
  vlCubeSource(float xL=1.0, float yL=1.0, float zL=1.0);
  char *GetClassName() {return "vlCubeSource";};
  void Execute();

  vlSetClampMacro(XLength,float,0.0,LARGE_FLOAT);
  vlGetMacro(XLength,float);

  vlSetClampMacro(YLength,float,0.0,LARGE_FLOAT);
  vlGetMacro(YLength,float);

  vlSetClampMacro(ZLength,float,0.0,LARGE_FLOAT);
  vlGetMacro(ZLength,float);

protected:
  float XLength;
  float YLength;
  float ZLength;
};

#endif


