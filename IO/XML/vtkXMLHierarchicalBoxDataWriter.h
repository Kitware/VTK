/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHierarchicalBoxDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLHierarchicalBoxDataWriter - writer for vtkHierarchicalBoxDataSet
// for backwards compatibility.
// .SECTION Description
// vtkXMLHierarchicalBoxDataWriter is an empty subclass of
// vtkXMLUniformGridAMRWriter for writing vtkUniformGridAMR datasets in
// VTK-XML format.

#ifndef vtkXMLHierarchicalBoxDataWriter_h
#define vtkXMLHierarchicalBoxDataWriter_h

#include "vtkXMLUniformGridAMRWriter.h"

class VTKIOXML_EXPORT vtkXMLHierarchicalBoxDataWriter : public vtkXMLUniformGridAMRWriter
{
public:
  static vtkXMLHierarchicalBoxDataWriter* New();
  vtkTypeMacro(vtkXMLHierarchicalBoxDataWriter, vtkXMLUniformGridAMRWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the default file extension for files written by this writer.
  virtual const char* GetDefaultFileExtension()
    { return "vth"; }

//BTX
protected:
  vtkXMLHierarchicalBoxDataWriter();
  ~vtkXMLHierarchicalBoxDataWriter();

private:
  vtkXMLHierarchicalBoxDataWriter(const vtkXMLHierarchicalBoxDataWriter&); // Not implemented.
  void operator=(const vtkXMLHierarchicalBoxDataWriter&); // Not implemented.
//ETX
};

#endif
