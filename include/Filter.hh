//
// Abstract class for specifying filter behaviour
//
#ifndef __vlFilter_h
#define __vlFilter_h

#include "Object.hh"
#include "DataSet.hh"

class vlFilter : virtual public vlObject 
{
public:
  vlFilter() : StartMethod(0), EndMethod(0), Updating(0) {};
  ~vlFilter() {};
  virtual void Execute() = 0;
  virtual void Update() = 0;
  void SetStartMethod(void (*f)());
  void SetEndMethod(void (*f)());
protected:
  char Updating;
  void (*StartMethod)();
  void (*EndMethod)();
  vlTimeStamp ExecuteTime;

};

#endif


