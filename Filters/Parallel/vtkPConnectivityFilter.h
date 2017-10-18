/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPConnectivityFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPConnectivityFilter
 * @brief   Parallel version of vtkConnectivityFilter
 *
 * This class is a subclass of vtkConnectivityFilter. It implements the additional
 * communication needed to properly compute connectivity of a distributed data set.
 * This parallel implementation does not support a number of features that the
 * vtkConnectivityFilter class supports, including:
 *
 *   - ScalarConnectivity
 *   - VTK_EXTRACT_POINT_SEEDED_REGIONS extraction mode
 *   - VTK_EXTRACT_CELL_SEEDED_REGIONS extraction mode
 *   - VTK_EXTRACT_SPECIFIED_REGIONS extraction mode
 */

#ifndef vtkPConnectivityFilter_h
#define vtkPConnectivityFilter_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkConnectivityFilter.h"

class VTKFILTERSPARALLEL_EXPORT vtkPConnectivityFilter : public vtkConnectivityFilter
{
public:
  vtkTypeMacro(vtkPConnectivityFilter,vtkConnectivityFilter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkPConnectivityFilter *New();

protected:
  vtkPConnectivityFilter();
  ~vtkPConnectivityFilter() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

private:
  vtkPConnectivityFilter(const vtkPConnectivityFilter&) = delete;
  void operator=(const vtkPConnectivityFilter&) = delete;
};

#endif
