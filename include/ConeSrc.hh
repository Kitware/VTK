//
// Create cone centered at origin. If resolution=0 line is created; if 
// resolution=1, single triangle; resolution=2, two crossed triangles; 
// resolution > 2, 3D cone.
//
#ifndef __ConeSource_h
#define __ConeSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_VERTS

class vlConeSource : public vlPolySource 
{
public:
  vlConeSource() : Resolution(6),Height(1.0),Radius(0.5),Capping(1) {};
  vlConeSource(int res) {this->Resolution=res;};
  void Execute();
  void SetHeight(float h);
  float GetHeight();
  void SetRadius(float r);
  float GetRadius();
  void SetResolution(int res);
  int GetResolution();
  void SetCapping(int flag);
  int GetCapping();

protected:
  float Height;
  float Radius;
  int Resolution;
  int Capping;

};

#endif


