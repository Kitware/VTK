//
// PolyFilter takes PolyData as input
//
#ifndef PolyFilter_h
#define PolyFilter_h

#include "Params.h"
#include "Filter.h"
#include "PolyData.h"

class PolyFilter : virtual public Filter {
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

void PolyFilter::setInput(PolyData *in)
{
  if (in != input )
  {
    input = in;
    input->Register((void *)this);
    modified();
  }
}
PolyData* PolyFilter::getInput()
{
    return input;
}

PolyFilter::~PolyFilter()
{
  if ( input != 0 )
  {
    input->UnRegister((void *)this);
  }
}

void PolyFilter::execute()
{
  cout << "Executing PolyFilter\n";
}

void PolyFilter::update()
{
  // make sure input is available
  if ( !input )
  {
    cout << "No input available for PolyFilter\n";
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


