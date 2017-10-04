/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalDataLevelFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHierarchicalDataLevelFilter
 * @brief   generate scalars from levels
 *
 * Legacy class. Use vtkLevelIdScalars instead.
 *
 * @sa
 * vtkLevelIdScalars
*/

#ifndef vtkHierarchicalDataLevelFilter_h
#define vtkHierarchicalDataLevelFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkLevelIdScalars.h"

class VTKFILTERSGENERAL_EXPORT vtkHierarchicalDataLevelFilter : public vtkLevelIdScalars
{
public:
  vtkTypeMacro(vtkHierarchicalDataLevelFilter,vtkLevelIdScalars);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static vtkHierarchicalDataLevelFilter *New();

protected:
  vtkHierarchicalDataLevelFilter();
  ~vtkHierarchicalDataLevelFilter() override;

private:
  vtkHierarchicalDataLevelFilter(const vtkHierarchicalDataLevelFilter&) = delete;
  void operator=(const vtkHierarchicalDataLevelFilter&) = delete;
};

#endif


