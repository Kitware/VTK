//
// Created cylinder centered at origin
//
#ifndef __CylinderSource_h
#define __CylinderSource_h

#include "PolySrc.hh"

#define MAX_RESOLUTION MAX_VERTS

class vlCylinderSource : public vlPolySource 
{
public:
  vlCylinderSource() : Resolution(6),Height(1.0),Radius(0.5),Capping(1) {};
  vlCylinderSource(int res) {this->Resolution=res;};
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


