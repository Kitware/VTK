//
// DataSetFilter takes general DataSet as input
//
#ifndef __vlDataSetFilter_h
#define __vlDataSetFilter_h

#include "Filter.h"
#include "DataSetF.h"

class vlDataSetFilter : virtual public vlFilter {
public:
  vlDataSetFilter();
  ~vlDataSetFilter();
  virtual void SetInput(vlDataSet *in);
  virtual vlDataSet*  GetInput();
  virtual void Execute();
  virtual void Update();
protected:
  vlDataSet *Input;

};

#endif


