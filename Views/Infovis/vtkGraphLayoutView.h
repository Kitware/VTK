// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkGraphLayoutView
 * @brief   Lays out and displays a graph
 *
 *
 * vtkGraphLayoutView performs graph layout and displays a vtkGraph.
 * You may color and label the vertices and edges using fields in the graph.
 * If coordinates are already assigned to the graph vertices in your graph,
 * set the layout strategy to PassThrough in this view. The default layout
 * is Fast2D which is fast but not that good, for better layout set the
 * layout to Simple2D or ForceDirected. There are also tree and circle
 * layout strategies. :)
 *
 * .SEE ALSO
 * vtkFast2DLayoutStrategy
 * vtkSimple2DLayoutStrategy
 * vtkForceDirectedLayoutStrategy
 *
 * @par Thanks:
 * Thanks a bunch to the holographic unfolding pattern.
 */

#ifndef vtkGraphLayoutView_h
#define vtkGraphLayoutView_h

#include "vtkRenderView.h"
#include "vtkViewsInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkEdgeLayoutStrategy;
class vtkGraphLayoutStrategy;
class vtkRenderedGraphRepresentation;
class vtkViewTheme;

class VTKVIEWSINFOVIS_EXPORT vtkGraphLayoutView : public vtkRenderView
{
public:
  static vtkGraphLayoutView* New();
  vtkTypeMacro(vtkGraphLayoutView, vtkRenderView);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * The array to use for vertex labeling.  Default is "VertexDegree".
   */
  void SetVertexLabelArrayName(const char* name);
  const char* GetVertexLabelArrayName();
  ///@}

  ///@{
  /**
   * The array to use for edge labeling.  Default is "LabelText".
   */
  void SetEdgeLabelArrayName(const char* name);
  const char* GetEdgeLabelArrayName();
  ///@}

  ///@{
  /**
   * Whether to show vertex labels.  Default is off.
   */
  void SetVertexLabelVisibility(bool vis);
  bool GetVertexLabelVisibility();
  vtkBooleanMacro(VertexLabelVisibility, bool);
  ///@}

  ///@{
  /**
   * Whether to hide vertex labels during mouse interactions.  Default is off.
   */
  void SetHideVertexLabelsOnInteraction(bool vis);
  bool GetHideVertexLabelsOnInteraction();
  vtkBooleanMacro(HideVertexLabelsOnInteraction, bool);
  ///@}

  ///@{
  /**
   * Whether to show the edges at all. Default is on
   */
  void SetEdgeVisibility(bool vis);
  bool GetEdgeVisibility();
  vtkBooleanMacro(EdgeVisibility, bool);
  ///@}

  ///@{
  /**
   * Whether to show edge labels.  Default is off.
   */
  void SetEdgeLabelVisibility(bool vis);
  bool GetEdgeLabelVisibility();
  vtkBooleanMacro(EdgeLabelVisibility, bool);
  ///@}

  ///@{
  /**
   * Whether to hide edge labels during mouse interactions.  Default is off.
   */
  void SetHideEdgeLabelsOnInteraction(bool vis);
  bool GetHideEdgeLabelsOnInteraction();
  vtkBooleanMacro(HideEdgeLabelsOnInteraction, bool);
  ///@}

  ///@{
  /**
   * The array to use for coloring vertices.  The default behavior
   * is to color by vertex degree.
   */
  void SetVertexColorArrayName(const char* name);
  const char* GetVertexColorArrayName();
  ///@}

  ///@{
  /**
   * Whether to color vertices.  Default is off.
   */
  void SetColorVertices(bool vis);
  bool GetColorVertices();
  vtkBooleanMacro(ColorVertices, bool);
  ///@}

  ///@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();
  ///@}

  ///@{
  /**
   * Whether to color edges.  Default is off.
   */
  void SetColorEdges(bool vis);
  bool GetColorEdges();
  vtkBooleanMacro(ColorEdges, bool);
  ///@}

  ///@{
  /**
   * Whether edges are selectable. Default is on.
   */
  void SetEdgeSelection(bool vis);
  bool GetEdgeSelection();
  vtkBooleanMacro(EdgeSelection, bool);
  ///@}

