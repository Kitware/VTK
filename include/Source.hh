//
// Abstract class for specifying behavior of data sources
//
#ifndef Source_h
#define Source_h

#include "Params.h"
#include "TimeSt.h"
#include "RefCount.h"

class Source : virtual public TimeStamp, virtual public RefCount {
public:
  Source() : startMethod(0), endMethod(0) {};
  virtual ~Source() {};
  virtual void execute();
  virtual void update();
  void setStartMethod(void (*f)());
  void setEndMethod(void (*f)());

protected:
  void (*startMethod)();
  void (*endMethod)();
  TimeStamp executeTime;
};

#endif


