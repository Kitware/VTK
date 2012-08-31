/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTreeReader - read vtkTree data file
// .SECTION Description
// vtkTreeReader is a source object that reads ASCII or binary
// vtkTree data files in vtk format. (see text for format details).
// The output of this reader is a single vtkTree data object.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkTree vtkDataReader vtkTreeWriter

#ifndef __vtkTreeReader_h
#define __vtkTreeReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkTree;

class VTKIOLEGACY_EXPORT vtkTreeReader : public vtkDataReader
{
public:
  static vtkTreeReader *New();
  vtkTypeMacro(vtkTreeReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkTree *GetOutput();
  vtkTree *GetOutput(int idx);
  void SetOutput(vtkTree *output);

protected:
  vtkTreeReader();
  ~vtkTreeReader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  // Since the Outputs[0] has the same UpdateExtent format
  // as the generic DataObject we can copy the UpdateExtent
  // as a default behavior.
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkTreeReader(const vtkTreeReader&);  // Not implemented.
  void operator=(const vtkTreeReader&);  // Not implemented.
};

#endif
