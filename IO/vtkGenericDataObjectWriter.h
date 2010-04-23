/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataObjectWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericDataObjectWriter - writes any type of vtk data object to file
// .SECTION Description
// vtkGenericDataObjectWriter is a concrete class that writes data objects
// to disk. The input to this object is any subclass of vtkDataObject.

#ifndef __vtkGenericDataObjectWriter_h
#define __vtkGenericDataObjectWriter_h

#include "vtkDataWriter.h"

class VTK_IO_EXPORT vtkGenericDataObjectWriter : public vtkDataWriter
{
public:
  static vtkGenericDataObjectWriter *New();
  vtkTypeMacro(vtkGenericDataObjectWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkGenericDataObjectWriter();
  ~vtkGenericDataObjectWriter();

  void WriteData();
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkGenericDataObjectWriter(const vtkGenericDataObjectWriter&);  // Not implemented.
  void operator=(const vtkGenericDataObjectWriter&);  // Not implemented.
};

#endif
