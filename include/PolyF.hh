//
// PolyFilter takes PolyData as input
//
#ifndef PolyFilter_h
#define PolyFilter_h

#include "Params.h"
#include "Filter.h"
#include "PolyData.h"

class PolyFilter : public Filter {
public:
  PolyFilter() : input(0) {};
  ~PolyFilter();
  virtual void setInput(PolyData *in);
  virtual PolyData* getInput();
  virtual void execute();
  virtual void update();
protected:
  PolyData *input;

};

#endif


