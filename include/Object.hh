/*=========================================================================

  Program:   Visualization Library
  Module:    Object.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
//
// Common abstract class for Visualization Library.  Maintains debug
// flag, reference counting, modified time, and other common
// functions/parameters.
//

#ifndef __vlObject_hh
#define __vlObject_hh

#include <iostream.h>
#include "TimeSt.hh"
#include "SetGet.hh"
#include "Indent.hh"
#include "PrintLst.hh"

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

  void Print(ostream& os);
  virtual void PrintHeader(ostream& os, vlIndent indent);
  virtual void PrintSelf(ostream& os, vlIndent indent);
  int ShouldIPrint(char *a) { return this->PrintList.ShouldIPrint(a);};
  virtual void PrintTrailer(ostream& os, vlIndent indent);
  void PrintWatchOn() {this->PrintList.ActiveOn();};
  void PrintWatchOff() {this->PrintList.ActiveOff();};

protected:
  int Debug;       // Enable debug messages
  vlTimeStamp Mtime; // Keep track of modification time

private:
  int RefCount;    // Number of uses of this object by other objects
  vlPrintList PrintList;

friend ostream& operator<<(ostream& os, vlObject& o) {o.Print(os);return os;}
};

#endif

