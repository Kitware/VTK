//
// Abstract class for specifying shrinkFilter behaviour
//
#ifndef ShrinkPolyData_h
#define ShrinkPolyData_h

#include "Params.h"
#include "PolyF.h"
#include "PolyData.h"

class ShrinkPolyData : public PolyFilter, public PolyData {
public:
  ShrinkPolyData(const float sf=0.5) {shrinkFactor = sf;};
  virtual ~ShrinkPolyData() {};
  virtual void execute();
private:
  float shrinkFactor;
};

#endif


