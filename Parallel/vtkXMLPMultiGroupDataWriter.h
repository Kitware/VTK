/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLPMultiGroupDataWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPMultiGroupDataWriter -  Writer for hierarchical datasets
// .SECTION Description
// vtkXMLMultiGroupDataWriter writes (in parallel or serially) the VTK
// XML hierarchical and hierarchical box files. XML hierarchical data files
// are meta-files that point to a list of serial VTK XML files.

#ifndef __vtkXMLPMultiGroupDataWriter_h
#define __vtkXMLPMultiGroupDataWriter_h

#include "vtkXMLMultiGroupDataWriter.h"

class vtkMultiProcessController;

class VTK_PARALLEL_EXPORT vtkXMLPMultiGroupDataWriter : public vtkXMLMultiGroupDataWriter
{
public:
  static vtkXMLPMultiGroupDataWriter* New();
  vtkTypeRevisionMacro(vtkXMLPMultiGroupDataWriter,vtkXMLMultiGroupDataWriter);
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
  vtkXMLPMultiGroupDataWriter();
  ~vtkXMLPMultiGroupDataWriter();
  
  virtual void FillDataTypes(vtkMultiGroupDataSet*);

  vtkMultiProcessController* Controller;

private:
  vtkXMLPMultiGroupDataWriter(const vtkXMLPMultiGroupDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPMultiGroupDataWriter&);  // Not implemented.
};

#endif
