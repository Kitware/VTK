/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTreeWriter
 * @brief   write vtkTree data to a file
 *
 * vtkTreeWriter is a sink object that writes ASCII or binary
 * vtkTree data files in vtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
*/

#ifndef vtkTreeWriter_h
#define vtkTreeWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"

class vtkTree;

class VTKIOLEGACY_EXPORT vtkTreeWriter : public vtkDataWriter
{
public:
  static vtkTreeWriter *New();
  vtkTypeMacro(vtkTreeWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  vtkTree* GetInput();
  vtkTree* GetInput(int port);
  //@}

protected:
  vtkTreeWriter() {}
  ~vtkTreeWriter() override {}

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation *info) override;

private:
  vtkTreeWriter(const vtkTreeWriter&) = delete;
  void operator=(const vtkTreeWriter&) = delete;

  void WriteEdges(ostream& Stream, vtkTree* Tree);
};

#endif
