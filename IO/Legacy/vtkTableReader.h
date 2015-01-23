/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTableReader - read vtkTable data file
// .SECTION Description
// vtkTableReader is a source object that reads ASCII or binary
// vtkTable data files in vtk format. (see text for format details).
// The output of this reader is a single vtkTable data object.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkTable vtkDataReader vtkTableWriter

#ifndef vtkTableReader_h
#define vtkTableReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkTable;

class VTKIOLEGACY_EXPORT vtkTableReader : public vtkDataReader
{
public:
  static vtkTableReader *New();
  vtkTypeMacro(vtkTableReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkTable *GetOutput();
  vtkTable *GetOutput(int idx);
  void SetOutput(vtkTable *output);

protected:
  vtkTableReader();
  ~vtkTableReader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  // Since the Outputs[0] has the same UpdateExtent format
  // as the generic DataObject we can copy the UpdateExtent
  // as a default behavior.
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkTableReader(const vtkTableReader&);  // Not implemented.
  void operator=(const vtkTableReader&);  // Not implemented.
};

#endif
