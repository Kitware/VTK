//
// Created Line centered at origin
//
#ifndef __LineSource_h
#define __LineSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_VERTS

class vlLineSource : public vlPolySource 
{
public:
  vlLineSource(int res=1);
  void Execute();
  void SetPoint1(float *x);
  void GetPoint1(float* &x);
  void SetPoint2(float *x);
  void GetPoint2(float* &x);
  void SetResolution(int res);
  int GetResolution();

protected:
  float Pt1[3];
  float Pt2[3];
  int Resolution;
};

#endif


