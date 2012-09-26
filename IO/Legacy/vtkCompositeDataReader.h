/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCompositeDataReader - read vtkCompositeDataSet data file.
// .SECTION Description
// .SECTION CAVEATS
// This is an experimental format. Use XML-based formats for writing composite
// datasets. Saving composite dataset in legacy VTK format is expected to change
// in future including changes to the file layout.

#ifndef __vtkCompositeDataReader_h
#define __vtkCompositeDataReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataReader.h"

class vtkCompositeDataSet;
class vtkHierarchicalBoxDataSet;
class vtkMultiBlockDataSet;
class vtkMultiPieceDataSet;
class vtkNonOverlappingAMR;
class vtkOverlappingAMR;

class VTKIOLEGACY_EXPORT vtkCompositeDataReader : public vtkDataReader
{
public:
  static vtkCompositeDataReader* New();
  vtkTypeMacro(vtkCompositeDataReader, vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkCompositeDataSet *GetOutput();
  vtkCompositeDataSet *GetOutput(int idx);
  void SetOutput(vtkCompositeDataSet *output);

//BTX
protected:
  vtkCompositeDataReader();
  ~vtkCompositeDataReader();

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

  virtual int FillOutputPortInformation(int, vtkInformation*);

  // Description:
  // Read the output type information.
  int ReadOutputType();

  bool ReadCompositeData(vtkMultiPieceDataSet*);
  bool ReadCompositeData(vtkMultiBlockDataSet*);
  bool ReadCompositeData(vtkHierarchicalBoxDataSet*);
  bool ReadCompositeData(vtkOverlappingAMR*);
  bool ReadCompositeData(vtkNonOverlappingAMR*);
  vtkDataObject* ReadChild();

private:
  vtkCompositeDataReader(const vtkCompositeDataReader&); // Not implemented.
  void operator=(const vtkCompositeDataReader&); // Not implemented.
//ETX
};

#endif
