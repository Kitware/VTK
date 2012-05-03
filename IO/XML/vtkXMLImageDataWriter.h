/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLImageDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLImageDataWriter - Write VTK XML ImageData files.
// .SECTION Description
// vtkXMLImageDataWriter writes the VTK XML ImageData file format.
// One image data input can be written into one file in any number of
// streamed pieces.  The standard extension for this writer's file
// format is "vti".  This writer is also used to write a single piece
// of the parallel file format.

// .SECTION See Also
// vtkXMLPImageDataWriter

#ifndef __vtkXMLImageDataWriter_h
#define __vtkXMLImageDataWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLStructuredDataWriter.h"

class vtkImageData;

class VTKIOXML_EXPORT vtkXMLImageDataWriter : public vtkXMLStructuredDataWriter
{
public:
  static vtkXMLImageDataWriter* New();
  vtkTypeMacro(vtkXMLImageDataWriter,vtkXMLStructuredDataWriter);
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
  vtkXMLImageDataWriter();
  ~vtkXMLImageDataWriter();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  void WritePrimaryElementAttributes(ostream &os, vtkIndent indent);
  void GetInputExtent(int* extent);
  const char* GetDataSetName();

private:
  vtkXMLImageDataWriter(const vtkXMLImageDataWriter&);  // Not implemented.
  void operator=(const vtkXMLImageDataWriter&);  // Not implemented.
};

#endif
