/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataWriter.h
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
  vtkTypeRevisionMacro(vtkPolyDataWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  void SetInput(vtkPolyData *input);
  vtkPolyData *GetInput();
                               
protected:
  vtkPolyDataWriter() {};
  ~vtkPolyDataWriter() {};

  void WriteData();

private:
  vtkPolyDataWriter(const vtkPolyDataWriter&);  // Not implemented.
  void operator=(const vtkPolyDataWriter&);  // Not implemented.
};

#endif


