//
// Common abstract class for Visualization Library.  Maintains debug
// flag, reference counting, modified time, and other common
// functions/parameters.
//

#ifndef __vlObject_h
#define __vlObject_h

#include <iostream.h>
#include "TimeSt.hh"
#include "SetGet.hh"

//
// Common #defines / parameters
//

//
// Class definition
//

class vlObject 
{
public:
  vlObject();
  virtual ~vlObject();
  void Register(const void* p) {this->RefCount++;};
  void UnRegister(const void* p) {if (--this->RefCount <= 0) delete this;};
  int  GetRefCount() {return this->RefCount;};
  void DebugOn();
  void DebugOff();
  virtual unsigned long int GetMtime() {return this->Mtime.GetMtime();};
  int GetDebug();
  void Modified() {Mtime.Modified();};
  virtual char *GetClassName() {return "vlObject";};

protected:
  vlTimeStamp Mtime; // Keep track of modification time
  int Debug;       // Enable debug messages

private:
  int RefCount;    // Number of uses of this object by other objects

};

#endif

