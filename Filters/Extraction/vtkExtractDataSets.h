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
/**
 * @class   vtkExtractDataSets
 * @brief   extracts a number of datasets.
 *
 * vtkExtractDataSets accepts a vtkHierarchicalBoxDataSet as input and extracts
 * different datasets from different levels. The output is
 * vtkMultiBlockDataSet of vtkMultiPiece datasets. Each block corresponds to
 * a level in the vktHierarchicalBoxDataSet. Individual datasets, within a level,
 * are stored in a vtkMultiPiece dataset.
 *
 * @sa
 * vtkHierarchicalBoxDataSet, vtkMultiBlockDataSet vtkMultiPieceDataSet
*/

#ifndef vtkExtractDataSets_h
#define vtkExtractDataSets_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkMultiBlockDataSetAlgorithm.h"

class VTKFILTERSEXTRACTION_EXPORT vtkExtractDataSets :
          public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkExtractDataSets* New();
  vtkTypeMacro(vtkExtractDataSets, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Add a dataset to be extracted.
   */
  void AddDataSet(unsigned int level, unsigned int idx);

  /**
   * Remove all entries from the list of datasets to be extracted.
   */
  void ClearDataSetList();

protected:
  vtkExtractDataSets();
  ~vtkExtractDataSets() VTK_OVERRIDE;

  int RequestData(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;
  int FillOutputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

private:
  vtkExtractDataSets(const vtkExtractDataSets&) VTK_DELETE_FUNCTION;
  void operator=(const vtkExtractDataSets&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;

};

#endif


