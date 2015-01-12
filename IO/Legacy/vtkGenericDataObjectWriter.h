/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericDataObjectWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkGenericDataObjectWriter - writes any type of vtk data object to file
// .SECTION Description
// vtkGenericDataObjectWriter is a concrete class that writes data objects
// to disk. The input to this object is any subclass of vtkDataObject.

#ifndef vtkGenericDataObjectWriter_h
#define vtkGenericDataObjectWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"

class VTKIOLEGACY_EXPORT vtkGenericDataObjectWriter : public vtkDataWriter
{
public:
  static vtkGenericDataObjectWriter *New();
  vtkTypeMacro(vtkGenericDataObjectWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkGenericDataObjectWriter();
  ~vtkGenericDataObjectWriter();

  void WriteData();
  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkGenericDataObjectWriter(const vtkGenericDataObjectWriter&);  // Not implemented.
  void operator=(const vtkGenericDataObjectWriter&);  // Not implemented.
};

#endif
