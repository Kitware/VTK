//
// Created Line centered at origin
//
#ifndef __vlLineSource_h
#define __vlLineSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_VERTS

class vlLineSource : public vlPolySource 
{
public:
  vlLineSource(int res=1);
  char *GetClassName() {return "vlLineSource";};
  void Execute();

  vlSetVector3Macro(Pt1,float);
  vlGetVectorMacro(Pt1,float);

  vlSetVector3Macro(Pt2,float);
  vlGetVectorMacro(Pt2,float);

  vlSetClampMacro(Resolution,int,1,LARGE_INTEGER);
  vlGetMacro(Resolution,int);

protected:
  float Pt1[3];
  float Pt2[3];
  int Resolution;
};

#endif


