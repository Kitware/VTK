//
// Create nxm array of quadrilaterals
//
#ifndef __vlPlaneSource_h
#define __vlPlaneSource_h

#include "PolySrc.hh"

class vlPlaneSource : public vlPolySource 
{
public:
  vlPlaneSource() : XRes(1), YRes(1) {};
  vlPlaneSource(const int xR, const int yR) {XRes=xR; YRes=yR;};
  void SetResolution(const int xR, const int yR);
  void GetResolution(int& xR,int& yR) {xR=this->XRes; yR=this->YRes;};
  char *GetClassName() {return "vlPlaneSource";};

protected:
  void Execute();
  int XRes;
  int YRes;
};

#endif


