//
// Abstract class for specifying shrinkFilter behaviour
//
#ifndef ShrinkPolyData_h
#define ShrinkPolyData_h

#include "Params.h"
#include "P2PF.h"

class ShrinkPolyData : public PolyToPolyFilter {
public:
  ShrinkPolyData() {shrinkFactor = 0.5;};
  virtual ~ShrinkPolyData() {};
  virtual void execute();
  void setShrinkFactor(float sf);
  float getShrinkFactor();
private:
  float shrinkFactor;
};

#endif


