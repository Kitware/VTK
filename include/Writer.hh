/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Writer.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkWriter - abstract class to write data to file(s)
// .SECTION Description
// vtkWriter is an abstract class for mapper objects that write their data
// to disk (or into a communications port). All writers respond to Write()
// method. This method insures that there is input and input is up to date.
// .SECTION Caveats
// Every subclass of vtkWriter must implement a WriteData() method. Most likely
// will have to create SetInput() method as well.

#ifndef __vtkWriter_hh
#define __vtkWriter_hh

#include "Object.hh"
#include "DataSet.hh"

class vtkWriter : public vtkObject 
{
public:
  vtkWriter();
  ~vtkWriter() {};
  char *GetClassName() {return "vtkWriter";};
  void PrintSelf(ostream& os, vtkIndent indent);

  virtual void Write();
  void Update();

  void SetStartWrite(void (*f)(void *), void *arg);
  void SetEndWrite(void (*f)(void *), void *arg);
  void SetStartWriteArgDelete(void (*f)(void *));
  void SetEndWriteArgDelete(void (*f)(void *));

protected:
  vtkDataSet *Input;
  virtual void WriteData() = 0;

  void (*StartWrite)(void *);
  void (*StartWriteArgDelete)(void *);
  void *StartWriteArg;
  void (*EndWrite)(void *);
  void (*EndWriteArgDelete)(void *);
  void *EndWriteArg;

};

#endif


