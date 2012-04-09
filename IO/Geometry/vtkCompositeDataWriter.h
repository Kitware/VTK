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
// .NAME vtkCompositeDataWriter - legacy VTK file writer for vtkCompositeDataSet
// subclasses.
// .SECTION Description
// vtkCompositeDataWriter is a writer for writing legacy VTK files for
// vtkCompositeDataSet and subclasses.

#ifndef __vtkCompositeDataWriter_h
#define __vtkCompositeDataWriter_h

#include "vtkDataWriter.h"

class vtkCompositeDataSet;
class vtkMultiPieceDataSet;
class vtkMultiBlockDataSet;
class vtkHierarchicalBoxDataSet;

class VTK_IO_EXPORT vtkCompositeDataWriter : public vtkDataWriter
{
public:
  static vtkCompositeDataWriter* New();
  vtkTypeMacro(vtkCompositeDataWriter, vtkDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the input to this writer.
  vtkCompositeDataSet* GetInput();
  vtkCompositeDataSet* GetInput(int port);

//BTX
protected:
  vtkCompositeDataWriter();
  ~vtkCompositeDataWriter();

  // Description:
  // Performs the actual writing.
  virtual void WriteData();
  virtual int FillInputPortInformation(int port, vtkInformation *info);

  bool WriteCompositeData(ostream*, vtkMultiBlockDataSet*);
  bool WriteCompositeData(ostream*, vtkMultiPieceDataSet*);
  bool WriteCompositeData(ostream*, vtkHierarchicalBoxDataSet*);
  bool WriteBlock(ostream* fp, vtkDataObject* block);

private:
  vtkCompositeDataWriter(const vtkCompositeDataWriter&); // Not implemented
  void operator=(const vtkCompositeDataWriter&); // Not implemented
//ETX
};

#endif
