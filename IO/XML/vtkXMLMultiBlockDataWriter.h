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
/**
 * @class   vtkXMLMultiBlockDataWriter
 * @brief   writer for vtkMultiBlockDataSet.
 *
 * vtkXMLMultiBlockDataWriter is a vtkXMLCompositeDataWriter subclass to handle
 * vtkMultiBlockDataSet.
*/

#ifndef vtkXMLMultiBlockDataWriter_h
#define vtkXMLMultiBlockDataWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLCompositeDataWriter.h"

class VTKIOXML_EXPORT vtkXMLMultiBlockDataWriter : public vtkXMLCompositeDataWriter
{
public:
  static vtkXMLMultiBlockDataWriter* New();
  vtkTypeMacro(vtkXMLMultiBlockDataWriter, vtkXMLCompositeDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Get the default file extension for files written by this writer.
   */
  virtual const char* GetDefaultFileExtension()
    { return "vtm"; }

protected:
  vtkXMLMultiBlockDataWriter();
  ~vtkXMLMultiBlockDataWriter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  virtual int WriteComposite(vtkCompositeDataSet* compositeData,
    vtkXMLDataElement* parent, int &writerIdx);

private:
  vtkXMLMultiBlockDataWriter(const vtkXMLMultiBlockDataWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLMultiBlockDataWriter&) VTK_DELETE_FUNCTION;

};

#endif


