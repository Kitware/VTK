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

#include "vtkXMLHierarchicalDataReader.h"

//BTX
struct vtkXMLHierarchicalBoxDataReaderInternals;
//ETX

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
  virtual int ReadPrimaryElement(vtkXMLDataElement* ePrimary);
  virtual int FillOutputPortInformation(int, vtkInformation* info);

  virtual void HandleDataSet(
    vtkXMLDataElement* ds, int level, int dsId, 
    vtkMultiGroupDataSet* output, vtkDataSet* data);
  
private:
  vtkXMLHierarchicalBoxDataReaderInternals* Internal;

  vtkXMLHierarchicalBoxDataReader(const vtkXMLHierarchicalBoxDataReader&);  // Not implemented.
  void operator=(const vtkXMLHierarchicalBoxDataReader&);  // Not implemented.

};

#endif
