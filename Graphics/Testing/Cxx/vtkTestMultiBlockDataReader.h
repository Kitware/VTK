/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTestMultiBlockDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTestMultiBlockDataReader - reader used in testing
// .SECTION Description
// This reader uses the xml reader and puts together one AMR dataset
// using hard-coded values.

#ifndef __vtkTestMultiBlockDataReader_h
#define __vtkTestMultiBlockDataReader_h

#include "vtkHierarchicalDataSetAlgorithm.h"

class vtkTestMultiBlockDataReader : public vtkHierarchicalDataSetAlgorithm 
{
public:
  static vtkTestMultiBlockDataReader *New();
  vtkTypeRevisionMacro(vtkTestMultiBlockDataReader,vtkHierarchicalDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the file prefix.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkTestMultiBlockDataReader();
  ~vtkTestMultiBlockDataReader();

  virtual int RequestData(vtkInformation*, 
                          vtkInformationVector**, 
                          vtkInformationVector*);
  virtual int RequestInformation(vtkInformation*, 
                                 vtkInformationVector**, 
                                 vtkInformationVector*);
  virtual int SetUpdateBlocks(vtkInformation*, 
                              vtkInformationVector**, 
                              vtkInformationVector*);

  char* FileName;

private:
  vtkTestMultiBlockDataReader(const vtkTestMultiBlockDataReader&);  // Not implemented.
  void operator=(const vtkTestMultiBlockDataReader&);  // Not implemented.
};

#endif


