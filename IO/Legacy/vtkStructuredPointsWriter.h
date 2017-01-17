/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStructuredPointsWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkStructuredPointsWriter
 * @brief   write vtk structured points data file
 *
 * vtkStructuredPointsWriter is a source object that writes ASCII or binary
 * structured points data in vtk file format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
*/

#ifndef vtkStructuredPointsWriter_h
#define vtkStructuredPointsWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"

class vtkImageData;

class VTKIOLEGACY_EXPORT vtkStructuredPointsWriter : public vtkDataWriter
{
public:
  static vtkStructuredPointsWriter *New();
  vtkTypeMacro(vtkStructuredPointsWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get the input to this writer.
   */
  vtkImageData* GetInput();
  vtkImageData* GetInput(int port);
  //@}

protected:
  vtkStructuredPointsWriter() {}
  ~vtkStructuredPointsWriter() VTK_OVERRIDE {}

  void WriteData() VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

private:
  vtkStructuredPointsWriter(const vtkStructuredPointsWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkStructuredPointsWriter&) VTK_DELETE_FUNCTION;
};

#endif
