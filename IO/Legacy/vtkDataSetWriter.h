/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataSetWriter - write any type of vtk dataset to file
// .SECTION Description
// vtkDataSetWriter is an abstract class for mapper objects that write their
// data to disk (or into a communications port). The input to this object is
// a dataset of any type.

#ifndef vtkDataSetWriter_h
#define vtkDataSetWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"

class VTKIOLEGACY_EXPORT vtkDataSetWriter : public vtkDataWriter
{
public:
  static vtkDataSetWriter *New();
  vtkTypeMacro(vtkDataSetWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the input to this writer.
  vtkDataSet* GetInput();
  vtkDataSet* GetInput(int port);

protected:
  vtkDataSetWriter() {}
  ~vtkDataSetWriter() {}

  void WriteData();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkDataSetWriter(const vtkDataSetWriter&);  // Not implemented.
  void operator=(const vtkDataSetWriter&);  // Not implemented.
};

#endif
