/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCompositeDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCompositeDataWriter
 * @brief   legacy VTK file writer for vtkCompositeDataSet
 * subclasses.
 *
 * vtkCompositeDataWriter is a writer for writing legacy VTK files for
 * vtkCompositeDataSet and subclasses.
 * @warning
 * This is an experimental format. Use XML-based formats for writing composite
 * datasets. Saving composite dataset in legacy VTK format is expected to change
 * in future including changes to the file layout.
*/

#ifndef vtkCompositeDataWriter_h
#define vtkCompositeDataWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkDataWriter.h"

class vtkCompositeDataSet;
class vtkHierarchicalBoxDataSet;
class vtkMultiBlockDataSet;
class vtkMultiPieceDataSet;
class vtkNonOverlappingAMR;
class vtkOverlappingAMR;

class VTKIOLEGACY_EXPORT vtkCompositeDataWriter : public vtkDataWriter
{
public:
  static vtkCompositeDataWriter* New();
  vtkTypeMacro(vtkCompositeDataWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get the input to this writer.
   */
  vtkCompositeDataSet* GetInput();
  vtkCompositeDataSet* GetInput(int port);
  //@}

protected:
  vtkCompositeDataWriter();
  ~vtkCompositeDataWriter();

  //@{
  /**
   * Performs the actual writing.
   */
  virtual void WriteData();
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  //@}

  bool WriteCompositeData(ostream*, vtkMultiBlockDataSet*);
  bool WriteCompositeData(ostream*, vtkMultiPieceDataSet*);
  bool WriteCompositeData(ostream*, vtkHierarchicalBoxDataSet*);
  bool WriteCompositeData(ostream*, vtkOverlappingAMR*);
  bool WriteCompositeData(ostream*, vtkNonOverlappingAMR*);
  bool WriteBlock(ostream* fp, vtkDataObject* block);

private:
  vtkCompositeDataWriter(const vtkCompositeDataWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCompositeDataWriter&) VTK_DELETE_FUNCTION;

};

#endif
