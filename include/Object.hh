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
// .NAME vlObject - abstract base class for many objects
// .SECTION Description
// vlObject is the base class for many objects in the visualization 
// library. vlObject provides methods for reference counting, keeping 
// track of modified time, debugging, and printing object.

#ifndef __vlObject_hh
#define __vlObject_hh

#include <iostream.h>
#include "TimeSt.hh"
#include "SetGet.hh"
#include "Indent.hh"
#include "PrintLst.hh"

class vlObject 
{
public:
  vlObject();
  virtual ~vlObject();
  virtual char *GetClassName() {return "vlObject";};

  // reference counting
  void Register(vlObject* o);
  void UnRegister(vlObject* o);
  int  GetRefCount() {return this->RefCount;};

  // debugging
  void DebugOn();
  void DebugOff();
  int GetDebug();

  // modified time
  virtual unsigned long int GetMTime() { return this->MTime.GetMTime(); };
  void Modified() {MTime.Modified();};

  // printing
  virtual void PrintSelf(ostream& os, vlIndent indent);
  void Print(ostream& os);
  virtual void PrintHeader(ostream& os, vlIndent indent);
  int ShouldIPrint(char *a) { return this->PrintList.ShouldIPrint(a);};
  virtual void PrintTrailer(ostream& os, vlIndent indent);
  void PrintWatchOn() {this->PrintList.ActiveOn();};
  void PrintWatchOff() {this->PrintList.ActiveOff();};

protected:
  int Debug;         // Enable debug messages
  vlTimeStamp MTime; // Keep track of modification time

private:
  int RefCount;      // Number of uses of this object by other objects
  vlPrintList PrintList;

  friend ostream& operator<<(ostream& os, vlObject& o);
};

#endif

