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

#ifndef __vtkXMLPImageDataWriter_h
#define __vtkXMLPImageDataWriter_h

#include "vtkXMLPStructuredDataWriter.h"

class vtkImageData;

class VTK_IO_EXPORT vtkXMLPImageDataWriter : public vtkXMLPStructuredDataWriter
{
public:
  static vtkXMLPImageDataWriter* New();
  vtkTypeRevisionMacro(vtkXMLPImageDataWriter,vtkXMLPStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the writer's input.
  void SetInput(vtkImageData* input);
  vtkImageData* GetInput();
  
  // Description:
  // Get the default file extension for files written by this writer.
  const char* GetDefaultFileExtension();
  
protected:
  vtkXMLPImageDataWriter();
  ~vtkXMLPImageDataWriter();
  
  const char* GetDataSetName();
  void WritePrimaryElementAttributes();
  vtkXMLStructuredDataWriter* CreateStructuredPieceWriter(); 
  
private:
  vtkXMLPImageDataWriter(const vtkXMLPImageDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPImageDataWriter&);  // Not implemented.
};

#endif
