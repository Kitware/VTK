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
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get the input to this writer.
   */
  vtkUnstructuredGrid* GetInput();
  vtkUnstructuredGrid* GetInput(int port);
  //@}

protected:
  vtkUnstructuredGridWriter() {}
  ~vtkUnstructuredGridWriter() {}

  void WriteData();

  int WriteCellsAndFaces(ostream *fp, vtkUnstructuredGrid *grid,
                         const char *label);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkUnstructuredGridWriter(const vtkUnstructuredGridWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkUnstructuredGridWriter&) VTK_DELETE_FUNCTION;
};

#endif
