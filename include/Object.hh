/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Object.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkObject - abstract base class for visualization library
// .SECTION Description
// vtkObject is the base class for many objects in the visualization 
// library. vtkObject provides methods for tracking modification time, 
// debugging, and printing.

#ifndef __vtkObject_hh
#define __vtkObject_hh

#include <iostream.h>
#include "TimeSt.hh"
#include "SetGet.hh"
#include "Indent.hh"

class vtkObject 
{
public:
  vtkObject(); //create a vtk object
  virtual void Delete(); //delete a vtk object.
  virtual ~vtkObject(); //use Delete() whenever possible
  virtual char *GetClassName() {return "vtkObject";};

  // debugging
  virtual void DebugOn();
  virtual void DebugOff();
  int GetDebug();

  // modified time
  virtual unsigned long int GetMTime();
  virtual void Modified();

  // printing
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  void Print(ostream& os);
  virtual void PrintHeader(ostream& os, vtkIndent indent);
  virtual void PrintTrailer(ostream& os, vtkIndent indent);

protected:
  int Debug;         // Enable debug messages
  vtkTimeStamp MTime; // Keep track of modification time

private:
  friend ostream& operator<<(ostream& os, vtkObject& o);
};

// Description:
// Update the modification time for this object.
inline void vtkObject::Modified()
{
  this->MTime.Modified();
}

#endif

