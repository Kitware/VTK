/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractDataSets.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractDataSets - extracts a number of datasets.
// .SECTION Description
// vtkExtractDataSets accepts a vtkHierarchicalBoxDataSet as input and extracts
// different datasets from different levels. The output is
// vtkHierarchicalBoxDataSet with same structure as the input with only the
// selected datasets passed through. 

#ifndef __vtkExtractDataSets_h
#define __vtkExtractDataSets_h

#include "vtkHierarchicalBoxDataSetAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkExtractDataSets : public vtkHierarchicalBoxDataSetAlgorithm
{
public:
  static vtkExtractDataSets* New();
  vtkTypeMacro(vtkExtractDataSets, vtkHierarchicalBoxDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Add a dataset to be extracted.
  void AddDataSet(unsigned int level, unsigned int idx);

  // Description:
  // Remove all entries from the list of datasets to be extracted.
  void ClearDataSetList();

//BTX
protected:
  vtkExtractDataSets();
  ~vtkExtractDataSets();

  virtual int RequestData(vtkInformation *, 
                          vtkInformationVector **, 
                          vtkInformationVector *);

private:
  vtkExtractDataSets(const vtkExtractDataSets&); // Not implemented.
  void operator=(const vtkExtractDataSets&); // Not implemented.

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif


