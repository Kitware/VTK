/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPDataSetWriter - Write any type of PVTK XML file.
// .SECTION Description
// vtkXMLPDataSetWriter is a wrapper around the PVTK XML file format
// writers.  Given an input vtkDataSet, the correct writer is
// automatically selected based on the type of input.

// .SECTION See Also
// vtkXMLPImageDataWriter vtkXMLPStructuredGridWriter
// vtkXMLPRectilinearGridWriter vtkXMLPPolyDataWriter
// vtkXMLPUnstructuredGridWriter

#ifndef vtkXMLPDataSetWriter_h
#define vtkXMLPDataSetWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPDataWriter.h"

class VTKIOPARALLELXML_EXPORT vtkXMLPDataSetWriter : public vtkXMLPDataWriter
{
public:
  vtkTypeMacro(vtkXMLPDataSetWriter,vtkXMLPDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPDataSetWriter* New();

  //BTX
  // Description:
  // Get/Set the writer's input.
  vtkDataSet* GetInput();
  //ETX

protected:
  vtkXMLPDataSetWriter();
  ~vtkXMLPDataSetWriter();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Override writing method from superclass.
  virtual int WriteInternal();

  // Dummies to satisfy pure virtuals from superclass.
  const char* GetDataSetName();
  const char* GetDefaultFileExtension();
  vtkXMLWriter* CreatePieceWriter(int index);

private:
  vtkXMLPDataSetWriter(const vtkXMLPDataSetWriter&);  // Not implemented.
  void operator=(const vtkXMLPDataSetWriter&);  // Not implemented.
};

#endif
