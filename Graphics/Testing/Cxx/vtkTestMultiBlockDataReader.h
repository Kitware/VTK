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

#include "vtkMultiBlockDataSetAlgorithm.h"

class vtkTestMultiBlockDataReader : public vtkMultiBlockDataSetAlgorithm 
{
public:
  static vtkTestMultiBlockDataReader *New();
  vtkTypeRevisionMacro(vtkTestMultiBlockDataReader,vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the file prefix.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkTestMultiBlockDataReader();
  ~vtkTestMultiBlockDataReader();

  virtual int RequestCompositeData(vtkInformation*, 
                                   vtkInformationVector**, 
                                   vtkInformationVector*);
  virtual int RequestCompositeInformation(vtkInformation*, 
                                          vtkInformationVector**, 
                                          vtkInformationVector*);

  char* FileName;

private:
  vtkTestMultiBlockDataReader(const vtkTestMultiBlockDataReader&);  // Not implemented.
  void operator=(const vtkTestMultiBlockDataReader&);  // Not implemented.
};

#endif


