/*=========================================================================

  Program:   Visualization Library
  Module:    Writer.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vlWriter - abstract class to write data to file(s)
// .SECTION Description
// vlWriter is an abstract class for mapper objects that write their data
// to disk (or into a communications port). All writers respond to Write()
// method. This method insures that there is input and input is up to date.
// .SECTION Caveats
// Every subclass of vlWriter must implement a WriteData() method. Most likely
// will have to create SetInput() method as well.

#ifndef __vlWriter_hh
#define __vlWriter_hh

#include "Object.hh"
#include "DataSet.hh"

class vlWriter : public vlObject 
{
public:
  vlWriter();
  ~vlWriter() {};
  char *GetClassName() {return "vlWriter";};
  void PrintSelf(ostream& os, vlIndent indent);

  virtual void Write();
  void Update();

  void SetStartWrite(void (*f)(void *), void *arg);
  void SetEndWrite(void (*f)(void *), void *arg);
  void SetStartWriteArgDelete(void (*f)(void *));
  void SetEndWriteArgDelete(void (*f)(void *));

protected:
  vlDataSet *Input;
  virtual void WriteData() = 0;

  void (*StartWrite)(void *);
  void (*StartWriteArgDelete)(void *);
  void *StartWriteArg;
  void (*EndWrite)(void *);
  void (*EndWriteArgDelete)(void *);
  void *EndWriteArg;

};

#endif


