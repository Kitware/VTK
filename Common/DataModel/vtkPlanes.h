/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlanes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPlanes
 * @brief   implicit function for convex set of planes
 *
 * vtkPlanes computes the implicit function and function gradient for a set
 * of planes. The planes must define a convex space.
 *
 * The function value is the closest first order distance of a point to the
 * convex region defined by the planes. The function gradient is the plane
 * normal at the function value.  Note that the normals must point outside of
 * the convex region. Thus, a negative function value means that a point is
 * inside the convex region.
 *
 * There are several methods to define the set of planes. The most general is
 * to supply an instance of vtkPoints and an instance of vtkDataArray. (The
 * points define a point on the plane, and the normals corresponding plane
 * normals.) Two other specialized ways are to 1) supply six planes defining
 * the view frustrum of a camera, and 2) provide a bounding box.
 *
 * @sa
 * vtkCamera
*/

#ifndef vtkPlanes_h
#define vtkPlanes_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImplicitFunction.h"

class vtkPlane;
class vtkPoints;
class vtkDataArray;

class VTKCOMMONDATAMODEL_EXPORT vtkPlanes : public vtkImplicitFunction
{
public:
  static vtkPlanes *New();
  vtkTypeMacro(vtkPlanes,vtkImplicitFunction);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Evaluate plane equations. Return smallest absolute value.
   */
  double EvaluateFunction(double x[3]) VTK_OVERRIDE;
  double EvaluateFunction(double x, double y, double z)
    {return this->vtkImplicitFunction::EvaluateFunction(x, y, z); } ;
  //@}

  /**
   * Evaluate planes gradient.
   */
  void EvaluateGradient(double x[3], double n[3]) VTK_OVERRIDE;

  //@{
  /**
   * Specify a list of points defining points through which the planes pass.
   */
  virtual void SetPoints(vtkPoints*);
  vtkGetObjectMacro(Points,vtkPoints);
  //@}

  //@{
  /**
   * Specify a list of normal vectors for the planes. There is a one-to-one
   * correspondence between plane points and plane normals.
   */
  void SetNormals(vtkDataArray* normals);
  vtkGetObjectMacro(Normals,vtkDataArray);
  //@}

  /**
   * An alternative method to specify six planes defined by the camera view
   * frustrum. See vtkCamera::GetFrustumPlanes() documentation.
   */
  void SetFrustumPlanes(double planes[24]);

  //@{
  /**
   * An alternative method to specify six planes defined by a bounding box.
   * The bounding box is a six-vector defined as (xmin,xmax,ymin,ymax,zmin,zmax).
   * It defines six planes orthogonal to the x-y-z coordinate axes.
   */
  void SetBounds(const double bounds[6]);
  void SetBounds(double xmin, double xmax, double ymin, double ymax,
                 double zmin, double zmax);
  //@}

  /**
   * Return the number of planes in the set of planes.
   */
  int GetNumberOfPlanes();

  /**
   * Create and return a pointer to a vtkPlane object at the ith
   * position. Asking for a plane outside the allowable range returns NULL.
   * This method always returns the same object.
   * Use GetPlane(int i, vtkPlane *plane) instead.
   */
  vtkPlane *GetPlane(int i);

  /**
   * If i is within the allowable range, mutates the given plane's
   * Normal and Origin to match the vtkPlane object at the ith
   * position. Does nothing if i is outside the allowable range.
   */
  void GetPlane(int i, vtkPlane *plane);

protected:
  vtkPlanes();
  ~vtkPlanes() VTK_OVERRIDE;

  vtkPoints *Points;
  vtkDataArray *Normals;
  vtkPlane *Plane;

private:
  double Planes[24];
  double Bounds[6];

private:
  vtkPlanes(const vtkPlanes&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPlanes&) VTK_DELETE_FUNCTION;
};

#endif


