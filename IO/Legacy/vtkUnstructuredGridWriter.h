/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUnstructuredGridWriter
 * @brief   write vtk unstructured grid data file
 *
 * vtkUnstructuredGridWriter is a source object that writes ASCII or binary
 * unstructured grid data files in vtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
*/

#ifndef vtkUnstructuredGridWriter_h
#define vtkUnstructuredGridWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"
class vtkUnstructuredGrid;

class VTKIOLEGACY_EXPORT vtkUnstructuredGridWriter : public vtkDataWriter
{
public:
  static vtkUnstructuredGridWriter *New();
  vtkTypeMacro(vtkUnstructuredGridWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  vtkUnstructuredGrid* GetInput();
  vtkUnstructuredGrid* GetInput(int port);
  //@}

protected:
  vtkUnstructuredGridWriter() {}
  ~vtkUnstructuredGridWriter() override {}

  void WriteData() override;

  int WriteCellsAndFaces(ostream *fp, vtkUnstructuredGrid *grid,
                         const char *label);

  int FillInputPortInformation(int port, vtkInformation *info) override;

private:
  vtkUnstructuredGridWriter(const vtkUnstructuredGridWriter&) = delete;
  void operator=(const vtkUnstructuredGridWriter&) = delete;
};

#endif
