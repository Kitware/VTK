// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkLassoStencilSource
 * @brief   Create a stencil from a contour
 *
 * vtkLassoStencilSource will create an image stencil from a
 * set of points that define a contour.  Its output can be
 * used with vtkImageStecil or other vtk classes that apply
 * a stencil to an image.
 * @sa
 * vtkROIStencilSource vtkPolyDataToImageStencil
 * @par Thanks:
 * Thanks to David Gobbi for contributing this class to VTK.
 */

#ifndef vtkLassoStencilSource_h
#define vtkLassoStencilSource_h

#include "vtkImageStencilSource.h"
#include "vtkImagingStencilModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkPoints;
class vtkSpline;
class vtkLSSPointMap;

class VTKIMAGINGSTENCIL_EXPORT vtkLassoStencilSource : public vtkImageStencilSource
{
public:
  static vtkLassoStencilSource* New();
  vtkTypeMacro(vtkLassoStencilSource, vtkImageStencilSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  enum
  {
    POLYGON = 0,
    SPLINE = 1
  };

  ///@{
  /**
   * The shape to use, default is "Polygon".  The spline is a
   * cardinal spline.  Bezier splines are not yet supported.
   */
  vtkGetMacro(Shape, int);
  vtkSetClampMacro(Shape, int, POLYGON, SPLINE);
  void SetShapeToPolygon() { this->SetShape(POLYGON); }
  void SetShapeToSpline() { this->SetShape(SPLINE); }
  virtual const char* GetShapeAsString();
  ///@}

  ///@{
  /**
   * The points that make up the lassoo.  The loop does not
   * have to be closed, the last point will automatically be
   * connected to the first point by a straight line segment.
   */
  virtual void SetPoints(vtkPoints* points);
  vtkGetObjectMacro(Points, vtkPoints);
  ///@}

  ///@{
  /**
   * The slice orientation.  The default is 2, which is XY.
   * Other values are 0, which is YZ, and 1, which is XZ.
   */
  vtkGetMacro(SliceOrientation, int);
  vtkSetClampMacro(SliceOrientation, int, 0, 2);
  ///@}

  ///@{
  /**
   * The points for a particular slice.  This will override the
   * points that were set by calling SetPoints() for the slice.
   * To clear the setting, call SetSlicePoints(slice, nullptr).
   */
  virtual void SetSlicePoints(int i, vtkPoints* points);
  virtual vtkPoints* GetSlicePoints(int i);
  ///@}

  /**
   * Remove points from all slices.
   */
  virtual void RemoveAllSlicePoints();

  /**
   * Overload GetMTime() to include the timestamp on the points.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkLassoStencilSource();
  ~vtkLassoStencilSource() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int Shape;
  int SliceOrientation;
  vtkPoints* Points;
  vtkSpline* SplineX;
  vtkSpline* SplineY;
  vtkLSSPointMap* PointMap;

private:
  vtkLassoStencilSource(const vtkLassoStencilSource&) = delete;
  void operator=(const vtkLassoStencilSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
