/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataExtractDataSets.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHierarchicalDataExtractDataSets
 * @brief   extract a number of datasets
 *
 * Legacy class. Use vtkExtractDataSets instead.
 *
 * @sa
 * vtkExtractDataSets
*/

#ifndef vtkHierarchicalDataExtractDataSets_h
#define vtkHierarchicalDataExtractDataSets_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkExtractDataSets.h"

struct vtkHierarchicalDataExtractDataSetsInternals;

class VTKFILTERSEXTRACTION_EXPORT vtkHierarchicalDataExtractDataSets :
  public vtkExtractDataSets
{
public:
  vtkTypeMacro(vtkHierarchicalDataExtractDataSets,vtkExtractDataSets);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkHierarchicalDataExtractDataSets *New();

protected:
  vtkHierarchicalDataExtractDataSets();
  ~vtkHierarchicalDataExtractDataSets() override;

private:
  vtkHierarchicalDataExtractDataSets(const vtkHierarchicalDataExtractDataSets&) = delete;
  void operator=(const vtkHierarchicalDataExtractDataSets&) = delete;
};

#endif


