//
// Abstract class for specifying behavior of data sources
//
#ifndef vlSource_h
#define vlSource_h

#include "Object.h"

class vlSource : virtual public vlObject {
public:
  vlSource() : StartMethod(0), EndMethod(0) {};
  virtual ~vlSource() {};
  virtual void Execute();
  virtual void Update();
  void SetStartMethod(void (*f)());
  void SetEndMethod(void (*f)());

protected:
  void (*StartMethod)();
  void (*EndMethod)();
  vlTimeStamp ExecuteTime;
};

#endif


