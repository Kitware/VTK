//
// Abstract class for specifying shrinkFilter behaviour
//
#ifndef ShrinkFilter_h
#define ShrinkFilter_h

#include "Params.h"
#include "DataSetF.h"
#include "PolyData.h"

class ShrinkFilter : public DataSetFilter, public PolyData {
public:
  ShrinkFilter(const float sf=0.5) {shrinkFactor = sf;};
  virtual ~ShrinkFilter() {};
  virtual void execute();
private:
  float shrinkFactor;
};

#endif


