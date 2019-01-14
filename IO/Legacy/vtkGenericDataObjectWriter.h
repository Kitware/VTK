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
/**
 * @class   vtkGenericDataObjectWriter
 * @brief   writes any type of vtk data object to file
 *
 * vtkGenericDataObjectWriter is a concrete class that writes data objects
 * to disk. The input to this object is any subclass of vtkDataObject.
*/

#ifndef vtkGenericDataObjectWriter_h
#define vtkGenericDataObjectWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"

class VTKIOLEGACY_EXPORT vtkGenericDataObjectWriter : public vtkDataWriter
{
public:
  static vtkGenericDataObjectWriter *New();
  vtkTypeMacro(vtkGenericDataObjectWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkGenericDataObjectWriter();
  ~vtkGenericDataObjectWriter() override;

  void WriteData() override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

private:
  vtkGenericDataObjectWriter(const vtkGenericDataObjectWriter&) = delete;
  void operator=(const vtkGenericDataObjectWriter&) = delete;
};

#endif
