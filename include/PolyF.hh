//
// PolyFilter takes PolyData as input
//
#ifndef __vlPolyFilter_h
#define __vlPolyFilter_h

#include "Filter.hh"
#include "PolyData.hh"

class vlPolyFilter : public vlFilter {
public:
  vlPolyFilter() : Input(0) {};
  ~vlPolyFilter();
  char *GetClassName() {return "vlPolyFilter";};

  void Update();
  vlSetObjectMacro(Input,vlPolyData);
  vlGetObjectMacro(Input,vlPolyData);

protected:
  vlPolyData *Input;

};

#endif


