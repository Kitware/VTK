/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStructuredGridWriter
 * @brief   write vtk structured grid data file
 *
 * vtkStructuredGridWriter is a source object that writes ASCII or binary
 * structured grid data files in vtk format. See text for format details.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
*/

#ifndef vtkStructuredGridWriter_h
#define vtkStructuredGridWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"

class vtkStructuredGrid;

class VTKIOLEGACY_EXPORT vtkStructuredGridWriter : public vtkDataWriter
{
public:
  static vtkStructuredGridWriter *New();
  vtkTypeMacro(vtkStructuredGridWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get the input to this writer.
   */
  vtkStructuredGrid* GetInput();
  vtkStructuredGrid* GetInput(int port);
  //@}

protected:
  vtkStructuredGridWriter() {}
  ~vtkStructuredGridWriter() {}

  void WriteData();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkStructuredGridWriter(const vtkStructuredGridWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStructuredGridWriter&) VTK_DELETE_FUNCTION;
};

#endif
