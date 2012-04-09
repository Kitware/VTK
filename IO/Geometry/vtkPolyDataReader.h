/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPolyDataReader - read vtk polygonal data file
// .SECTION Description
// vtkPolyDataReader is a source object that reads ASCII or binary 
// polygonal data files in vtk format (see text for format details).
// The output of this reader is a single vtkPolyData data object.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkPolyData vtkDataReader

#ifndef __vtkPolyDataReader_h
#define __vtkPolyDataReader_h

#include "vtkDataReader.h"

class vtkPolyData;

class VTK_IO_EXPORT vtkPolyDataReader : public vtkDataReader
{
public:
  static vtkPolyDataReader *New();
  vtkTypeMacro(vtkPolyDataReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx);
  void SetOutput(vtkPolyData *output);

protected:
  vtkPolyDataReader();
  ~vtkPolyDataReader();

  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *);

  // Update extent of PolyData is specified in pieces.  
  // Since all DataObjects should be able to set UpdateExent as pieces,
  // just copy output->UpdateExtent  all Inputs.
  int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
  
  int FillOutputPortInformation(int, vtkInformation*);

private:
  vtkPolyDataReader(const vtkPolyDataReader&);  // Not implemented.
  void operator=(const vtkPolyDataReader&);  // Not implemented.
};

#endif


