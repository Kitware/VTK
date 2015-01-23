/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPImageDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPImageDataWriter - Write PVTK XML ImageData files.
// .SECTION Description
// vtkXMLPImageDataWriter writes the PVTK XML ImageData file format.
// One image data input can be written into a parallel file format
// with any number of pieces spread across files.  The standard
// extension for this writer's file format is "pvti".  This writer
// uses vtkXMLImageDataWriter to write the individual piece files.

// .SECTION See Also
// vtkXMLImageDataWriter

#ifndef vtkXMLPImageDataWriter_h
#define vtkXMLPImageDataWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPStructuredDataWriter.h"

class vtkImageData;

class VTKIOPARALLELXML_EXPORT vtkXMLPImageDataWriter : public vtkXMLPStructuredDataWriter
{
public:
  static vtkXMLPImageDataWriter* New();
  vtkTypeMacro(vtkXMLPImageDataWriter,vtkXMLPStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //BTX
  // Description:
  // Get/Set the writer's input.
  vtkImageData* GetInput();
  //ETX

  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();

protected:
  vtkXMLPImageDataWriter();
  ~vtkXMLPImageDataWriter();

  const char* GetDataSetName();
  void WritePrimaryElementAttributes(ostream &os, vtkIndent indent);
  vtkXMLStructuredDataWriter* CreateStructuredPieceWriter();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkXMLPImageDataWriter(const vtkXMLPImageDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPImageDataWriter&);  // Not implemented.
};

#endif
