/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsReader.h
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
// .NAME vtkStructuredPointsReader - read vtk structured points data file
// .SECTION Description
// vtkStructuredPointsReader is a source object that reads ASCII or binary 
// structured points data files in vtk format (see text for format details).
// The output of this reader is a single vtkStructuredPoints data object.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkStructuredPoints vtkDataReader

#ifndef __vtkStructuredPointsReader_h
#define __vtkStructuredPointsReader_h

#include "vtkDataReader.h"

class vtkStructuredPoints;

class VTK_IO_EXPORT vtkStructuredPointsReader : public vtkDataReader
{
public:
  static vtkStructuredPointsReader *New();
  vtkTypeRevisionMacro(vtkStructuredPointsReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the output of this reader.
  void SetOutput(vtkStructuredPoints *output);
  vtkStructuredPoints *GetOutput(int idx)
    {return (vtkStructuredPoints *) this->vtkSource::GetOutput(idx); };
  vtkStructuredPoints *GetOutput();
  
protected:
  vtkStructuredPointsReader();
  ~vtkStructuredPointsReader();

  void Execute();

  // Default method performs Update to get information.  Not all the old
  // structured points sources compute information
  void ExecuteInformation();

private:
  vtkStructuredPointsReader(const vtkStructuredPointsReader&);  // Not implemented.
  void operator=(const vtkStructuredPointsReader&);  // Not implemented.
};

#endif


