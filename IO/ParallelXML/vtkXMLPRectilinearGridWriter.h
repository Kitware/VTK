/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPRectilinearGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPRectilinearGridWriter - Write PVTK XML RectilinearGrid files.
// .SECTION Description
// vtkXMLPRectilinearGridWriter writes the PVTK XML RectilinearGrid
// file format.  One rectilinear grid input can be written into a
// parallel file format with any number of pieces spread across files.
// The standard extension for this writer's file format is "pvtr".
// This writer uses vtkXMLRectilinearGridWriter to write the
// individual piece files.

// .SECTION See Also
// vtkXMLRectilinearGridWriter

#ifndef vtkXMLPRectilinearGridWriter_h
#define vtkXMLPRectilinearGridWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPStructuredDataWriter.h"

class vtkRectilinearGrid;

class VTKIOPARALLELXML_EXPORT vtkXMLPRectilinearGridWriter : public vtkXMLPStructuredDataWriter
{
public:
  static vtkXMLPRectilinearGridWriter* New();
  vtkTypeMacro(vtkXMLPRectilinearGridWriter,vtkXMLPStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Get/Set the writer's input.
  vtkRectilinearGrid* GetInput();
  //ETX

  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();

protected:
  vtkXMLPRectilinearGridWriter();
  ~vtkXMLPRectilinearGridWriter();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  const char* GetDataSetName();
  vtkXMLStructuredDataWriter* CreateStructuredPieceWriter();
  void WritePData(vtkIndent indent);

private:
  vtkXMLPRectilinearGridWriter(const vtkXMLPRectilinearGridWriter&);  // Not implemented.
  void operator=(const vtkXMLPRectilinearGridWriter&);  // Not implemented.
};

#endif
