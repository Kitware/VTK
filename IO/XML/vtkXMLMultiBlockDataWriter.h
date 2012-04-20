/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLMultiBlockDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLMultiBlockDataWriter - writer for vtkMultiBlockDataSet.
// .SECTION Description
// vtkXMLMultiBlockDataWriter is a vtkXMLCompositeDataWriter subclass to handle
// vtkMultiBlockDataSet.
#ifndef __vtkXMLMultiBlockDataWriter_h
#define __vtkXMLMultiBlockDataWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLCompositeDataWriter.h"

class VTKIOXML_EXPORT vtkXMLMultiBlockDataWriter : public vtkXMLCompositeDataWriter
{
public:
  static vtkXMLMultiBlockDataWriter* New();
  vtkTypeMacro(vtkXMLMultiBlockDataWriter, vtkXMLCompositeDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the default file extension for files written by this writer.
  virtual const char* GetDefaultFileExtension()
    { return "vtm"; }

//BTX
protected:
  vtkXMLMultiBlockDataWriter();
  ~vtkXMLMultiBlockDataWriter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  virtual int WriteComposite(vtkCompositeDataSet* compositeData,
    vtkXMLDataElement* parent, int &writerIdx);

private:
  vtkXMLMultiBlockDataWriter(const vtkXMLMultiBlockDataWriter&); // Not implemented.
  void operator=(const vtkXMLMultiBlockDataWriter&); // Not implemented.
//ETX
};

#endif


