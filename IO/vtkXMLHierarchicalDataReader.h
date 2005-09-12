/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLHierarchicalDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLHierarchicalDataReader - Reader for hierarchical datasets
// .SECTION Description
// vtkXMLHierarchicalDataReader reads the VTK XML hierarchical data file
// (multi-block and amr) format. XML hierarchical data files are meta-files
// that point to a list of serial VTK XML files. Reading in parallel is not
// yet supported.

#ifndef __vtkXMLHierarchicalDataReader_h
#define __vtkXMLHierarchicalDataReader_h

#include "vtkXMLReader.h"

class vtkHierarchicalDataSet;
//BTX
struct vtkXMLHierarchicalDataReaderInternals;
//ETX

class VTK_IO_EXPORT vtkXMLHierarchicalDataReader : public vtkXMLReader
{
public:
  static vtkXMLHierarchicalDataReader* New();
  vtkTypeRevisionMacro(vtkXMLHierarchicalDataReader,vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkHierarchicalDataSet* GetOutput();
  vtkHierarchicalDataSet* GetOutput(int);

protected:
  vtkXMLHierarchicalDataReader();
  ~vtkXMLHierarchicalDataReader();  

  // Get the name of the data set being read.
  virtual const char* GetDataSetName();

  virtual void ReadXMLData();
  virtual int ReadPrimaryElement(vtkXMLDataElement* ePrimary);

  // Setup the output with no data available.  Used in error cases.
  virtual void SetupEmptyOutput();

  virtual int FillOutputPortInformation(int, vtkInformation* info);

  // Create a default executive.
  virtual vtkExecutive* CreateDefaultExecutive();

  vtkXMLReader* GetReaderOfType(const char* type);

  virtual void HandleBlock(
    vtkXMLDataElement* ds, int level, int dsId, 
    vtkHierarchicalDataSet* output, vtkDataSet* data);
  
private:
  vtkXMLHierarchicalDataReader(const vtkXMLHierarchicalDataReader&);  // Not implemented.
  void operator=(const vtkXMLHierarchicalDataReader&);  // Not implemented.

  vtkXMLHierarchicalDataReaderInternals* Internal;
};

#endif
