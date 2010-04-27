/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLHierarchicalBoxDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLHierarchicalBoxDataWriter - writer for
// vtkHierarchicalBoxDataSet.
// .SECTION Description
// vtkXMLHierarchicalBoxDataWriter is a vtkXMLCompositeDataWriter subclass to
// handle vtkHierarchicalBoxDataSet.

#ifndef __vtkXMLHierarchicalBoxDataWriter_h
#define __vtkXMLHierarchicalBoxDataWriter_h

#include "vtkXMLCompositeDataWriter.h"

class VTK_IO_EXPORT vtkXMLHierarchicalBoxDataWriter : public vtkXMLCompositeDataWriter
{
public:
  static vtkXMLHierarchicalBoxDataWriter* New();
  vtkTypeMacro(vtkXMLHierarchicalBoxDataWriter, vtkXMLCompositeDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the default file extension for files written by this writer.
  virtual const char* GetDefaultFileExtension()
    { return "vth"; }

//BTX
protected:
  vtkXMLHierarchicalBoxDataWriter();
  ~vtkXMLHierarchicalBoxDataWriter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);

  // Fills up this->AMRBoxes, this->AMRBoxDims with boxes for the dataset.
  virtual void FillDataTypes(vtkCompositeDataSet*);

  // Internal method called recursively to create the xml tree for the children
  // of compositeData.
  virtual int WriteComposite(vtkCompositeDataSet* compositeData, 
    vtkXMLDataElement* parent, int &writerIdx);

  int *AMRBoxes;
  int *AMRBoxDims;
private:
  vtkXMLHierarchicalBoxDataWriter(const vtkXMLHierarchicalBoxDataWriter&); // Not implemented.
  void operator=(const vtkXMLHierarchicalBoxDataWriter&); // Not implemented.
//ETX
};

#endif


