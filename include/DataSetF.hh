//
// DataSetFilter takes general DataSet as input
//
#ifndef DataSetFilter_h
#define DataSetFilter_h

#include "Params.h"
#include "Filter.h"
#include "DataSetF.h"

class DataSetFilter : virtual public Filter {
public:
  DataSetFilter();
  ~DataSetFilter();
  virtual void setInput(DataSet *in);
  virtual DataSet*  getInput();
  virtual void execute();
  virtual void update();
protected:
  DataSet *input;

};

#endif


