/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPCellSizeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPCellSizeFilter
 * @brief   Computes cell sizes in parallel.
 *
 * Computes the cell sizes for all types of cells in parallel in VTK. For
 * triangles, quads, tets and hexes the static methods in vtkMeshQuality are
 * used. This is done through Verdict for higher accuracy.
 * Other cell types are individually done analytically where possible
 * and breaking into triangles or tets when not possible. When cells are
 * broken into triangles or tets the accuracy may be diminished. By default
 * all sizes are computed but point, length, area and volumetric cells
 * can each be optionally ignored. For dimensions of cells that do not
 * have their size computed, a value of 0 will be given. For cells that
 * should have their size computed but can't, the filter will return -1.
*/

#ifndef vtkPCellSizeFilter_h
#define vtkPCellSizeFilter_h

#include "vtkFiltersParallelVerdictModule.h" // For export macro
#include "vtkCellSizeFilter.h"

class VTKFILTERSPARALLELVERDICT_EXPORT vtkPCellSizeFilter : public vtkCellSizeFilter
{
public:
  vtkTypeMacro(vtkPCellSizeFilter, vtkCellSizeFilter);
  static vtkPCellSizeFilter* New();

protected:
  vtkPCellSizeFilter();
  ~vtkPCellSizeFilter() VTK_OVERRIDE;

  //@{
  /**
   * Method to compute the global sum information.
   */
  virtual void ComputeGlobalSum(vtkDoubleArray*) VTK_OVERRIDE;
  //@}

private:
  vtkPCellSizeFilter(const vtkPCellSizeFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPCellSizeFilter&) VTK_DELETE_FUNCTION;
};

#endif
