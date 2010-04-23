/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLHierarchicalBoxDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLHierarchicalBoxDataReader - Reader for hierarchical datasets
// .SECTION Description
// vtkXMLHierarchicalBoxDataReader reads the VTK XML hierarchical data file
// format. XML hierarchical data files are meta-files that point to a list
// of serial VTK XML files. When reading in parallel, it will distribute
// sub-blocks among processor. If the number of sub-blocks is less than
// the number of processors, some processors will not have any sub-blocks
// for that level. If the number of sub-blocks is larger than the
// number of processors, each processor will possibly have more than
// 1 sub-block.

#ifndef __vtkXMLHierarchicalBoxDataReader_h
#define __vtkXMLHierarchicalBoxDataReader_h

#include "vtkXMLCompositeDataReader.h"

class VTK_IO_EXPORT vtkXMLHierarchicalBoxDataReader : public vtkXMLCompositeDataReader
{
public:
  static vtkXMLHierarchicalBoxDataReader* New();
  vtkTypeMacro(vtkXMLHierarchicalBoxDataReader,vtkXMLCompositeDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkXMLHierarchicalBoxDataReader();
  ~vtkXMLHierarchicalBoxDataReader();  

  // Get the name of the data set being read.
  virtual const char* GetDataSetName();

  virtual int FillOutputPortInformation(int, vtkInformation* info);

  // Read the XML element for the subtree of a the composite dataset.
  // dataSetIndex is used to rank the leaf nodes in an inorder traversal.
  virtual void ReadComposite(vtkXMLDataElement* element, 
    vtkCompositeDataSet* composite, const char* filePath, 
    unsigned int &dataSetIndex);

  // Read the vtkDataSet (a leaf) in the composite dataset.
  virtual vtkDataSet* ReadDataset(vtkXMLDataElement* xmlElem, const char* filePath);

  // Read v0.1
  virtual void ReadVersion0(vtkXMLDataElement* element, 
    vtkCompositeDataSet* composite, const char* filePath, 
    unsigned int &dataSetIndex);


private:
  vtkXMLHierarchicalBoxDataReader(const vtkXMLHierarchicalBoxDataReader&);  // Not implemented.
  void operator=(const vtkXMLHierarchicalBoxDataReader&);  // Not implemented.

};

#endif
