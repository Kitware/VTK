/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridWriter.h
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
// .NAME vtkRectilinearGridWriter - write vtk rectilinear grid data file
// .SECTION Description
// vtkRectilinearGridWriter is a source object that writes ASCII or binary 
// rectilinear grid data files in vtk format. See text for format details.

// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.

#ifndef __vtkRectilinearGridWriter_h
#define __vtkRectilinearGridWriter_h

#include "vtkDataWriter.h"
#include "vtkRectilinearGrid.h"

class VTK_IO_EXPORT vtkRectilinearGridWriter : public vtkDataWriter
{
public:
  static vtkRectilinearGridWriter *New();
  vtkTypeRevisionMacro(vtkRectilinearGridWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set / get the input data or filter.
  void SetInput(vtkRectilinearGrid *input);
  vtkRectilinearGrid *GetInput();
                               
protected:
  vtkRectilinearGridWriter() {};
  ~vtkRectilinearGridWriter() {};

  void WriteData();

private:
  vtkRectilinearGridWriter(const vtkRectilinearGridWriter&);  // Not implemented.
  void operator=(const vtkRectilinearGridWriter&);  // Not implemented.
};

#endif


