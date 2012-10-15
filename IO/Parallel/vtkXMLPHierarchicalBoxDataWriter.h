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
// vtkHierarchicalBoxDataSet for backwards compatibility.
// .SECTION Description
// vtkXMLPHierarchicalBoxDataWriter is an empty subclass of
// vtkXMLPUniformGridAMRWriter for backwards compatibility.

#ifndef __vtkXMLPHierarchicalBoxDataWriter_h
#define __vtkXMLPHierarchicalBoxDataWriter_h

#include "vtkXMLPUniformGridAMRWriter.h"

class VTKIOPARALLEL_EXPORT vtkXMLPHierarchicalBoxDataWriter :
  public vtkXMLPUniformGridAMRWriter
{
public:
  static vtkXMLPHierarchicalBoxDataWriter* New();
  vtkTypeMacro(vtkXMLPHierarchicalBoxDataWriter, vtkXMLPUniformGridAMRWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkXMLPHierarchicalBoxDataWriter();
  ~vtkXMLPHierarchicalBoxDataWriter();

private:
  vtkXMLPHierarchicalBoxDataWriter(const vtkXMLPHierarchicalBoxDataWriter&); // Not implemented.
  void operator=(const vtkXMLPHierarchicalBoxDataWriter&); // Not implemented.
};

#endif
