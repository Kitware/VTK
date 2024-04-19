// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkGraphAnnotationLayersFilter
 * @brief   Produce filled convex hulls around
 * subsets of vertices in a vtkGraph.
 *
 *
 * Produces a vtkPolyData comprised of filled polygons of the convex hull
 * of a cluster. Alternatively, you may choose to output bounding rectangles.
 * Clusters with fewer than three vertices are artificially expanded to
 * ensure visibility (see vtkConvexHull2D).
 *
 * The first input is a vtkGraph with points, possibly set by
 * passing the graph through vtkGraphLayout (z-values are ignored). The second
 * input is a vtkAnnotationsLayer containing vtkSelectionNodeS of vertex
 * ids (the 'clusters' output of vtkTulipReader for example).
 *
 * Setting OutlineOn() additionally produces outlines of the clusters on
 * output port 1.
 *
 * Three arrays are added to the cells of the output: "Hull id"; "Hull name";
 * and "Hull color".
 *
 * Note: This filter operates in the x,y-plane and as such works best with an
 * interactor style that does not allow camera rotation, such as
 * vtkInteractorStyleRubberBand2D.
 *
 * @sa
 * vtkContext2D
 *
 * @par Thanks:
 * Thanks to Colin Myers, University of Leeds for providing this implementation.
 */

#ifndef vtkGraphAnnotationLayersFilter_h
#define vtkGraphAnnotationLayersFilter_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkRenderingAnnotationModule.h" // For export macro
#include "vtkSmartPointer.h"              // needed for ivars
#include "vtkWrappingHints.h"             // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkAppendPolyData;
class vtkConvexHull2D;
class vtkRenderer;

class VTKRENDERINGANNOTATION_EXPORT VTK_MARSHALAUTO vtkGraphAnnotationLayersFilter
  : public vtkPolyDataAlgorithm
{
public:
  static vtkGraphAnnotationLayersFilter* New();
  vtkTypeMacro(vtkGraphAnnotationLayersFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Produce outlines of the hulls on output port 1.
   */
  void OutlineOn();
  void OutlineOff();
  void SetOutline(bool b);
  ///@}

  /**
   * Scale each hull by the amount specified. Defaults to 1.0.
   */
  void SetScaleFactor(double scale);

  /**
   * Set the shape of the hulls to bounding rectangle.
   */
  void SetHullShapeToBoundingRectangle();

  /**
   * Set the shape of the hulls to convex hull. Default.
   */
  void SetHullShapeToConvexHull();

  /**
   * Set the minimum x,y-dimensions of each hull in world coordinates. Defaults
   * to 1.0. Set to 0.0 to disable.
   */
  void SetMinHullSizeInWorld(double size);

  /**
   * Set the minimum x,y-dimensions of each hull in pixels. You must also set a
   * vtkRenderer. Defaults to 1. Set to 0 to disable.
   */
  void SetMinHullSizeInDisplay(int size);

  /**
   * Renderer needed for MinHullSizeInDisplay calculation. Not reference counted.
   */
  void SetRenderer(vtkRenderer* renderer);

  /**
   * The modified time of this filter.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkGraphAnnotationLayersFilter();
  ~vtkGraphAnnotationLayersFilter() override;

  /**
   * This is called by the superclass. This is the method you should override.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Set the input to vtkGraph and vtkAnnotationLayers.
   */
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkGraphAnnotationLayersFilter(const vtkGraphAnnotationLayersFilter&) = delete;
  void operator=(const vtkGraphAnnotationLayersFilter&) = delete;

  vtkSmartPointer<vtkAppendPolyData> HullAppend;
  vtkSmartPointer<vtkAppendPolyData> OutlineAppend;
  vtkSmartPointer<vtkConvexHull2D> ConvexHullFilter;
};

VTK_ABI_NAMESPACE_END
#endif // vtkGraphAnnotationLayersFilter_h
