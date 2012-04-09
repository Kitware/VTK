/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPUnstructuredGridWriter - Write PVTK XML UnstructuredGrid files.
// .SECTION Description
// vtkXMLPUnstructuredGridWriter writes the PVTK XML UnstructuredGrid
// file format.  One unstructured grid input can be written into a
// parallel file format with any number of pieces spread across files.
// The standard extension for this writer's file format is "pvtu".
// This writer uses vtkXMLUnstructuredGridWriter to write the
// individual piece files.

// .SECTION See Also
// vtkXMLUnstructuredGridWriter

#ifndef __vtkXMLPUnstructuredGridWriter_h
#define __vtkXMLPUnstructuredGridWriter_h

#include "vtkXMLPUnstructuredDataWriter.h"

class vtkUnstructuredGrid;

class VTK_IO_EXPORT vtkXMLPUnstructuredGridWriter : public vtkXMLPUnstructuredDataWriter
{
public:
  static vtkXMLPUnstructuredGridWriter* New();
  vtkTypeMacro(vtkXMLPUnstructuredGridWriter,vtkXMLPUnstructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Get/Set the writer's input.
  vtkUnstructuredGrid* GetInput();
  //ETX
  
  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();
  
protected:
  vtkXMLPUnstructuredGridWriter();
  ~vtkXMLPUnstructuredGridWriter();
  
  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  const char* GetDataSetName();
  vtkXMLUnstructuredDataWriter* CreateUnstructuredPieceWriter(); 
  
private:
  vtkXMLPUnstructuredGridWriter(const vtkXMLPUnstructuredGridWriter&);  // Not implemented.
  void operator=(const vtkXMLPUnstructuredGridWriter&);  // Not implemented.
};

#endif
