/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGraphReader - read vtkGraph data file
// .SECTION Description
// vtkGraphReader is a source object that reads ASCII or binary
// vtkGraph data files in vtk format. (see text for format details).
// The output of this reader is a single vtkGraph data object.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// .SECTION See Also
// vtkGraph vtkDataReader vtkGraphWriter

#ifndef __vtkGraphReader_h
#define __vtkGraphReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkGraph;

class VTKIOLEGACY_EXPORT vtkGraphReader : public vtkDataReader
{
public:
  static vtkGraphReader *New();
  vtkTypeMacro(vtkGraphReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkGraph *GetOutput();
  vtkGraph *GetOutput(int idx);
  void SetOutput(vtkGraph *output);

protected:
  vtkGraphReader();
  ~vtkGraphReader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  // Override ProcessRequest to handle request data object event
  virtual int ProcessRequest(vtkInformation *, vtkInformationVector **,
                             vtkInformationVector *);

  // Since the Outputs[0] has the same UpdateExtent format
  // as the generic DataObject we can copy the UpdateExtent
  // as a default behavior.
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);

  // Create output (a directed or undirected graph).
  virtual int RequestDataObject(vtkInformation *, vtkInformationVector **,
                                vtkInformationVector *);

  // Read beginning of file to determine whether the graph is directed.
  virtual int ReadGraphDirectedness(bool & directed);


  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkGraphReader(const vtkGraphReader&);  // Not implemented.
  void operator=(const vtkGraphReader&);  // Not implemented.
};

#endif
