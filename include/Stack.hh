/*=========================================================================

  Program:   Visualization Library
  Module:    Stack.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlStack - create and manipulate lists of objects
// .SECTION Description
// vlStack is a general object for creating and manipulating lists
// of objects. vlStack also serves as a base class for lists of
// specific types of objects.

#ifndef __vlStack_hh
#define __vlStack_hh

#include "Object.hh"

//BTX begin tcl exclude
class vlStackElement //;prevents pick-up by man page generator
{
 public:
  vlStackElement():Item(NULL),Next(NULL) {};
  vlObject *Item;
  vlStackElement *Next;
};
//ETX end tcl exclude

class vlStack : public vlObject
{
public:
  vlStack();
  ~vlStack();
  void PrintSelf(ostream& os, vlIndent indent);
  char *GetClassName() {return "vlStack";};

  void Push(vlObject *);
  vlObject *Pop();
  vlObject *GetTop();
  int  GetNumberOfItems();

protected:
  int NumberOfItems;
  vlStackElement *Top;
  vlStackElement *Bottom;
};

#endif
