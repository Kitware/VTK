//
// PolyToPolyFilter are filters that take PolyData in and generate PolyData
//
#ifndef PolyToPolyFilter_h
#define PolyToPolyFilter_h

#include "Params.h"
#include "PolyF.h"
#include "PolyData.h"

class PolyToPolyFilter : public PolyFilter, public PolyData {
public:
  void update();

};

#endif


