//
// Abstract class for specifying filter behaviour
//
#ifndef Filter_h
#define Filter_h

#include "Params.h"
#include "DataSet.h"

class Filter : virtual public DataSet {
public:
  Filter() : startMethod(0), endMethod(0), updating(0) {};
  virtual ~Filter() {};
  virtual void execute() = 0;
  virtual void update() = 0;
  void setStartMethod(void (*f)());
  void setEndMethod(void (*f)());
protected:
  char updating;
  void (*startMethod)();
  void (*endMethod)();
};

void Filter::setStartMethod(void (*f)())
{
  if ( f != startMethod )
  {
    startMethod = f;
    modified();
  }
}

void Filter::setEndMethod(void (*f)())
{
  if ( f != endMethod )
  {
    endMethod = f;
    modified();
  }

}

#endif


