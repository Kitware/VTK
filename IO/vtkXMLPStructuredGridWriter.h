/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredGridWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPStructuredGridWriter - Write PVTK XML StructuredGrid files.
// .SECTION Description
// vtkXMLPStructuredGridWriter writes the PVTK XML StructuredGrid
// file format.  One structured grid input can be written into a
// parallel file format with any number of pieces spread across files.
// The standard extension for this writer's file format is "pvts".
// This writer uses vtkXMLStructuredGridWriter to write the individual
// piece files.

// .SECTION See Also
// vtkXMLStructuredGridWriter

#ifndef __vtkXMLPStructuredGridWriter_h
#define __vtkXMLPStructuredGridWriter_h

#include "vtkXMLPStructuredDataWriter.h"

class vtkStructuredGrid;

class VTK_IO_EXPORT vtkXMLPStructuredGridWriter : public vtkXMLPStructuredDataWriter
{
public:
  static vtkXMLPStructuredGridWriter* New();
  vtkTypeRevisionMacro(vtkXMLPStructuredGridWriter,vtkXMLPStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkStructuredGrid* input);
  vtkStructuredGrid* GetInput();
  
protected:
  vtkXMLPStructuredGridWriter();
  ~vtkXMLPStructuredGridWriter();
  
  const char* GetDataSetName();
  const char* GetDefaultFileExtension();
  vtkXMLStructuredDataWriter* CreateStructuredPieceWriter(); 
  
private:
  vtkXMLPStructuredGridWriter(const vtkXMLPStructuredGridWriter&);  // Not implemented.
  void operator=(const vtkXMLPStructuredGridWriter&);  // Not implemented.
};

#endif
