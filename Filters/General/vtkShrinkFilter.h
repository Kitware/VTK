/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShrinkFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkShrinkFilter
 * @brief   shrink cells composing an arbitrary data set
 *
 * vtkShrinkFilter shrinks cells composing an arbitrary data set
 * towards their centroid. The centroid of a cell is computed as the
 * average position of the cell points. Shrinking results in
 * disconnecting the cells from one another. The output of this filter
 * is of general dataset type vtkUnstructuredGrid.
 *
 * @warning
 * It is possible to turn cells inside out or cause self intersection
 * in special cases.
 *
 * @sa
 * vtkShrinkPolyData
*/

#ifndef vtkShrinkFilter_h
#define vtkShrinkFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkShrinkFilter : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkShrinkFilter *New();
  vtkTypeMacro(vtkShrinkFilter,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the fraction of shrink for each cell. The default is 0.5.
   */
  vtkSetClampMacro(ShrinkFactor, double, 0.0, 1.0);
  vtkGetMacro(ShrinkFactor, double);
  //@}

protected:
  vtkShrinkFilter();
  ~vtkShrinkFilter() VTK_OVERRIDE;

  // Override to specify support for any vtkDataSet input type.
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  // Main implementation.
  int RequestData(vtkInformation*,
                  vtkInformationVector**,
                  vtkInformationVector*) VTK_OVERRIDE;

  double ShrinkFactor;

private:
  vtkShrinkFilter(const vtkShrinkFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkShrinkFilter&) VTK_DELETE_FUNCTION;
};

#endif
