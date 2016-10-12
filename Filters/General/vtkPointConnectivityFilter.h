/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPointConnectivityFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPointConnectivityFilter
 * @brief   output a scalar field indicating point connectivity
 *
 *
 * vtkPointConnectivityFilter is a filter the produces a point scalar field
 * that characerizes the connectivity of the point. What is meant by
 * connectivity is the number of cells that use each point. The output
 * scalar array is represented by a 16-bit integral value. A value of zero
 * means that no cells use a particular point.
*/

#ifndef vtkPointConnectivityFilter_h
#define vtkPointConnectivityFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkPointConnectivityFilter : public vtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information and
   * printing.
   */
  static vtkPointConnectivityFilter *New();
  vtkTypeMacro(vtkPointConnectivityFilter,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

protected:
  vtkPointConnectivityFilter();
  ~vtkPointConnectivityFilter() VTK_OVERRIDE;

  int RequestData(vtkInformation *,
                  vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

private:
  vtkPointConnectivityFilter(const vtkPointConnectivityFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPointConnectivityFilter&) VTK_DELETE_FUNCTION;
};

#endif
