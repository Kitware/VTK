/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPolyDataWriter
 * @brief   write vtk polygonal data
 *
 * vtkPolyDataWriter is a source object that writes ASCII or binary
 * polygonal data files in vtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
*/

#ifndef vtkPolyDataWriter_h
#define vtkPolyDataWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"

class vtkPolyData;

class VTKIOLEGACY_EXPORT vtkPolyDataWriter : public vtkDataWriter
{
public:
  static vtkPolyDataWriter *New();
  vtkTypeMacro(vtkPolyDataWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get the input to this writer.
   */
  vtkPolyData* GetInput();
  vtkPolyData* GetInput(int port);
  //@}

protected:
  vtkPolyDataWriter() {}
  ~vtkPolyDataWriter() {}

  void WriteData();

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkPolyDataWriter(const vtkPolyDataWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPolyDataWriter&) VTK_DELETE_FUNCTION;
};

#endif
