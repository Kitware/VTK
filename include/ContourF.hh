//
// Abstract class for specifying contourFilter behaviour
//
#ifndef __vlContourFilter_h
#define __vlContourFilter_h

#include "DataSetF.hh"
#include "PolyData.hh"

class vlContourFilter : public vlDataSetFilter, public vlPolyData 
{
public:
  vlContourFilter(float value=0.0) {this->Value = value;};
  ~vlContourFilter() {};
  char *GetClassName() {return "vlContourFilter";};

  vlSetMacro(Value,float);
  vlGetMacro(Value,float);

protected:
  void Execute();
  float Value;
};

#endif


