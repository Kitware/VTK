//
// Common abstract class for Visualization Library.  Maintains debug
// flag, reference counting, modified time, and other common
// functions/parameters.
//

#ifndef __vlObject_h
#define __vlObject_h

#include <iostream.h>
#include "TimeSt.h"
//
// Common #defines / parameters
//

//
// Class definition
//

class vlObject {
public:
  vlObject();
  virtual ~vlObject();
  void Register(const void* p) {this->RefCount++;};
  void UnRegister(const void* p) {if (--this->RefCount <= 0) delete this;};
  int  GetRefCount() {return this->RefCount;};
  void DebugOn();
  void DebugOff();
  int GetDebug();
  vlTimeStamp Mtime; // Keep track of modification time
  void Modified() {Mtime.Modified();};

protected:
  int Debug;       // Enable debug messages

private:
  int RefCount;    // Number of uses of this object by other objects

};

#endif

