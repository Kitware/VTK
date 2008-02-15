/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPMultiBlockDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPMultiBlockDataWriter - parallel writer for
// vtkHierarchicalBoxDataSet.
// .SECTION Description
// vtkXMLPCompositeDataWriter writes (in parallel or serially) the VTK XML
// multi-group, multi-block hierarchical and hierarchical box files. XML
// multi-group data files are meta-files that point to a list of serial VTK
// XML files.

#ifndef __vtkXMLPMultiBlockDataWriter_h
#define __vtkXMLPMultiBlockDataWriter_h

#include "vtkXMLMultiBlockDataWriter.h"

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkXMLPMultiBlockDataWriter : public vtkXMLMultiBlockDataWriter
{
public:
  static vtkXMLPMultiBlockDataWriter* New();
  vtkTypeRevisionMacro(vtkXMLPMultiBlockDataWriter, vtkXMLMultiBlockDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controller used to communicate data type of blocks.
  // By default, the global controller is used. If you want another
  // controller to be used, set it with this.
  // If no controller is set, only the local blocks will be written
  // to the meta-file.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

//BTX
protected:
  vtkXMLPMultiBlockDataWriter();
  ~vtkXMLPMultiBlockDataWriter();

  
  virtual void FillDataTypes(vtkCompositeDataSet*);

  vtkMultiProcessController* Controller;

private:
  vtkXMLPMultiBlockDataWriter(const vtkXMLPMultiBlockDataWriter&); // Not implemented.
  void operator=(const vtkXMLPMultiBlockDataWriter&); // Not implemented.
//ETX
};

#endif


