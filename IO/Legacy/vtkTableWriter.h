/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkTableWriter
 * @brief   write vtkTable to a file
 *
 * vtkTableWriter is a sink object that writes ASCII or binary
 * vtkTable data files in vtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
*/

#ifndef vtkTableWriter_h
#define vtkTableWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"
class vtkTable;

class VTKIOLEGACY_EXPORT vtkTableWriter : public vtkDataWriter
{
public:
  static vtkTableWriter *New();
  vtkTypeMacro(vtkTableWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  vtkTable* GetInput();
  vtkTable* GetInput(int port);
  //@}

protected:
  vtkTableWriter() {}
  ~vtkTableWriter() override {}

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation *info) override;

private:
  vtkTableWriter(const vtkTableWriter&) = delete;
  void operator=(const vtkTableWriter&) = delete;
};

#endif
