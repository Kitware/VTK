/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataWriter - write vtk polygonal data
// .SECTION Description
// vtkPolyDataWriter is a source object that writes ASCII or binary 
// polygonal data files in vtk format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vtkPolyDataWriter_h
#define __vtkPolyDataWriter_h

#include "vtkDataWriter.h"

class vtkPolyData;

class VTK_IO_EXPORT vtkPolyDataWriter : public vtkDataWriter
{
public:
  static vtkPolyDataWriter *New();
  vtkTypeMacro(vtkPolyDataWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the input to this writer.
  vtkPolyData* GetInput();
  vtkPolyData* GetInput(int port);

protected:
  vtkPolyDataWriter() {};
  ~vtkPolyDataWriter() {};

  void WriteData();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkPolyDataWriter(const vtkPolyDataWriter&);  // Not implemented.
  void operator=(const vtkPolyDataWriter&);  // Not implemented.
};

#endif


