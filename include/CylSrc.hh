//
// Created cylinder centered at origin
//
#ifndef __vlCylinderSource_h
#define __vlCylinderSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_VERTS

class vlCylinderSource : public vlPolySource 
{
public:
  vlCylinderSource(int res=6);
  char *GetClassName() {return "vlCylinderSource";};
  void Execute();

  vlSetClampMacro(Height,float,0.0,LARGE_FLOAT)
  vlGetMacro(Height,float);

  vlSetClampMacro(Radius,float,0.0,LARGE_FLOAT)
  vlGetMacro(Radius,float);

  vlSetClampMacro(Resolution,int,0,MAX_RESOLUTION)
  vlGetMacro(Resolution,int);

  vlSetMacro(Capping,int);
  vlGetMacro(Capping,int);
  vlBooleanMacro(Capping,int);

protected:
  float Height;
  float Radius;
  int Resolution;
  int Capping;

};

#endif


