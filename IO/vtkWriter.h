/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWriter.h
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
// .NAME vtkWriter - abstract class to write data to file(s)
// .SECTION Description
// vtkWriter is an abstract class for mapper objects that write their data
// to disk (or into a communications port). All writers respond to Write()
// method. This method insures that there is input and input is up to date.
//
// Since vtkWriter is a subclass of vtkProcessObject, StartMethod(), 
// EndMethod(), and ProgressMethod() are all available to writers.
// These methods are executed before and after execution of the Write() 
// method. You can also specify arguments to these methods.

// .SECTION Caveats
// Every subclass of vtkWriter must implement a WriteData() method. Most likely
// will have to create SetInput() method as well.

// .SECTION See Also
// vtkBYUWriter vtkDataWriter vtkSTLWriter vtkVoxelWriter vtkMCubesWriter

#ifndef __vtkWriter_h
#define __vtkWriter_h

#include "vtkProcessObject.h"

class vtkDataObject;

#define VTK_ASCII 1
#define VTK_BINARY 2

class VTK_IO_EXPORT vtkWriter : public vtkProcessObject
{
public:
  vtkTypeRevisionMacro(vtkWriter,vtkProcessObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Write data to output. Method executes subclasses WriteData() method, as 
  // well as StartMethod() and EndMethod() methods.
  virtual void Write();

  // Description:
  // Convenient alias for Write() method.
  void Update();

  // Description:
  // Encode the name so that the reader will not have problems.
  // The resulting string is up to four time the size of the input 
  // string.
  void EncodeArrayName(char* resname, const char* name);
  
//BTX
  vtkDataObject *GetInput();
//ETX
protected:
  vtkWriter();
  ~vtkWriter();
  
  virtual void WriteData() = 0; //internal method subclasses must respond to
  vtkTimeStamp WriteTime;
private:
  vtkWriter(const vtkWriter&);  // Not implemented.
  void operator=(const vtkWriter&);  // Not implemented.
};

#endif


