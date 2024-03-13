// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkConvexHull2D
 * @brief   Produce filled convex hulls around a set of points.
 *
 *
 * Produces a vtkPolyData comprised of a filled polygon of the convex hull
 * of the input points. You may alternatively choose to output a bounding
 * rectangle. Static methods are provided that calculate a (counter-clockwise)
 * hull based on a set of input points.
 *
 * To help maintain the property of <i>guaranteed visibility</i> hulls may be
 * artificially scaled by setting MinHullSizeInWorld. This is particularly
 * helpful in the case that there are only one or two points as it avoids
 * producing a degenerate polygon. This setting is also available as an
 * argument to the static methods.
 *
 * Setting a vtkRenderer on the filter enables the possibility to set
 * MinHullSizeInDisplay to the desired number of display pixels to cover in
 * each of the x- and y-dimensions.
 *
 * Setting OutlineOn() additionally produces an outline of the hull on output
 * port 1.
 *
 * @attention
 * This filter operates in the x,y-plane and as such works best with an
 * interactor style that does not permit camera rotation such as
 * vtkInteractorStyleRubberBand2D.
 *
 * @par Thanks:
 * Thanks to Colin Myers, University of Leeds for providing this implementation.
 */

#ifndef vtkConvexHull2D_h
#define vtkConvexHull2D_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkSmartPointer.h"              // needed for ivars
#include "vtkWrappingHints.h"             // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkCoordinate;
class vtkPoints;
class vtkPolygon;
class vtkPolyLine;
class vtkRenderer;
class vtkTransform;
class vtkTransformPolyDataFilter;

class VTKRENDERINGANNOTATION_EXPORT VTK_MARSHALAUTO vtkConvexHull2D : public vtkPolyDataAlgorithm
{
public:
  static vtkConvexHull2D* New();
  vtkTypeMacro(vtkConvexHull2D, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Scale the hull by the amount specified. Defaults to 1.0.
   */
  vtkGetMacro(ScaleFactor, double);
  vtkSetMacro(ScaleFactor, double);
  ///@}

  ///@{
  /**
   * Produce an outline (polyline) of the hull on output port 1.
   */
  vtkGetMacro(Outline, bool);
  vtkSetMacro(Outline, bool);
  vtkBooleanMacro(Outline, bool);
  ///@}

  enum HullShapes
  {
    BoundingRectangle = 0,
    ConvexHull
  };

  ///@{
  /**
   * Set the shape of the hull to BoundingRectangle or ConvexHull.
   */
  vtkGetMacro(HullShape, int);
  vtkSetClampMacro(HullShape, int, 0, 1);
  ///@}

  ///@{
  /**
   * Set the minimum x,y-dimensions of each hull in world coordinates. Defaults
   * to 1.0. Set to 0.0 to disable.
   */
  vtkSetClampMacro(MinHullSizeInWorld, double, 0.0, VTK_DOUBLE_MAX);
  vtkGetMacro(MinHullSizeInWorld, double);
  ///@}

  ///@{
  /**
   * Set the minimum x,y-dimensions of each hull in pixels. You must also set a
   * vtkRenderer. Defaults to 1. Set to 0 to disable.
   */
  vtkSetClampMacro(MinHullSizeInDisplay, int, 0, VTK_INT_MAX);
  vtkGetMacro(MinHullSizeInDisplay, int);
  ///@}

  ///@{
  /**
   * Renderer needed for MinHullSizeInDisplay calculation. Not reference counted.
   */
  void SetRenderer(vtkRenderer* renderer);
  vtkRenderer* GetRenderer();
  ///@}

  /**
   * The modified time of this filter.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Convenience methods to calculate a convex hull from a set of vtkPointS.
   */
  static void CalculateBoundingRectangle(
    vtkPoints* inPoints, vtkPoints* outPoints, double minimumHullSize = 1.0);
  static void CalculateConvexHull(
    vtkPoints* inPoints, vtkPoints* outPoints, double minimumHullSize = 1.0);
  ///@}

protected:
  vtkConvexHull2D();
  ~vtkConvexHull2D() override;

  /**
   * This is called by the superclass. This is the method you should override.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkConvexHull2D(const vtkConvexHull2D&) = delete;
  void operator=(const vtkConvexHull2D&) = delete;

  void ResizeHullToMinimumInDisplay(vtkPolyData* hullPolyData);

  double ScaleFactor;
  bool Outline;
  int HullShape;
  int MinHullSizeInDisplay;
  double MinHullSizeInWorld;
  vtkRenderer* Renderer;

  vtkSmartPointer<vtkCoordinate> Coordinate;
  vtkSmartPointer<vtkTransform> Transform;
  vtkSmartPointer<vtkTransform> OutputTransform;
  vtkSmartPointer<vtkTransformPolyDataFilter> OutputTransformFilter;
  vtkSmartPointer<vtkPolyLine> OutlineSource;
  vtkSmartPointer<vtkPolygon> HullSource;
};

VTK_ABI_NAMESPACE_END
#endif // vtkConvexHull2D_h
