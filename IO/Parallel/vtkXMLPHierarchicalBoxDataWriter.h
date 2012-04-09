/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPHierarchicalBoxDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPHierarchicalBoxDataWriter - parallel writer for
// vtkHierarchicalBoxDataSet.
// .SECTION Description
// vtkXMLPCompositeDataWriter writes (in parallel or serially) the VTK XML
// multi-group, multi-block hierarchical and hierarchical box files. XML
// multi-group data files are meta-files that point to a list of serial VTK
// XML files.

#ifndef __vtkXMLPHierarchicalBoxDataWriter_h
#define __vtkXMLPHierarchicalBoxDataWriter_h

#include "vtkXMLHierarchicalBoxDataWriter.h"

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkXMLPHierarchicalBoxDataWriter : public vtkXMLHierarchicalBoxDataWriter
{
public:
  static vtkXMLPHierarchicalBoxDataWriter* New();
  vtkTypeMacro(vtkXMLPHierarchicalBoxDataWriter, vtkXMLHierarchicalBoxDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controller used to communicate data type of blocks.
  // By default, the global controller is used. If you want another
  // controller to be used, set it with this.
  // If no controller is set, only the local blocks will be written
  // to the meta-file.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Set whether this instance will write the meta-file. WriteMetaFile
  // is set to flag only on process 0 and all other processes have
  // WriteMetaFile set to 0 by default.
  virtual void SetWriteMetaFile(int flag);

//BTX
protected:
  vtkXMLPHierarchicalBoxDataWriter();
  ~vtkXMLPHierarchicalBoxDataWriter();

  
  virtual void FillDataTypes(vtkCompositeDataSet*);

  vtkMultiProcessController* Controller;

private:
  vtkXMLPHierarchicalBoxDataWriter(const vtkXMLPHierarchicalBoxDataWriter&); // Not implemented.
  void operator=(const vtkXMLPHierarchicalBoxDataWriter&); // Not implemented.
//ETX
};

#endif


