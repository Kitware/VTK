//
// Created cube centered at origin
//
#ifndef __vlCubeSource_h
#define __vlCubeSource_h

#include "PolySrc.hh"

class vlCubeSource : public vlPolySource 
{
public:
  vlCubeSource();
  vlCubeSource(float xL, float yL, float zL);
  void Execute();
  float GetXLength();
  void SetXLength(float xL);
  float GetYLength();
  void SetYLength(float yL);
  float GetZLength();
  void SetZLength(float zL);

protected:
  float XLength;
  float YLength;
  float ZLength;
};

#endif