  ///@{
  /**
   * The array to use for coloring edges.
   */
  void SetEnabledEdgesArrayName(const char* name);
  const char* GetEnabledEdgesArrayName();
  ///@}

  ///@{
  /**
   * Whether to color edges.  Default is off.
   */
  void SetEnableEdgesByArray(bool vis);
  int GetEnableEdgesByArray();
  ///@}

  ///@{
  /**
   * The array to use for coloring vertices.
   */
  void SetEnabledVerticesArrayName(const char* name);
  const char* GetEnabledVerticesArrayName();
  ///@}

  ///@{
  /**
   * Whether to color vertices.  Default is off.
   */
  void SetEnableVerticesByArray(bool vis);
  int GetEnableVerticesByArray();
  ///@}

  ///@{
  /**
   * The array used for scaling (if ScaledGlyphs is ON)
   */
  void SetScalingArrayName(const char* name);
  const char* GetScalingArrayName();
  ///@}

  ///@{
  /**
   * Whether to use scaled glyphs or not.  Default is off.
   */
  void SetScaledGlyphs(bool arg);
  bool GetScaledGlyphs();
  vtkBooleanMacro(ScaledGlyphs, bool);
  ///@}

  ///@{
  /**
   * The layout strategy to use when performing the graph layout.
   * The possible strings are:
   * - "Random"         Randomly places vertices in a box.
   * - "Force Directed" A layout in 3D or 2D simulating forces on edges.
   * - "Simple 2D"      A simple 2D force directed layout.
   * - "Clustering 2D"  A 2D force directed layout that's just like
   * simple 2D but uses some techniques to cluster better.
   * - "Community 2D"   A linear-time 2D layout that's just like
   * Fast 2D but looks for and uses a community
   * array to 'accentuate' clusters.
   * - "Fast 2D"       A linear-time 2D layout.
   * - "Pass Through"  Use locations assigned to the input.
   * - "Circular"      Places vertices uniformly on a circle.
   * - "Cone"          Cone tree layout.
   * - "Span Tree"     Span Tree Layout.
   * Default is "Simple 2D".
   */
  void SetLayoutStrategy(const char* name);
  void SetLayoutStrategyToRandom() { this->SetLayoutStrategy("Random"); }
  void SetLayoutStrategyToForceDirected() { this->SetLayoutStrategy("Force Directed"); }
  void SetLayoutStrategyToSimple2D() { this->SetLayoutStrategy("Simple 2D"); }
  void SetLayoutStrategyToClustering2D() { this->SetLayoutStrategy("Clustering 2D"); }
  void SetLayoutStrategyToCommunity2D() { this->SetLayoutStrategy("Community 2D"); }
  void SetLayoutStrategyToFast2D() { this->SetLayoutStrategy("Fast 2D"); }
  void SetLayoutStrategyToPassThrough() { this->SetLayoutStrategy("Pass Through"); }
  void SetLayoutStrategyToCircular() { this->SetLayoutStrategy("Circular"); }
  void SetLayoutStrategyToTree() { this->SetLayoutStrategy("Tree"); }
  void SetLayoutStrategyToCosmicTree() { this->SetLayoutStrategy("Cosmic Tree"); }
  void SetLayoutStrategyToCone() { this->SetLayoutStrategy("Cone"); }
  void SetLayoutStrategyToSpanTree() { this->SetLayoutStrategy("Span Tree"); }
  const char* GetLayoutStrategyName();
  ///@}

  ///@{
  /**
   * The layout strategy to use when performing the graph layout.
   * This signature allows an application to create a layout
   * object directly and simply set the pointer through this method.
   */
  vtkGraphLayoutStrategy* GetLayoutStrategy();
  void SetLayoutStrategy(vtkGraphLayoutStrategy* s);
  ///@}

