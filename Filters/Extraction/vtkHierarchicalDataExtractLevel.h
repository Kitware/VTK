/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataExtractLevel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHierarchicalDataExtractLevel
 * @brief   extact levels between min and max
 *
 * Legacy class. Use vtkExtractLevel instead.
*/

#ifndef vtkHierarchicalDataExtractLevel_h
#define vtkHierarchicalDataExtractLevel_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkExtractLevel.h"

class VTKFILTERSEXTRACTION_EXPORT vtkHierarchicalDataExtractLevel : public vtkExtractLevel
{
public:
  vtkTypeMacro(vtkHierarchicalDataExtractLevel,vtkExtractLevel);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkHierarchicalDataExtractLevel *New();

protected:
  vtkHierarchicalDataExtractLevel();
  ~vtkHierarchicalDataExtractLevel();

private:
  vtkHierarchicalDataExtractLevel(const vtkHierarchicalDataExtractLevel&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHierarchicalDataExtractLevel&) VTK_DELETE_FUNCTION;
};

#endif


