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
  virtual char *GetClassName() {return "vlDataSetFilter";};
  virtual void SetInput(vlDataSet *in);
  virtual vlDataSet*  GetInput();
  void Execute();
  void Update();
protected:
  vlDataSet *Input;

};

#endif


