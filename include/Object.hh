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
// .NAME vlObject - abstract base class for visualization library
// .SECTION Description
// vlObject is the base class for many objects in the visualization 
// library. vlObject provides methods for tracking modification time, 
// debugging, and printing.

#ifndef __vlObject_hh
#define __vlObject_hh

#include <iostream.h>
#include "TimeSt.hh"
#include "SetGet.hh"
#include "Indent.hh"

class vlObject 
{
public:
  vlObject();
  virtual ~vlObject();
  virtual char *GetClassName() {return "vlObject";};

  // debugging
  void DebugOn();
  void DebugOff();
  int GetDebug();

  // modified time
  virtual unsigned long int GetMTime();
  virtual void Modified();

  // printing
  virtual void PrintSelf(ostream& os, vlIndent indent);
  void Print(ostream& os);
  virtual void PrintHeader(ostream& os, vlIndent indent);
  virtual void PrintTrailer(ostream& os, vlIndent indent);

protected:
  int Debug;         // Enable debug messages
  vlTimeStamp MTime; // Keep track of modification time

private:
  friend ostream& operator<<(ostream& os, vlObject& o);
};

// Description:
// Update the modification time for this object.
inline void vlObject::Modified()
{
  this->MTime.Modified();
}

#endif

