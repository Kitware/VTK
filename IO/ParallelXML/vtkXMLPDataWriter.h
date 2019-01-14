/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPDataWriter
 * @brief   Write data in a parallel XML format.
 *
 * vtkXMLPDataWriter is the superclass for all XML parallel data set
 * writers.  It provides functionality needed for writing parallel
 * formats, such as the selection of which writer writes the summary
 * file and what range of pieces are assigned to each serial writer.
*/

#ifndef vtkXMLPDataWriter_h
#define vtkXMLPDataWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPDataObjectWriter.h"

class vtkCallbackCommand;
class vtkMultiProcessController;

class VTKIOPARALLELXML_EXPORT vtkXMLPDataWriter : public vtkXMLPDataObjectWriter
{
public:
  vtkTypeMacro(vtkXMLPDataWriter, vtkXMLPDataObjectWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkXMLPDataWriter();
  ~vtkXMLPDataWriter() override;

  virtual vtkXMLWriter* CreatePieceWriter(int index) = 0;

  void WritePData(vtkIndent indent) override;

  int WritePieceInternal() override;

  int WritePiece(int index) override;

  void WritePrimaryElementAttributes(ostream& os, vtkIndent indent) override;

private:
  vtkXMLPDataWriter(const vtkXMLPDataWriter&) = delete;
  void operator=(const vtkXMLPDataWriter&) = delete;

  /**
   * Initializes PieceFileNameExtension.
   */
  void SetupPieceFileNameExtension() override;
};

#endif
