//
// Abstract class for specifying behavior of data sources
//
#ifndef PolySource_h
#define PolySource_h

#include "Params.h"
#include "Source.h"
#include "PolyData.h"

class PolySource : public Source, public PolyData {
public:
  virtual void update();

};

#endif


