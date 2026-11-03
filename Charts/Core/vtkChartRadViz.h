// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkChartRadViz
 * @brief   Handles the interaction for vtkPlotRadViz
 *
 * You can rotate the vertices by dragging on any vertex
 * position. User feedback is provided by drawing the active vertex using
 * the active color (vtkPlotRadViz::ActiveColor) when the mouse
 * pointer is over the vertex or when the vertices are rotated (by
 * dragging on a vertex).
 *
 * You can resize the lens by dragging on the lens outline. User
 * feedback is provided by drawing the lens outline using the active
 * color when the mouse pointer is over the outline or when the lens
 * is resized.
 *
 * You can move the lens by dragging on the lens interior. User
 * feedback is provided by drawing the lens center using the active
 * color when the mouse pointer is over the lens interior or when the
 * lens is moved. The lens center is not drawn otherwise.
 *
 * The plot invokes vtkCommand::StartInteractionEvent when a drag starts,
 * vtkCommand::InteractionEvent on every mouse move during the drag and
 * vtkCommand::EndInteractionEvent when the drag ends.
 *
 * The title is by default bold, black Arial with a font size of
 * 18, one step larger than the vertex labels of vtkPlotRadViz.
 *
 * @sa vtkPlotRadViz
 */

#ifndef vtkChartRadViz_h
#define vtkChartRadViz_h

#include "vtkChart.h"
#include "vtkChartsCoreModule.h" // For export macro
#include "vtkWrappingHints.h"    // For VTK_MARSHALAUTO
#include <memory>                // For std::unique_ptr
#include <string>                // For variable names

VTK_ABI_NAMESPACE_BEGIN
class vtkChartLegend;
class vtkTooltipItem;
class vtkPlotRadViz;
class vtkStringArray;

class VTKCHARTSCORE_EXPORT VTK_MARSHALAUTO vtkChartRadViz : public vtkChart
{
public:
  vtkTypeMacro(vtkChartRadViz, vtkChart);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Creates a RadViz chart object.
   */
  static vtkChartRadViz* New();

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  void Update() override;

  /**
   * Paint event for the chart, called whenever the chart needs to be drawn.
   */
  bool Paint(vtkContext2D* painter) override;

  using vtkChart::AddPlot;
  /**
   * Add a plot to the chart. If a plot already exists, it does nothing.
   * It returns the added or existing plot.
   */
  vtkPlot* AddPlot(int type) override;

  /**
   * Set the plot to use for the chart. Since this type of chart can
   * only contain one plot, this will replace the previous plot. Passing
   * nullptr removes the plot.
   */
  virtual void SetPlot(vtkPlotRadViz* plot);

  /**
   * Get the plot at the specified index, returns null if the index is invalid.
   */
  vtkPlot* GetPlot(vtkIdType index) override;

  /**
   * Get the number of plots the chart contains.
   */
  vtkIdType GetNumberOfPlots() override;

  ///@{
  /**
   * Set the visibility of the specified variable. Variables are the
   * columns of an input table or the components of the array processed
   * for an input dataset. Only visible variables are used to compute the
   * RadViz plot. This is a passthrough to the underlying vtkPlotRadViz
   * (and, in turn, to its vtkMultiVariateToRGBA); see
   * vtkMultiVariateToRGBA::SetVariableVisibility and
   * vtkPlotRadViz::SetVariableVisibility for details, including the
   * default visibility of every variable before this is called.
   * A no-op if the chart has no plot yet.
   */
  void SetVariableVisibility(const std::string& name, bool visible);

  /**
   * Set the visibility of all variables.
   */
  void SetVariableVisibilityAll(bool visible);

  /**
   * Get the visibility of the specified variable.
   */
  bool GetVariableVisibility(const std::string& name);

  /**
   * Get a list of the variables explicitly marked visible, and
   * the order in which they are used. Returns nullptr if the chart has no
   * plot yet.
   */
  virtual vtkStringArray* GetVisibleVariables();

  /**
   * Set the list of visible variables, and the order in which they will be used.
   */
  virtual void SetVisibleVariables(vtkStringArray* names);

  /**
   * Set the vtkContextScene for the item, always set for an item in a scene.
   */
  void SetScene(vtkContextScene* scene) override;
  ///@}

  ///@{
  /**
   * Return true if the supplied x, y coordinates are inside the item.
   */
  bool Hit(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse enter event.
   * Return true if the item holds the event, false if the event can be propagated to other items.
   */
  bool MouseEnterEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse move event.
   * Return true if the item holds the event, false if the event can be propagated to other items.
   */
  bool MouseMoveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse leave event.
   * Return true if the item holds the event, false if the event can be propagated to other items.
   */
  bool MouseLeaveEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button down event.
   * Return true if the item holds the event, false if the event can be propagated to other items.
   */
  bool MouseButtonPressEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse button release event.
   * Return true if the item holds the event, false if the event can be propagated to other items.
   */
  bool MouseButtonReleaseEvent(const vtkContextMouseEvent& mouse) override;

  /**
   * Mouse wheel event, positive delta indicates forward movement of the wheel.
   */
  bool MouseWheelEvent(const vtkContextMouseEvent& mouse, int delta) override;
  ///@}

protected:
  vtkChartRadViz();
  ~vtkChartRadViz() override;
  /**
   * Gets the active element (vertex, lens outline or lens interior) based
   * on the mouse position.
   */
  void GetActiveElement(const vtkVector2f& pos, int& activeVertexIndex, bool& activeLensOutline,
    bool& activeLensInterior);

private:
  vtkChartRadViz(const vtkChartRadViz&) = delete;
  void operator=(const vtkChartRadViz&) = delete;

  /**
   * Private implementation details
   */
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif // vtkChartRadViz_h
