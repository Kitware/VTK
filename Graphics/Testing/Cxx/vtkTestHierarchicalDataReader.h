/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestHierarchicalDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTestHierarchicalDataReader - reader used in testing
// .SECTION Description
// This reader uses the xml reader and puts together one AMR dataset
// using hard-coded values.

#ifndef __vtkTestHierarchicalDataReader_h
#define __vtkTestHierarchicalDataReader_h

#include "vtkHierarchicalDataSetAlgorithm.h"

class vtkTestHierarchicalDataReader : public vtkHierarchicalDataSetAlgorithm 
{
public:
  static vtkTestHierarchicalDataReader *New();
  vtkTypeRevisionMacro(vtkTestHierarchicalDataReader,vtkHierarchicalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the file prefix.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkTestHierarchicalDataReader();
  ~vtkTestHierarchicalDataReader();

  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*);
  virtual int RequestInformation(vtkInformation*, 
                                 vtkInformationVector**, 
                                 vtkInformationVector*);
  virtual int SetUpdateBlocks(vtkInformation*, 
                              vtkInformationVector**, 
                              vtkInformationVector*);

  char* GetBlockFileName(int blockId);
  void GetBlockIdx(int blockId, int& level, int& dsindex);

  char* FileName;

  virtual int FillOutputPortInformation(int port, vtkInformation* info);

private:
  vtkTestHierarchicalDataReader(const vtkTestHierarchicalDataReader&);  // Not implemented.
  void operator=(const vtkTestHierarchicalDataReader&);  // Not implemented.
};

#endif


