//
// Abstract class for specifying shrinkFilter behaviour
//
#ifndef __vlShrinkFilter_h
#define __vlShrinkFilter_h

#include "DataSetF.h"
#include "PolyData.h"

class vlShrinkFilter : public vlDataSetFilter, public vlPolyData {
public:
  vlShrinkFilter(const float sf=0.5) {this->ShrinkFactor = sf;};
  virtual ~vlShrinkFilter() {};
  virtual void Execute();
private:
  float ShrinkFactor;
};

#endif


