/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#ifndef vtkStructuredPointsReader_h
#define vtkStructuredPointsReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkStructuredPoints;

class VTKIOLEGACY_EXPORT vtkStructuredPointsReader : public vtkDataReader
{
public:
  static vtkStructuredPointsReader *New();
  vtkTypeMacro(vtkStructuredPointsReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the output of this reader.
  void SetOutput(vtkStructuredPoints *output);
  vtkStructuredPoints *GetOutput(int idx);
  vtkStructuredPoints *GetOutput();

  // Description:
  // Read the meta information from the file.  This needs to be public to it
  // can be accessed by vtkDataSetReader.
  virtual int ReadMetaData(vtkInformation *outInfo);

protected:
  vtkStructuredPointsReader();
  ~vtkStructuredPointsReader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  // Default method performs Update to get information.  Not all the old
  // structured points sources compute information
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *);

  virtual int FillOutputPortInformation(int, vtkInformation *);
private:
  vtkStructuredPointsReader(const vtkStructuredPointsReader&);  // Not implemented.
  void operator=(const vtkStructuredPointsReader&);  // Not implemented.
};

#endif
