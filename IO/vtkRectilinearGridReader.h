/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridReader.h
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
// .NAME vtkRectilinearGridReader - read vtk rectilinear grid data file
// .SECTION Description
// vtkRectilinearGridReader is a source object that reads ASCII or binary 
// rectilinear grid data files in vtk format (see text for format details).
// The output of this reader is a single vtkRectilinearGrid data object.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkRectilinearGrid vtkDataReader

#ifndef __vtkRectilinearGridReader_h
#define __vtkRectilinearGridReader_h

#include "vtkDataReader.h"
#include "vtkRectilinearGrid.h"

class VTK_IO_EXPORT vtkRectilinearGridReader : public vtkDataReader
{
public:
  static vtkRectilinearGridReader *New();
  vtkTypeRevisionMacro(vtkRectilinearGridReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get and set the output of this reader.
  vtkRectilinearGrid *GetOutput();
  vtkRectilinearGrid *GetOutput(int idx)
    {return (vtkRectilinearGrid *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkRectilinearGrid *output);

protected:
  vtkRectilinearGridReader();
  ~vtkRectilinearGridReader();

  void Execute();
  void ExecuteInformation();

private:
  vtkRectilinearGridReader(const vtkRectilinearGridReader&);  // Not implemented.
  void operator=(const vtkRectilinearGridReader&);  // Not implemented.
};

#endif


