//
// PolyFilter takes PolyData as input
//
#ifndef __vlPolyFilter_h
#define __vlPolyFilter_h

#include "Filter.h"
#include "PolyData.h"

class vlPolyFilter : public vlFilter {
public:
  vlPolyFilter() : Input(0) {};
  virtual ~vlPolyFilter();
  virtual void SetInput(vlPolyData *in);
  virtual vlPolyData* GetInput();
  virtual void Execute();
  virtual void Update();
protected:
  vlPolyData *Input;

};

#endif


