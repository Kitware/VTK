//
// Abstract class for specifying behavior of data sources
//
#ifndef PlaneSource_h
#define PlaneSource_h

#include "Params.h"
#include "PolySrc.h"

class PlaneSource : public PolySource {
public:
  PlaneSource() : xRes(1), yRes(1) {};
  PlaneSource(const int xR, const int yR) {xRes=xR; yRes=yR;};
  void execute();
  void setResolution(const int xR, const int yR);
  void getResolution(int& xR,int& yR) {xR=xRes; yR=yRes;};

private:
  int xRes;
  int yRes;
};

#endif


