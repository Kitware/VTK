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

private:
  void (*startMethod)();
  void (*endMethod)();
  TimeStamp executeTime;
};

void Source::execute()
{
  cout << "Executing Source\n";
}

void Source::update()
{
  // Make sure virtual getMtime method is called since subclasses will overload
  if (getMtime() > executeTime.getMtime())
  {
    (*startMethod)();
    execute();
    executeTime.modified();
    (*endMethod)();
  }
}

void Source::setStartMethod(void (*f)())
{
  if ( f != startMethod )
  {
    startMethod = f;
    modified();
  }
}

void Source::setEndMethod(void (*f)())
{
  if ( f != endMethod )
  {
    endMethod = f;
    modified();
  }

}

#endif


