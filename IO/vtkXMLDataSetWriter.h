/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataSetWriter.h
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
// .NAME vtkXMLDataSetWriter - Write any type of VTK XML file.
// .SECTION Description
// vtkXMLDataSetWriter is a wrapper around the VTK XML file format
// writers.  Given an input vtkDataSet, the correct writer is
// automatically selected based on the type of input.

// .SECTION See Also
// vtkXMLImageDataWriter vtkXMLStructuredGridWriter
// vtkXMLRectilinearGridWriter vtkXMLPolyDataWriter
// vtkXMLUnstructuredGridWriter

#ifndef __vtkXMLDataSetWriter_h
#define __vtkXMLDataSetWriter_h

#include "vtkXMLWriter.h"

class VTK_IO_EXPORT vtkXMLDataSetWriter : public vtkXMLWriter
{
public:
  vtkTypeRevisionMacro(vtkXMLDataSetWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLDataSetWriter* New();
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkDataSet* input);
  vtkDataSet* GetInput();
  
  // Description:
  // Invoke the writer.  Returns 1 for success, 0 for failure.
  virtual int Write();
  
protected:
  vtkXMLDataSetWriter();
  ~vtkXMLDataSetWriter();
  
  // Dummies to satisfy pure virtuals from superclass.
  int WriteData();
  const char* GetDataSetName();
  const char* GetDefaultFileExtension();
  
private:
  vtkXMLDataSetWriter(const vtkXMLDataSetWriter&);  // Not implemented.
  void operator=(const vtkXMLDataSetWriter&);  // Not implemented.
};

#endif
