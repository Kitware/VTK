/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkGraphWriter
 * @brief   write vtkGraph data to a file
 *
 * vtkGraphWriter is a sink object that writes ASCII or binary
 * vtkGraph data files in vtk format. See text for format details.
 * @warning
 * Binary files written on one system may not be readable on other systems.
*/

#ifndef vtkGraphWriter_h
#define vtkGraphWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"

class vtkGraph;
class vtkMolecule;

class VTKIOLEGACY_EXPORT vtkGraphWriter : public vtkDataWriter
{
public:
  static vtkGraphWriter *New();
  vtkTypeMacro(vtkGraphWriter,vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get the input to this writer.
   */
  vtkGraph* GetInput();
  vtkGraph* GetInput(int port);
  //@}

protected:
  vtkGraphWriter() {}
  ~vtkGraphWriter() {}

  void WriteData();

  void WriteMoleculeData(ostream *fp, vtkMolecule *m);

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkGraphWriter(const vtkGraphWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGraphWriter&) VTK_DELETE_FUNCTION;
};

#endif
