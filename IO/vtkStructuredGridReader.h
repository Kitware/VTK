/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridReader.h
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
// .NAME vtkStructuredGridReader - read vtk structured grid data file
// .SECTION Description
// vtkStructuredGridReader is a source object that reads ASCII or binary 
// structured grid data files in vtk format. (see text for format details).
// The output of this reader is a single vtkStructuredGrid data object.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkStructuredGrid vtkDataReader

#ifndef __vtkStructuredGridReader_h
#define __vtkStructuredGridReader_h

#include "vtkDataReader.h"

class vtkStructuredGrid;

class VTK_IO_EXPORT vtkStructuredGridReader : public vtkDataReader
{
public:
  static vtkStructuredGridReader *New();
  vtkTypeRevisionMacro(vtkStructuredGridReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkStructuredGrid *GetOutput();
  vtkStructuredGrid *GetOutput(int idx)
    {return (vtkStructuredGrid *) this->vtkSource::GetOutput(idx); };
  void SetOutput(vtkStructuredGrid *output);  

protected:
  vtkStructuredGridReader();
  ~vtkStructuredGridReader();

  void ExecuteInformation();
  void Execute();

private:
  vtkStructuredGridReader(const vtkStructuredGridReader&);  // Not implemented.
  void operator=(const vtkStructuredGridReader&);  // Not implemented.
};

#endif


