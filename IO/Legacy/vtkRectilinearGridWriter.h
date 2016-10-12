/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRectilinearGridWriter
 * @brief   write vtk rectilinear grid data file
 *
 * vtkRectilinearGridWriter is a source object that writes ASCII or binary
 * rectilinear grid data files in vtk format. See text for format details.
 *
 * @warning
 * Binary files written on one system may not be readable on other systems.
*/

#ifndef vtkRectilinearGridWriter_h
#define vtkRectilinearGridWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"

class vtkRectilinearGrid;

class VTKIOLEGACY_EXPORT vtkRectilinearGridWriter : public vtkDataWriter
{
public:
  static vtkRectilinearGridWriter *New();
  vtkTypeMacro(vtkRectilinearGridWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get the input to this writer.
   */
  vtkRectilinearGrid* GetInput();
  vtkRectilinearGrid* GetInput(int port);
  //@}

protected:
  vtkRectilinearGridWriter() {}
  ~vtkRectilinearGridWriter() {}

  void WriteData();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkRectilinearGridWriter(const vtkRectilinearGridWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRectilinearGridWriter&) VTK_DELETE_FUNCTION;
};

#endif
