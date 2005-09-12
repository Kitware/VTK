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
// vtkXMLHierarchicalBoxDataReader reads the VTK XML hierarchical box data
// file (uniform grid amr) format. XML hierarchical data files are
// meta-files that point to a list of serial VTK XML files. Reading in
// parallel is not yet supported.
// .SECTION See Also
// vtkXMLHierarchicalDataReader

#ifndef __vtkXMLHierarchicalBoxDataReader_h
#define __vtkXMLHierarchicalBoxDataReader_h

#include "vtkXMLHierarchicalDataReader.h"

class VTK_IO_EXPORT vtkXMLHierarchicalBoxDataReader : public vtkXMLHierarchicalDataReader
{
public:
  static vtkXMLHierarchicalBoxDataReader* New();
  vtkTypeRevisionMacro(vtkXMLHierarchicalBoxDataReader,vtkXMLHierarchicalDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkXMLHierarchicalBoxDataReader();
  ~vtkXMLHierarchicalBoxDataReader();  

  // Get the name of the data set being read.
  virtual const char* GetDataSetName();

  virtual void ReadXMLData();
  virtual int FillOutputPortInformation(int, vtkInformation* info);

  virtual void HandleBlock(
    vtkXMLDataElement* ds, int level, int dsId, 
    vtkHierarchicalDataSet* output, vtkDataSet* data);
  
private:
  vtkXMLHierarchicalBoxDataReader(const vtkXMLHierarchicalBoxDataReader&);  // Not implemented.
  void operator=(const vtkXMLHierarchicalBoxDataReader&);  // Not implemented.

};

#endif
