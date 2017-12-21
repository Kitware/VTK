/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUncertaintyTubeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkUncertaintyTubeFilter
 * @brief   generate uncertainty tubes along a polyline
 *
 * vtkUncertaintyTubeFilter is a filter that generates ellipsoidal (in cross
 * section) tubes that follows a polyline. The input is a vtkPolyData with
 * polylines that have associated vector point data. The vector data represents
 * the uncertainty of the polyline in the x-y-z directions.
 *
 * @warning
 * The vector uncertainty values define an axis-aligned ellipsoid at each
 * polyline point. The uncertainty tubes can be envisioned as the
 * interpolation of these ellipsoids between the points defining the
 * polyline (or rather, the interpolation of the cross section of the
 * ellipsoids along the polyline).
 *
 * @sa
 * vtkTensorGlyph vtkStreamTracer
*/

#ifndef vtkUncertaintyTubeFilter_h
#define vtkUncertaintyTubeFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkTubeArray;

class VTKFILTERSGENERAL_EXPORT vtkUncertaintyTubeFilter : public vtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for printing and obtaining type information for instances of this class.
   */
  vtkTypeMacro(vtkUncertaintyTubeFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  /**
   * Object factory method to instantiate this class.
   */
  static vtkUncertaintyTubeFilter *New();

  //@{
  /**
   * Set / get the number of sides for the tube. At a minimum,
   * the number of sides is 3.
   */
  vtkSetClampMacro(NumberOfSides,int,3,VTK_INT_MAX);
  vtkGetMacro(NumberOfSides,int);
  //@}

protected:
  vtkUncertaintyTubeFilter();
  ~vtkUncertaintyTubeFilter() override;

  // Integrate data
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int BuildTubes(vtkPointData *pd, vtkPointData *outPD,
                 vtkCellData *cd, vtkCellData *outCD, vtkPolyData *output);

  //array of uncertainty tubes
  vtkTubeArray *Tubes;
  int NumberOfTubes;

  // number of sides of tube
  int NumberOfSides;

private:
  vtkUncertaintyTubeFilter(const vtkUncertaintyTubeFilter&) = delete;
  void operator=(const vtkUncertaintyTubeFilter&) = delete;
};

#endif
