//
// Abstract class for specifying behavior of data sources
//
#ifndef __vlPlaneSource_h
#define __vlPlaneSource_h

#include "PolySrc.h"

class vlPlaneSource : public vlPolySource {
public:
  vlPlaneSource() : XRes(1), YRes(1) {};
  vlPlaneSource(const int xR, const int yR) {XRes=xR; YRes=yR;};
  void Execute();
  void SetResolution(const int xR, const int yR);
  void GetResolution(int& xR,int& yR) {xR=this->XRes; yR=this->YRes;};

private:
  int XRes;
  int YRes;
};

#endif


