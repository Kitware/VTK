// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEllipticalButtonSource
 * @brief   create a ellipsoidal-shaped button
 *
 * vtkEllipticalButtonSource creates a ellipsoidal shaped button with
 * texture coordinates suitable for application of a texture map. This
 * provides a way to make nice looking 3D buttons. The buttons are
 * represented as vtkPolyData that includes texture coordinates and
 * normals. The button lies in the x-y plane.
 *
 * To use this class you must define the major and minor axes lengths of an
 * ellipsoid (expressed as width (x), height (y) and depth (z)). The button
 * has a rectangular mesh region in the center with texture coordinates that
 * range smoothly from (0,1). (This flat region is called the texture
 * region.) The outer, curved portion of the button (called the shoulder) has
 * texture coordinates set to a user specified value (by default (0,0).
 * (This results in coloring the button curve the same color as the (s,t)
 * location of the texture map.) The resolution in the radial direction, the
 * texture region, and the shoulder region must also be set. The button can
 * be moved by specifying an origin.
 *
 * @sa
 * vtkButtonSource vtkRectangularButtonSource
 */

#ifndef vtkEllipticalButtonSource_h
#define vtkEllipticalButtonSource_h

#include "vtkButtonSource.h"
#include "vtkFiltersSourcesModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkFloatArray;
class vtkPoints;

class VTKFILTERSSOURCES_EXPORT vtkEllipticalButtonSource : public vtkButtonSource
{
public:
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkEllipticalButtonSource, vtkButtonSource);

  /**
   * Construct a circular button with depth 10% of its height.
   */
  static vtkEllipticalButtonSource* New();

  ///@{
  /**
   * Set/Get the width of the button (the x-ellipsoid axis length * 2).
   */
  vtkSetClampMacro(Width, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Width, double);
  ///@}

  ///@{
  /**
   * Set/Get the height of the button (the y-ellipsoid axis length * 2).
   */
  vtkSetClampMacro(Height, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Height, double);
  ///@}

  ///@{
  /**
   * Set/Get the depth of the button (the z-eliipsoid axis length).
   */
  vtkSetClampMacro(Depth, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(Depth, double);
  ///@}

  ///@{
  /**
   * Specify the resolution of the button in the circumferential direction.
   */
  vtkSetClampMacro(CircumferentialResolution, int, 4, VTK_INT_MAX);
  vtkGetMacro(CircumferentialResolution, int);
  ///@}

  ///@{
  /**
   * Specify the resolution of the texture in the radial direction in the
   * texture region.
   */
  vtkSetClampMacro(TextureResolution, int, 1, VTK_INT_MAX);
  vtkGetMacro(TextureResolution, int);
  ///@}

  ///@{
  /**
   * Specify the resolution of the texture in the radial direction in the
   * shoulder region.
   */
  vtkSetClampMacro(ShoulderResolution, int, 1, VTK_INT_MAX);
  vtkGetMacro(ShoulderResolution, int);
  ///@}

  ///@{
  /**
   * Set/Get the radial ratio. This is the measure of the radius of the
   * outer ellipsoid to the inner ellipsoid of the button. The outer
   * ellipsoid is the boundary of the button defined by the height and
   * width. The inner ellipsoid circumscribes the texture region. Larger
   * RadialRatio's cause the button to be more rounded (and the texture
   * region to be smaller); smaller ratios produce sharply curved shoulders
   * with a larger texture region.
   */
  vtkSetClampMacro(RadialRatio, double, 1.0, VTK_DOUBLE_MAX);
  vtkGetMacro(RadialRatio, double);
  ///@}

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
  vtkEllipticalButtonSource();
  ~vtkEllipticalButtonSource() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  double Width;
  double Height;
  double Depth;
  int CircumferentialResolution;
  int TextureResolution;
  int ShoulderResolution;
  int OutputPointsPrecision;
  double RadialRatio;

private:
  // internal variable related to axes of ellipsoid
  double A;
  double A2;
  double B;
  double B2;
  double C;
  double C2;

  double ComputeDepth(int inTextureRegion, double x, double y, double n[3]);
  void InterpolateCurve(int inTextureRegion, vtkPoints* newPts, int numPts, vtkFloatArray* normals,
    vtkFloatArray* tcoords, int res, int c1StartPoint, int c1Incr, int c2StartPoint, int s2Incr,
    int startPoint, int incr);
  void CreatePolygons(vtkCellArray* newPolys, int num, int res, int startIdx);
  void IntersectEllipseWithLine(double a2, double b2, double dX, double dY, double& xe, double& ye);

  vtkEllipticalButtonSource(const vtkEllipticalButtonSource&) = delete;
  void operator=(const vtkEllipticalButtonSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
