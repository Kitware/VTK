//
// Abstract class for specifying shrinkFilter behaviour
//
#ifndef __vlShrinkFilter_h
#define __vlShrinkFilter_h

#include "DataSetF.hh"
#include "PolyData.hh"

class vlShrinkFilter : public vlDataSetFilter, public vlPolyData 
{
public:
  vlShrinkFilter(const float sf=0.5) {this->ShrinkFactor = sf;};
  ~vlShrinkFilter() {};
  void Execute();
private:
  float ShrinkFactor;
};

#endif


