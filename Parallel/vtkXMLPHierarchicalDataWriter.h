/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLPHierarchicalDataWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPHierarchicalDataWriter -  Writer for hierarchical datasets
// .SECTION Description
// vtkXMLHierarchicalDataWriter writes (in parallel or serially) the VTK
// XML hierarchical and hierarchical box files. XML hierarchical data files
// are meta-files that point to a list of serial VTK XML files.

#ifndef __vtkXMLPHierarchicalDataWriter_h
#define __vtkXMLPHierarchicalDataWriter_h

#include "vtkXMLHierarchicalDataWriter.h"

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkXMLPHierarchicalDataWriter : public vtkXMLHierarchicalDataWriter
{
public:
  static vtkXMLPHierarchicalDataWriter* New();
  vtkTypeRevisionMacro(vtkXMLPHierarchicalDataWriter,vtkXMLHierarchicalDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);  

  // Description:
  // Controller used to communicate data type of blocks.
  // By default, the global controller is used. If you want another
  // controller to be used, set it with this.
  // If no controller is set, only the local blocks will be written
  // to the meta-file.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  
protected:
  vtkXMLPHierarchicalDataWriter();
  ~vtkXMLPHierarchicalDataWriter();
  
  virtual void FillDataTypes(vtkHierarchicalDataSet*);

  vtkMultiProcessController* Controller;

private:
  vtkXMLPHierarchicalDataWriter(const vtkXMLPHierarchicalDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPHierarchicalDataWriter&);  // Not implemented.
};

#endif
