/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

class vtkCallbackCommand;

class VTK_IO_EXPORT vtkXMLDataSetWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLDataSetWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLDataSetWriter* New();
  
  //BTX
  // Description:
  // Get/Set the writer's input.
  vtkDataSet* GetInput();
  //ETX

protected:
  vtkXMLDataSetWriter();
  ~vtkXMLDataSetWriter();
  
  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Override writing method from superclass.
  virtual int WriteInternal();
  
  // Dummies to satisfy pure virtuals from superclass.
  const char* GetDataSetName();
  const char* GetDefaultFileExtension();
  
  // Callback registered with the ProgressObserver.
  static void ProgressCallbackFunction(vtkObject*, unsigned long, void*,
                                       void*);
  // Progress callback from internal writer.
  virtual void ProgressCallback(vtkAlgorithm* w);
  
  // The observer to report progress from the internal writer.
  vtkCallbackCommand* ProgressObserver;  
  
private:
  vtkXMLDataSetWriter(const vtkXMLDataSetWriter&);  // Not implemented.
  void operator=(const vtkXMLDataSetWriter&);  // Not implemented.
};

#endif