  ///@{
  /**
   * The layout strategy to use when performing the edge layout.
   * The possible strings are:
   * "Arc Parallel"   - Arc parallel edges and self loops.
   * "Pass Through"   - Use edge routes assigned to the input.
   * Default is "Arc Parallel".
   */
  void SetEdgeLayoutStrategy(const char* name);
  void SetEdgeLayoutStrategyToArcParallel() { this->SetEdgeLayoutStrategy("Arc Parallel"); }
  void SetEdgeLayoutStrategyToPassThrough() { this->SetEdgeLayoutStrategy("Pass Through"); }
  const char* GetEdgeLayoutStrategyName();
  ///@}

  ///@{
  /**
   * The layout strategy to use when performing the edge layout.
   * This signature allows an application to create a layout
   * object directly and simply set the pointer through this method.
   */
  vtkEdgeLayoutStrategy* GetEdgeLayoutStrategy();
  void SetEdgeLayoutStrategy(vtkEdgeLayoutStrategy* s);
  ///@}

  /**
   * Associate the icon at index "index" in the vtkTexture to all vertices
   * containing "type" as a value in the vertex attribute array specified by
   * IconArrayName.
   */
  void AddIconType(const char* type, int index);

  /**
   * Clear all icon mappings.
   */
  void ClearIconTypes();

  /**
   * Specify where the icons should be placed in relation to the vertex.
   * See vtkIconGlyphFilter.h for possible values.
   */
  void SetIconAlignment(int alignment);

  ///@{
  /**
   * Whether icons are visible (default off).
   */
  void SetIconVisibility(bool b);
  bool GetIconVisibility();
  vtkBooleanMacro(IconVisibility, bool);
  ///@}

  ///@{
  /**
   * The array used for assigning icons
   */
  void SetIconArrayName(const char* name);
  const char* GetIconArrayName();
  ///@}

  ///@{
  /**
   * The type of glyph to use for the vertices
   */
  void SetGlyphType(int type);
  int GetGlyphType();
  ///@}

  ///@{
  /**
   * The size of the font used for vertex labeling
   */
  virtual void SetVertexLabelFontSize(int size);
  virtual int GetVertexLabelFontSize();
  ///@}

  ///@{
  /**
   * The size of the font used for edge labeling
   */
  virtual void SetEdgeLabelFontSize(int size);
  virtual int GetEdgeLabelFontSize();
  ///@}

  ///@{
  /**
   * Whether the scalar bar for edges is visible.  Default is off.
   */
  void SetEdgeScalarBarVisibility(bool vis);
  bool GetEdgeScalarBarVisibility();
  ///@}

  ///@{
  /**
   * Whether the scalar bar for vertices is visible.  Default is off.
   */
  void SetVertexScalarBarVisibility(bool vis);
  bool GetVertexScalarBarVisibility();
  ///@}

  /**
   * Reset the camera based on the bounds of the selected region.
   */
  void ZoomToSelection();

  /**
   * Is the graph layout complete? This method is useful
   * for when the strategy is iterative and the application
   * wants to show the iterative progress of the graph layout
   * See Also: UpdateLayout();
   */
  virtual int IsLayoutComplete();

  /**
   * This method is useful for when the strategy is iterative
   * and the application wants to show the iterative progress
   * of the graph layout. The application would have something like
   * while(!IsLayoutComplete())
   * {
   * UpdateLayout();
   * }
   * See Also: IsLayoutComplete();
   */
  virtual void UpdateLayout();

protected:
  vtkGraphLayoutView();
  ~vtkGraphLayoutView() override;

  ///@{
  /**
   * Overrides behavior in vtkView to create a vtkRenderedGraphRepresentation
   * by default.
   */
  vtkDataRepresentation* CreateDefaultRepresentation(vtkAlgorithmOutput* conn) override;
  virtual vtkRenderedGraphRepresentation* GetGraphRepresentation();
  // Called to process events.  Overrides behavior in vtkRenderView.
  void ProcessEvents(vtkObject* caller, unsigned long eventId, void* callData) override;
  ///@}

private:
  vtkGraphLayoutView(const vtkGraphLayoutView&) = delete;
  void operator=(const vtkGraphLayoutView&) = delete;
  bool VertexLabelsRequested;
  bool EdgeLabelsRequested;
};

VTK_ABI_NAMESPACE_END
#endif
