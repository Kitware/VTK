//
// DataSetFilter takes general DataSet as input
//
#ifndef __vlDataSetFilter_h
#define __vlDataSetFilter_h

#include "Filter.hh"
#include "DataSetF.hh"

class vlDataSetFilter : public vlFilter 
{
public:
  vlDataSetFilter();
  ~vlDataSetFilter();
  char *GetClassName() {return "vlDataSetFilter";};

  void Update();
  vlSetObjectMacro(Input,vlDataSet);
  vlGetObjectMacro(Input,vlDataSet);

protected:
  vlDataSet *Input;

};

#endif


