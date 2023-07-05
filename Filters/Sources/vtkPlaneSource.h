// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPlaneSource
 * @brief   create an array of quadrilaterals located in a plane
 *
 * vtkPlaneSource creates an m x n array of quadrilaterals arranged as
 * a regular tiling in a plane. The plane is defined by specifying an
 * origin point, and then two other points that, together with the
 * origin, define two axes for the plane. These axes do not have to be
 * orthogonal - so you can create a parallelogram. (The axes must not
 * be parallel.) The resolution of the plane (i.e., number of subdivisions) is
 * controlled by the ivars XResolution and YResolution.
 *
 * By default, the plane is centered at the origin and perpendicular to the
 * z-axis, with width and height of length 1 and resolutions set to 1.
 *
 * There are three convenience methods that allow you to easily move the
 * plane.  The first, SetNormal(), allows you to specify the plane
 * normal. The effect of this method is to rotate the plane around the center
 * of the plane, aligning the plane normal with the specified normal. The
 * rotation is about the axis defined by the cross product of the current
 * normal with the new normal. The second, SetCenter(), translates the center
 * of the plane to the specified center point. The third method, Push(),
 * allows you to translate the plane along the plane normal by the distance
 * specified. (Negative Push values translate the plane in the negative
 * normal direction.)  Note that the SetNormal(), SetCenter() and Push()
 * methods modify the Origin, Point1, and/or Point2 instance variables.
 *
 * @warning
 * The normal to the plane will point in the direction of the cross product
 * of the first axis (Origin->Point1) with the second (Origin->Point2). This
 * also affects the normals to the generated polygons.
 */

#ifndef vtkPlaneSource_h
#define vtkPlaneSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSSOURCES_EXPORT vtkPlaneSource : public vtkPolyDataAlgorithm
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkPlaneSource, vtkPolyDataAlgorithm);

  /**
   * Construct plane perpendicular to z-axis, resolution 1x1, width
   * and height 1.0, and centered at the origin.
   */
  static vtkPlaneSource* New();

  ///@{
  /**
   * Specify the resolution of the plane along the first axes.
   */
  vtkSetMacro(XResolution, int);
  vtkGetMacro(XResolution, int);
  ///@}

  ///@{
  /**
   * Specify the resolution of the plane along the second axes.
   */
  vtkSetMacro(YResolution, int);
  vtkGetMacro(YResolution, int);
  ///@}

  ///@{
  /**
   * Set the number of x-y subdivisions in the plane.
   */
  void SetResolution(int xR, int yR);
  void GetResolution(int& xR, int& yR)
  {
    xR = this->XResolution;
    yR = this->YResolution;
  }
  ///@}

  ///@{
  /**
   * Specify a point defining the origin of the plane.
   */
  vtkSetVector3Macro(Origin, double);
  vtkGetVectorMacro(Origin, double, 3);
  ///@}

  ///@{
  /**
   * Specify a point defining the first axis of the plane.
   */
  void SetPoint1(double x, double y, double z);
  void SetPoint1(double pnt[3]);
  vtkGetVectorMacro(Point1, double, 3);
  ///@}

  ///@{
  /**
   * Specify a point defining the second axis of the plane.
   */
  void SetPoint2(double x, double y, double z);
  void SetPoint2(double pnt[3]);
  vtkGetVectorMacro(Point2, double, 3);
  ///@}

  ///@{
  /**
   * Convenience methods to retrieve the axes of the plane; that is
   * axis a1 is the vector (Point1-Origin), and axis a2 is the vector
   * (Point2-Origin).
   */
  void GetAxis1(double a1[3]);
  void GetAxis2(double a2[3]);
  ///@}

  ///@{
  /**
   * Set/Get the center of the plane. Works in conjunction with the plane
   * normal to position the plane. Don't use this method to define the plane.
   * Instead, use it to move the plane to a new center point.
   */
  void SetCenter(double x, double y, double z);
  void SetCenter(double center[3]);
  vtkGetVectorMacro(Center, double, 3);
  ///@}

  ///@{
  /**
   * Set/Get the plane normal. Works in conjunction with the plane center to
   * orient the plane. Don't use this method to define the plane. Instead, use
   * it to rotate the plane around the current center point.
   */
  void SetNormal(double nx, double ny, double nz);
  void SetNormal(double n[3]);
  vtkGetVectorMacro(Normal, double, 3);
  ///@}

  /**
   * Translate the plane in the direction of the normal by the
   * distance specified.  Negative values move the plane in the
   * opposite direction.
   */
  void Push(double distance);

  /**
   * Rotate plane at center around a given axis
   * If the absolute value of the angle is inferior to the defined EPSILON, then don't
   * rotate
   */
  void Rotate(double angle, double rotationAxis[3]);

  ///@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision, int);
  vtkGetMacro(OutputPointsPrecision, int);
  ///@}

protected:
  vtkPlaneSource();
  ~vtkPlaneSource() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int XResolution;
  int YResolution;
  double Origin[3];
  double Point1[3];
  double Point2[3];
  double Normal[3];
  double Center[3];
  int OutputPointsPrecision;

  int UpdatePlane(double v1[3], double v2[3]);

private:
  vtkPlaneSource(const vtkPlaneSource&) = delete;
  void operator=(const vtkPlaneSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
