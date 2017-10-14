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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Get the input to this writer.
   */
  vtkStructuredGrid* GetInput();
  vtkStructuredGrid* GetInput(int port);
  //@}

  //@{
  /**
  * When WriteExtent is on, vtkStructuredPointsWriter writes
  * data extent in the output file. Otherwise, it writes dimensions.
  * The only time this option is useful is when the extents do
  * not start at (0, 0, 0). This is an options to support writing
  * of older formats while still using a newer VTK.
  */
  vtkSetMacro(WriteExtent, bool);
  vtkGetMacro(WriteExtent, bool);
  vtkBooleanMacro(WriteExtent, bool);
  //@}

protected:
  vtkStructuredGridWriter() : WriteExtent(false) {}
  ~vtkStructuredGridWriter() override {}

  void WriteData() override;

  int FillInputPortInformation(int port, vtkInformation *info) override;

  bool WriteExtent;

private:
  vtkStructuredGridWriter(const vtkStructuredGridWriter&) = delete;
  void operator=(const vtkStructuredGridWriter&) = delete;
};

#endif
