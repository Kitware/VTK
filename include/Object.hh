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
  void Register(void* p);
  void UnRegister(void* p);
  int  GetRefCount() {return this->RefCount;};
  void DebugOn();
  void DebugOff();
  virtual unsigned long int GetMtime() {return this->Mtime.GetMtime();};
  int GetDebug();
  void Modified() {Mtime.Modified();};
  virtual char *GetClassName() {return "vlObject";};

  void Print(ostream& os) 
    {this->PrintHeader(os); this->PrintSelf(os); this->PrintTrailer(os);};
  virtual void PrintHeader(ostream& os);
  virtual void PrintSelf(ostream& os);
  virtual void PrintTrailer(ostream& os);

protected:
  int Debug;       // Enable debug messages
  vlTimeStamp Mtime; // Keep track of modification time

private:
  int RefCount;    // Number of uses of this object by other objects

friend ostream& operator<<(ostream& os, vlObject& o) {o.Print(os);return os;}
};

#endif

