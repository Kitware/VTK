//
// Class to shrink PolyData
//
#ifndef __vlShrinkPolyData_h
#define __vlShrinkPolyData_h

#include "P2PF.h"

class vlShrinkPolyData : public vlPolyToPolyFilter 
{
public:
  vlShrinkPolyData() {this->ShrinkFactor = 0.5;};
  ~vlShrinkPolyData() {};
  void Execute();
  void SetShrinkFactor(float sf);
  float GetShrinkFactor();
private:
  float ShrinkFactor;
};

#endif


