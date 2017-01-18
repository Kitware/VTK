/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSelectEnclosedPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkSelectEnclosedPoints
 * @brief   mark points as to whether they are inside a closed surface
 *
 * vtkSelectEnclosedPoints is a filter that evaluates all the input points to
 * determine whether they are in an enclosed surface. The filter produces a
 * (0,1) mask (in the form of a vtkDataArray) that indicates whether points
 * are outside (mask value=0) or inside (mask value=1) a provided surface.
 * (The name of the output vtkDataArray is "SelectedPointsArray".)
 *
 * After running the filter, it is possible to query it as to whether a point
 * is inside/outside by invoking the IsInside(ptId) method.
 *
 * @warning
 * The filter assumes that the surface is closed and manifold. A boolean flag
 * can be set to force the filter to first check whether this is true. If false,
 * all points will be marked outside. Note that if this check is not performed
 * and the surface is not closed, the results are undefined.
 *
 * @warning
 * This filter produces and output data array, but does not modify the input
 * dataset. If you wish to extract cells or poinrs, various threshold filters
 * are available (i.e., threshold the output array).
 *
 * @sa
 * vtkMaskPoints
*/

#ifndef vtkSelectEnclosedPoints_h
#define vtkSelectEnclosedPoints_h

#include "vtkFiltersModelingModule.h" // For export macro
#include "vtkDataSetAlgorithm.h"

class vtkUnsignedCharArray;
class vtkCellLocator;
class vtkIdList;
class vtkGenericCell;


class VTKFILTERSMODELING_EXPORT vtkSelectEnclosedPoints : public vtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkSelectEnclosedPoints,vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Instantiate this class.
   */
  static vtkSelectEnclosedPoints *New();

  //@{
  /**
   * Set the surface to be used to test for containment. Two methods are
   * provided: one directly for vtkPolyData, and one for the output of a
   * filter.
   */
  void SetSurfaceData(vtkPolyData *pd);
  void SetSurfaceConnection(vtkAlgorithmOutput* algOutput);
  //@}

  //@{
  /**
   * Return a pointer to the enclosing surface.
   */
  vtkPolyData *GetSurface();
  vtkPolyData *GetSurface(vtkInformationVector *sourceInfo);
  //@}

  //@{
  /**
   * By default, points inside the surface are marked inside or sent to
   * the output. If InsideOut is on, then the points outside the surface
   * are marked inside.
   */
  vtkSetMacro(InsideOut,int);
  vtkBooleanMacro(InsideOut,int);
  vtkGetMacro(InsideOut,int);
  //@}

  //@{
  /**
   * Specify whether to check the surface for closure. If on, then the
   * algorithm first checks to see if the surface is closed and manifold.
   */
  vtkSetMacro(CheckSurface,int);
  vtkBooleanMacro(CheckSurface,int);
  vtkGetMacro(CheckSurface,int);
  //@}

  /**
   * Query an input point id as to whether it is inside or outside. Note that
   * the result requires that the filter execute first.
   */
  int IsInside(vtkIdType inputPtId);

  //@{
  /**
   * Specify the tolerance on the intersection. The tolerance is expressed
   * as a fraction of the bounding box of the enclosing surface.
   */
  vtkSetClampMacro(Tolerance,double,0.0,VTK_FLOAT_MAX);
  vtkGetMacro(Tolerance,double);
  //@}

  //@{
  /**
   * This is a backdoor that can be used to test many points for containment.
   * First initialize the instance, then repeated calls to IsInsideSurface()
   * can be used without rebuilding the search structures. The complete
   * method releases memory.
   */
  void Initialize(vtkPolyData *surface);
  int IsInsideSurface(double x, double y, double z);
  int IsInsideSurface(double x[3]);
  void Complete();
  //@}

protected:
  vtkSelectEnclosedPoints();
  ~vtkSelectEnclosedPoints() VTK_OVERRIDE;

  int    CheckSurface;
  int    InsideOut;
  double Tolerance;

  int IsSurfaceClosed(vtkPolyData *surface);
  vtkUnsignedCharArray *InsideOutsideArray;

  // Internal structures for accelerating the intersection test
  vtkCellLocator *CellLocator;
  vtkIdList      *CellIds;
  vtkGenericCell *Cell;
  vtkPolyData    *Surface;
  double          Bounds[6];
  double          Length;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int, vtkInformation *) VTK_OVERRIDE;

  void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;

private:
  vtkSelectEnclosedPoints(const vtkSelectEnclosedPoints&) VTK_DELETE_FUNCTION;
  void operator=(const vtkSelectEnclosedPoints&) VTK_DELETE_FUNCTION;
};

#endif
