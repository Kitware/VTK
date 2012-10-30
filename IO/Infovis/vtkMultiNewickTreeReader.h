/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMultiNewickTreeReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMultiNewickTreeReader - read multiple vtkTrees from Newick formatted file
// .SECTION Description
// vtkMultiNewickTreeReader is a source object that reads Newick tree format
// files.
// The output of this reader is a single vtkMultiPieceDataSet that contains multiple vtkTree objects.
// The superclass of this class, vtkDataReader, provides many methods for
// controlling the reading of the data file, see vtkDataReader for more
// information.
// .SECTION See Also
// vtkTree vtkDataReader

#ifndef __vtkMultiNewickTreeReader_h
#define __vtkMultiNewickTreeReader_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkDataReader.h"

class vtkMultiPieceDataSet;
class vtkNewickTreeReader;
class VTKIOINFOVIS_EXPORT vtkMultiNewickTreeReader : public vtkDataReader
{
public:
  static vtkMultiNewickTreeReader *New();
  vtkTypeMacro(vtkMultiNewickTreeReader,vtkDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output of this reader.
  vtkMultiPieceDataSet *GetOutput();
  vtkMultiPieceDataSet *GetOutput(int idx);
  void SetOutput(vtkMultiPieceDataSet *output);

protected:
  vtkMultiNewickTreeReader();
  ~vtkMultiNewickTreeReader();

  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);

  // Since the Outputs[0] has the same UpdateExtent format
  // as the generic DataObject we can copy the UpdateExtent
  // as a default behavior.
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *);

  virtual int FillOutputPortInformation(int, vtkInformation*);
private:
  vtkMultiNewickTreeReader(const vtkMultiNewickTreeReader&);  // Not implemented.
  void operator=(const vtkMultiNewickTreeReader&);  // Not implemented.
};

#endif
