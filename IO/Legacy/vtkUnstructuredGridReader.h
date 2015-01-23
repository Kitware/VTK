/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkUnstructuredGridReader - read vtk unstructured grid data file
// .SECTION Description
// vtkUnstructuredGridReader is a source object that reads ASCII or binary
// unstructured grid data files in vtk format. (see text for format details).
// The output of this reader is a single vtkUnstructuredGrid data object.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkUnstructuredGrid vtkDataReader

#ifndef vtkUnstructuredGridReader_h
#define vtkUnstructuredGridReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkUnstructuredGrid;

class VTKIOLEGACY_EXPORT vtkUnstructuredGridReader : public vtkDataReader
{
public:
  static vtkUnstructuredGridReader *New();
  vtkTypeMacro(vtkUnstructuredGridReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkUnstructuredGrid *GetOutput();
  vtkUnstructuredGrid *GetOutput(int idx);
  void SetOutput(vtkUnstructuredGrid *output);

protected:
  vtkUnstructuredGridReader();
  ~vtkUnstructuredGridReader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  // Since the Outputs[0] has the same UpdateExtent format
  // as the generic DataObject we can copy the UpdateExtent
  // as a default behavior.
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkUnstructuredGridReader(const vtkUnstructuredGridReader&);  // Not implemented.
  void operator=(const vtkUnstructuredGridReader&);  // Not implemented.
};

#endif
