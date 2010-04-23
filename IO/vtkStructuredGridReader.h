/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
  vtkTypeMacro(vtkStructuredGridReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkStructuredGrid *GetOutput();
  vtkStructuredGrid *GetOutput(int idx);
  void SetOutput(vtkStructuredGrid *output);  

  // Description:
  // Read the meta information from the file.  This needs to be public to it
  // can be accessed by vtkDataSetReader.
  virtual int ReadMetaData(vtkInformation *outInfo);

protected:
  vtkStructuredGridReader();
  ~vtkStructuredGridReader();

  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkStructuredGridReader(const vtkStructuredGridReader&);  // Not implemented.
  void operator=(const vtkStructuredGridReader&);  // Not implemented.
};

#endif


