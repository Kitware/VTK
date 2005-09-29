/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLMultiBlockDataReader.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLMultiBlockDataReader - Reader for multi-block datasets
// .SECTION Description
// vtkXMLMultiBlockDataReader reads the VTK XML multi-block data file
// format. XML multi-block data files are meta-files that point to a list
// of serial VTK XML files. When reading in parallel, it will distribute
// sub-blocks among processor. If the number of sub-blocks is less than
// the number of processors, some processors will not have any sub-blocks
// for that block. If the number of sub-blocks is larger than the
// number of processors, each processor will possibly have more than
// 1 sub-block.

#ifndef __vtkXMLMultiBlockDataReader_h
#define __vtkXMLMultiBlockDataReader_h

#include "vtkXMLMultiGroupDataReader.h"

class vtkMultiBlockDataSet;

class VTK_IO_EXPORT vtkXMLMultiBlockDataReader : public vtkXMLMultiGroupDataReader
{
public:
  static vtkXMLMultiBlockDataReader* New();
  vtkTypeRevisionMacro(vtkXMLMultiBlockDataReader,vtkXMLMultiGroupDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the output data object for a port on this algorithm.
  vtkMultiBlockDataSet* GetOutput();
  vtkMultiBlockDataSet* GetOutput(int);

protected:
  vtkXMLMultiBlockDataReader();
  ~vtkXMLMultiBlockDataReader();  

  // Get the name of the data set being read.
  virtual const char* GetDataSetName();

  virtual int FillOutputPortInformation(int, vtkInformation* info);

private:
  vtkXMLMultiBlockDataReader(const vtkXMLMultiBlockDataReader&);  // Not implemented.
  void operator=(const vtkXMLMultiBlockDataReader&);  // Not implemented.
};

#endif
