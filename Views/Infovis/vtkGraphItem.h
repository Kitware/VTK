// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGraphItem
 * @brief   A 2D graphics item for rendering a graph.
 *
 *
 * This item draws a graph as a part of a vtkContextScene. This simple
 * class has minimal state and delegates the determination of visual
 * vertex and edge properties like color, size, width, etc. to
 * a set of virtual functions. To influence the rendering of the graph,
 * subclass this item and override the property functions you wish to
 * customize.
 */

#ifndef vtkGraphItem_h
#define vtkGraphItem_h

#include "vtkContextItem.h"
#include "vtkViewsInfovisModule.h" // For export macro

#include "vtkColor.h"  // For color types in API
#include "vtkNew.h"    // For vtkNew ivars
#include "vtkVector.h" // For vector types in API

VTK_ABI_NAMESPACE_BEGIN
class vtkGraph;
class vtkImageData;
class vtkIncrementalForceLayout;
class vtkRenderWindowInteractor;
class vtkTooltipItem;

class VTKVIEWSINFOVIS_EXPORT vtkGraphItem : public vtkContextItem
{
public:
  static vtkGraphItem* New();
  vtkTypeMacro(vtkGraphItem, vtkContextItem);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The graph that this item draws.
   */
  virtual void SetGraph(vtkGraph* graph);
  vtkGetObjectMacro(Graph, vtkGraph);
  ///@}

  /**
   * Exposes the incremental graph layout for updating parameters.
   */
  virtual vtkIncrementalForceLayout* GetLayout();

  ///@{
  /**
   * Begins or ends the layout animation.
   */
  virtual void StartLayoutAnimation(vtkRenderWindowInteractor* interactor);
  virtual void StopLayoutAnimation();
  ///@}

  /**
   * Incrementally updates the graph layout.
   */
  virtual void UpdateLayout();

protected:
  vtkGraphItem();
  ~vtkGraphItem() override;

  /**
   * Paints the graph. This method will call RebuildBuffers()
   * if the graph is dirty, then call PaintBuffers().
   */
  bool Paint(vtkContext2D* painter) override;

  /**
   * Builds a cache of data from the graph by calling the virtual functions
   * such as VertexColor(), EdgeColor(), etc. This will only get called when
   * the item is dirty (i.e. IsDirty() returns true).
   */
  virtual void RebuildBuffers();

  /**
   * Efficiently draws the contents of the buffers built in RebuildBuffers.
   * This occurs once per frame.
   */
  virtual void PaintBuffers(vtkContext2D* painter);

  /**
   * Returns true if the underlying vtkGraph has been modified since the last
   * RebuildBuffers, signaling a new RebuildBuffers is needed. When the graph
   * was modified, it assumes the buffers will be rebuilt, so it updates
   * the modified time of the last build. Override this function if you have
   * a subclass that uses any information in addition to the vtkGraph to determine
   * visual properties that may be dynamic.
   */
  virtual bool IsDirty();

  /**
   * Returns the number of vertices in the graph. Generally you do not need
   * to override this method as it simply queries the underlying vtkGraph.
   */
  virtual vtkIdType NumberOfVertices();

  /**
   * Returns the number of edges in the graph. Generally you do not need
   * to override this method as it simply queries the underlying vtkGraph.
   */
  virtual vtkIdType NumberOfEdges();

  /**
   * Returns the number of edge control points for a particular edge. The
   * implementation returns GetNumberOfEdgePoints(edge) + 2 for the specified edge
   * to incorporate the source and target vertex positions as initial
   * and final edge points.
   */
  virtual vtkIdType NumberOfEdgePoints(vtkIdType edge);

  /**
   * Returns the edge width. Override in a subclass to change the edge width.
   * Note: The item currently supports one width per edge, queried on the first point.
   */
  virtual float EdgeWidth(vtkIdType edge, vtkIdType point);

  /**
   * Returns the edge color. Override in a subclass to change the edge color.
   * Each edge control point may be rendered with a separate color with interpolation
   * on line segments between points.
   */
  virtual vtkColor4ub EdgeColor(vtkIdType edge, vtkIdType point);

  /**
   * Returns the edge control point positions. You generally do not need to
   * override this method, instead change the edge control points on the
   * underlying vtkGraph with SetEdgePoint(), AddEdgePoint(), etc., then call
   * Modified() on the vtkGraph and re-render the scene.
   */
  virtual vtkVector2f EdgePosition(vtkIdType edge, vtkIdType point);

  /**
   * Returns the vertex size in pixels, which is remains the same at any zoom level.
   * Override in a subclass to change the graph vertex size.
   * Note: The item currently supports one size per graph, queried on the first vertex.
   */
  virtual float VertexSize(vtkIdType vertex);

  /**
   * Returns the color of each vertex. Override in a subclass to change the
   * graph vertex colors.
   */
  virtual vtkColor4ub VertexColor(vtkIdType vertex);

  /**
   * Returns the marker type for each vertex, as defined in vtkMarkerUtilities.
   * Override in a subclass to change the graph marker type.
   * Note: The item currently supports one marker type for all vertices,
   * queried on the first vertex.
   */
  virtual int VertexMarker(vtkIdType vertex);

  /**
   * Returns the position of each vertex. You generally do not need to override
   * this method. Instead, change the vertex positions with vtkGraph's SetPoint(),
   * then call Modified() on the graph and re-render the scene.
   */
  virtual vtkVector2f VertexPosition(vtkIdType vertex);

  /**
   * Returns the tooltip for each vertex. Override in a subclass to change the tooltip
   * text.
   */
  virtual vtkStdString VertexTooltip(vtkIdType vertex);

  /**
   * Process events and dispatch to the appropriate member functions.
   */
  static void ProcessEvents(
    vtkObject* caller, unsigned long event, void* clientData, void* callerData);

  /**
   * Return index of hit vertex, or -1 if no hit.
   */
  virtual vtkIdType HitVertex(const vtkVector2f& pos);

  ///@{
  /**
   * Handle mouse events.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& event) override;
  bool MouseLeaveEvent(const vtkContextMouseEvent& event) override;
  bool MouseEnterEvent(const vtkContextMouseEvent& event) override;
  bool MouseButtonPressEvent(const vtkContextMouseEvent& event) override;
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& event) override;
  bool MouseWheelEvent(const vtkContextMouseEvent& event, int delta) override;
  ///@}

  /**
   * Whether this graph item is hit.
   */
  bool Hit(const vtkContextMouseEvent& event) override;

  /**
   * Change the position of the tooltip based on the vertex hovered.
   */
  virtual void PlaceTooltip(vtkIdType v);

private:
  vtkGraphItem(const vtkGraphItem&) = delete;
  void operator=(const vtkGraphItem&) = delete;

  struct Internals;
  Internals* Internal;

  vtkGraph* Graph;
  vtkMTimeType GraphBuildTime;
  vtkNew<vtkImageData> Sprite;
  vtkNew<vtkIncrementalForceLayout> Layout;
  vtkNew<vtkTooltipItem> Tooltip;
};

VTK_ABI_NAMESPACE_END
#endif
