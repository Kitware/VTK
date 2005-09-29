/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLMultiGroupDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLMultiGroupDataReader - Reader for multi-group datasets
// .SECTION Description
// vtkXMLMultiGroupDataReader reads the VTK XML multi-group data file
// format. XML multi-group data files are meta-files that point to a list
// of serial VTK XML files. When reading in parallel, it will distribute
// sub-blocks among processor. If the number of sub-blocks is less than
// the number of processors, some processors will not have any sub-blocks
// for that group. If the number of sub-blocks is larger than the
// number of processors, each processor will possibly have more than
// 1 sub-block.

#ifndef __vtkXMLMultiGroupDataReader_h
#define __vtkXMLMultiGroupDataReader_h

#include "vtkXMLReader.h"

class vtkMultiGroupDataSet;
//BTX
struct vtkXMLMultiGroupDataReaderInternals;
//ETX

class VTK_IO_EXPORT vtkXMLMultiGroupDataReader : public vtkXMLReader
{
public:
  static vtkXMLMultiGroupDataReader* New();
  vtkTypeRevisionMacro(vtkXMLMultiGroupDataReader,vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkMultiGroupDataSet* GetOutput();
  vtkMultiGroupDataSet* GetOutput(int);

protected:
  vtkXMLMultiGroupDataReader();
  ~vtkXMLMultiGroupDataReader();  

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

  virtual void HandleDataSet(
    vtkXMLDataElement* ds, int group, int dsId, 
    vtkMultiGroupDataSet* output, vtkDataSet* data);

  virtual int RequestInformation(vtkInformation*, 
                                 vtkInformationVector**, 
                                 vtkInformationVector*);
  
private:
  vtkXMLMultiGroupDataReader(const vtkXMLMultiGroupDataReader&);  // Not implemented.
  void operator=(const vtkXMLMultiGroupDataReader&);  // Not implemented.

  vtkXMLMultiGroupDataReaderInternals* Internal;
};

#endif
