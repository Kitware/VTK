/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgrammableDataObjectSource.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkProgrammableDataObjectSource - generate source data object via a user-specified function
// .SECTION Description
// vtkProgrammableDataObjectSource is a source object that is programmable by
// the user. The output of the filter is a data object (vtkDataObject) which
// represents data via an instance of field data. To use this object, you
// must specify a function that creates the output.  
//
// Example use of this filter includes reading tabular data and encoding it
// as vtkFieldData. You can then use filters like vtkDataObjectToDataSetFilter
// to convert the data object to a dataset and then visualize it.  Another
// important use of this class is that it allows users of interpreters (e.g.,
// Tcl or Java) the ability to write source objects without having to
// recompile C++ code or generate new libraries.
// 
// .SECTION See Also
// vtkProgrammableFilter vtkProgrammableAttributeDataFilter
// vtkProgrammableSource vtkDataObjectToDataSetFilter

#ifndef __vtkProgrammableDataObjectSource_h
#define __vtkProgrammableDataObjectSource_h

#include "vtkSource.h"

class VTK_GRAPHICS_EXPORT vtkProgrammableDataObjectSource : public vtkSource
{
public:
  static vtkProgrammableDataObjectSource *New();
  vtkTypeRevisionMacro(vtkProgrammableDataObjectSource,vtkSource);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the function to use to generate the output data object. Note
  // that the function takes a single (void *) argument.
  void SetExecuteMethod(void (*f)(void *), void *arg);

  // Description:
  // Set the arg delete method. This is used to free user memory.
  void SetExecuteMethodArgDelete(void (*f)(void *));

  // Description:
  // Get the output data object.
  vtkDataObject *GetOutput();
  vtkDataObject *GetOutput(int idx)
    {return (vtkDataObject *) this->vtkSource::GetOutput(idx); };

protected:
  vtkProgrammableDataObjectSource();
  ~vtkProgrammableDataObjectSource();

  void Execute();

  void (*ExecuteMethod)(void *); //function to invoke
  void (*ExecuteMethodArgDelete)(void *);
  void *ExecuteMethodArg;
private:
  vtkProgrammableDataObjectSource(const vtkProgrammableDataObjectSource&);  // Not implemented.
  void operator=(const vtkProgrammableDataObjectSource&);  // Not implemented.
};

#endif

