/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridWriter.h
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
// .NAME vtkUnstructuredGridWriter - write vtk unstructured grid data file
// .SECTION Description
// vtkUnstructuredGridWriter is a source object that writes ASCII or binary 
// unstructured grid data files in vtk format. See text for format details.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vtkUnstructuredGridWriter_h
#define __vtkUnstructuredGridWriter_h

#include "vtkDataWriter.h"
#include "vtkUnstructuredGrid.h"

class VTK_IO_EXPORT vtkUnstructuredGridWriter : public vtkDataWriter
{
public:
  static vtkUnstructuredGridWriter *New();
  vtkTypeRevisionMacro(vtkUnstructuredGridWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the input data or filter.
  void SetInput(vtkUnstructuredGrid *input);
  vtkUnstructuredGrid *GetInput();

protected:
  vtkUnstructuredGridWriter() {};
  ~vtkUnstructuredGridWriter() {};

  void WriteData();

private:
  vtkUnstructuredGridWriter(const vtkUnstructuredGridWriter&);  // Not implemented.
  void operator=(const vtkUnstructuredGridWriter&);  // Not implemented.
};

#endif


