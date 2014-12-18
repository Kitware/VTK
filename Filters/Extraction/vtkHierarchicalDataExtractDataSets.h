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
// .NAME vtkHierarchicalDataExtractDataSets - extract a number of datasets
// .SECTION Description
// Legacy class. Use vtkExtractDataSets instead.
//
// .SECTION See Also
// vtkExtractDataSets

#ifndef vtkHierarchicalDataExtractDataSets_h
#define vtkHierarchicalDataExtractDataSets_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkExtractDataSets.h"

//BTX
struct vtkHierarchicalDataExtractDataSetsInternals;
//ETX

class VTKFILTERSEXTRACTION_EXPORT vtkHierarchicalDataExtractDataSets :
  public vtkExtractDataSets
{
public:
  vtkTypeMacro(vtkHierarchicalDataExtractDataSets,vtkExtractDataSets);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkHierarchicalDataExtractDataSets *New();

protected:
  vtkHierarchicalDataExtractDataSets();
  ~vtkHierarchicalDataExtractDataSets();

private:
  vtkHierarchicalDataExtractDataSets(const vtkHierarchicalDataExtractDataSets&);  // Not implemented.
  void operator=(const vtkHierarchicalDataExtractDataSets&);  // Not implemented.
};

#endif


