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

void DataSetFilter::setInput(DataSet *in)
{
  if (in != input )
  {
    input = in;
    input->Register((void *)this);
    modified();
  }
}
DataSet* DataSetFilter::getInput()
{
    return input;
}

DataSetFilter::~DataSetFilter()
{
  if ( input != 0 )
  {
    input->UnRegister((void *)this);
  }
}

void DataSetFilter::execute()
{
  cout << "Executing DataSetFilter\n";
}

void DataSetFilter::update()
{
  // make sure input is available
  if ( !input )
  {
    cout << "No input available for DataSetFilter\n";
    return;
  }

  // prevent chasing our tail
  if (updating) return;

  updating = 1;
  input->update();
  updating = 0;

  if (input->getMtime() > getMtime() )
  {
    (*startMethod)();
    execute();
    modified();
    (*endMethod)();
  }
 

}



#endif


