/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImplicitSelectionLoop.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkImplicitSelectionLoop
 * @brief   implicit function for a selection loop
 *
 * vtkImplicitSelectionLoop computes the implicit function value and
 * function gradient for a irregular, cylinder-like object whose cross
 * section is defined by a set of points forming a loop. The loop need
 * not be convex nor its points coplanar. However, the loop must be
 * non-self-intersecting when projected onto the plane defined by the
 * accumulated cross product around the loop (i.e., the axis of the
 * loop). (Alternatively, you can specify the normal to use.)
 *
 * The following procedure is used to compute the implicit function
 * value for a point x. Each point of the loop is first projected onto
 * the plane defined by the loop normal. This forms a polygon. Then,
 * to evaluate the implicit function value, inside/outside tests are
 * used to determine if x is inside the polygon, and the distance to
 * the loop boundary is computed (negative values are inside the
 * loop).
 *
 * One example application of this implicit function class is to draw a
 * loop on the surface of a mesh, and use the loop to clip or extract
 * cells from within the loop. Remember, the selection loop is "infinite"
 * in length, you can use a plane (in boolean combination) to cap the extent
 * of the selection loop. Another trick is to use a connectivity filter to
 * extract the closest region to a given point (i.e., one of the points used
 * to define the selection loop).
 *
 * @sa
 * vtkImplicitFunction vtkImplicitBoolean vtkExtractGeometry vtkClipPolyData
 * vtkConnectivityFilter vtkPolyDataConnectivityFilter
*/

#ifndef vtkImplicitSelectionLoop_h
#define vtkImplicitSelectionLoop_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class vtkPoints;
class vtkPolygon;

class VTKCOMMONDATAMODEL_EXPORT vtkImplicitSelectionLoop : public vtkImplicitFunction
{
public:
  //@{
  /**
   * Standard VTK methods for printing and type information.
   */
  vtkTypeMacro(vtkImplicitSelectionLoop,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  /**
   * Instantiate object with no initial loop.
   */
  static vtkImplicitSelectionLoop *New();

  //@{
  /**
   * Evaluate selection loop returning a signed distance.
   */
  using vtkImplicitFunction::EvaluateFunction;
  double EvaluateFunction(double x[3]) VTK_OVERRIDE;
  //@}

  /**
   * Evaluate selection loop returning the gradient.
   */
  void EvaluateGradient(double x[3], double n[3]) VTK_OVERRIDE;

  //@{
  /**
   * Set/Get the array of point coordinates defining the loop. There must
   * be at least three points used to define a loop.
   */
  virtual void SetLoop(vtkPoints*);
  vtkGetObjectMacro(Loop,vtkPoints);
  //@}

  //@{
  /**
   * Turn on/off automatic normal generation. By default, the normal is
   * computed from the accumulated cross product of the edges. You can also
   * specify the normal to use.
   */
  vtkSetMacro(AutomaticNormalGeneration,int);
  vtkGetMacro(AutomaticNormalGeneration,int);
  vtkBooleanMacro(AutomaticNormalGeneration,int);
  //@}

  //@{
  /**
   * Set / get the normal used to determine whether a point is inside or outside
   * the selection loop.
   */
  vtkSetVector3Macro(Normal,double);
  vtkGetVectorMacro(Normal,double,3);
  //@}

  /**
   * Overload GetMTime() because we depend on the Loop
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

protected:
  vtkImplicitSelectionLoop();
  ~vtkImplicitSelectionLoop() VTK_OVERRIDE;

  vtkPoints *Loop;
  double Normal[3];
  int AutomaticNormalGeneration;

private:
  void Initialize();
  vtkPolygon *Polygon;

  double Origin[3];
  double Bounds[6]; //bounds of the projected polyon
  double DeltaX;
  double DeltaY;
  double DeltaZ;

  vtkTimeStamp InitializationTime;

private:
  vtkImplicitSelectionLoop(const vtkImplicitSelectionLoop&) VTK_DELETE_FUNCTION;
  void operator=(const vtkImplicitSelectionLoop&) VTK_DELETE_FUNCTION;
};

#endif


