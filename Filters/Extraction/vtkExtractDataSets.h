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
// vtkMultiBlockDataSet of vtkMultiPiece datasets. Each block corresponds to
// a level in the vktHierarchicalBoxDataSet. Individual datasets, within a level,
// are stored in a vtkMultiPiece dataset.
//
// .SECTION See Also
// vtkHierarchicalBoxDataSet, vtkMultiBlockDataSet vtkMultiPieceDataSet

#ifndef __vtkExtractDataSets_h
#define __vtkExtractDataSets_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class VTKFILTERSEXTRACTION_EXPORT vtkExtractDataSets :
          public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkExtractDataSets* New();
  vtkTypeMacro(vtkExtractDataSets, vtkMultiBlockDataSetAlgorithm);
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
  virtual int FillInputPortInformation(int port, vtkInformation *info);
  virtual int FillOutputPortInformation(int port, vtkInformation *info);

private:
  vtkExtractDataSets(const vtkExtractDataSets&); // Not implemented.
  void operator=(const vtkExtractDataSets&); // Not implemented.

  class vtkInternals;
  vtkInternals* Internals;
//ETX
};

#endif


