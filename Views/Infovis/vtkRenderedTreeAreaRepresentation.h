// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkRenderedTreeAreaRepresentation
 *
 *
 */

#ifndef vtkRenderedTreeAreaRepresentation_h
#define vtkRenderedTreeAreaRepresentation_h

#include "vtkRenderedRepresentation.h"
#include "vtkViewsInfovisModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkActor;
class vtkActor2D;
class vtkAreaLayout;
class vtkAreaLayoutStrategy;
class vtkConvertSelection;
class vtkEdgeCenters;
class vtkLabeledDataMapper;
class vtkPointSetToLabelHierarchy;
class vtkPolyData;
class vtkPolyDataAlgorithm;
class vtkPolyDataMapper;
class vtkScalarBarWidget;
class vtkTextProperty;
class vtkTreeFieldAggregator;
class vtkTreeLevelsFilter;
class vtkVertexDegree;
class vtkWorldPointPicker;

class VTKVIEWSINFOVIS_EXPORT vtkRenderedTreeAreaRepresentation : public vtkRenderedRepresentation
{
public:
  static vtkRenderedTreeAreaRepresentation* New();
  vtkTypeMacro(vtkRenderedTreeAreaRepresentation, vtkRenderedRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Set the label render mode.
   * QT - Use vtkQtTreeRingLabeler with fitted labeling
   * and unicode support. Requires VTK_USE_QT to be on.
   * FREETYPE - Use standard freetype text rendering.
   */
  void SetLabelRenderMode(int mode) override;

  ///@{
  /**
   * The array to use for area labeling.  Default is "label".
   */
  virtual void SetAreaLabelArrayName(const char* name);
  virtual const char* GetAreaLabelArrayName();
  ///@}

  ///@{
  /**
   * The array to use for area sizes. Default is "size".
   */
  virtual void SetAreaSizeArrayName(const char* name);
  virtual const char* GetAreaSizeArrayName();
  ///@}

  ///@{
  /**
   * The array to use for area labeling priority.
   * Default is "GraphVertexDegree".
   */
  virtual void SetAreaLabelPriorityArrayName(const char* name);
  virtual const char* GetAreaLabelPriorityArrayName();
  ///@}

  ///@{
  /**
   * The array to use for edge labeling.  Default is "label".
   */
  virtual void SetGraphEdgeLabelArrayName(const char* name)
  {
    this->SetGraphEdgeLabelArrayName(name, 0);
  }
  virtual void SetGraphEdgeLabelArrayName(const char* name, int idx);
  virtual const char* GetGraphEdgeLabelArrayName() { return this->GetGraphEdgeLabelArrayName(0); }
  virtual const char* GetGraphEdgeLabelArrayName(int idx);
  ///@}

  ///@{
  /**
   * The text property for the graph edge labels.
   */
  virtual void SetGraphEdgeLabelTextProperty(vtkTextProperty* tp)
  {
    this->SetGraphEdgeLabelTextProperty(tp, 0);
  }
  virtual void SetGraphEdgeLabelTextProperty(vtkTextProperty* tp, int idx);
  virtual vtkTextProperty* GetGraphEdgeLabelTextProperty()
  {
    return this->GetGraphEdgeLabelTextProperty(0);
  }
  virtual vtkTextProperty* GetGraphEdgeLabelTextProperty(int idx);
  ///@}

  ///@{
  /**
   * The name of the array whose value appears when the mouse hovers
   * over a rectangle in the treemap.
   */
  vtkSetStringMacro(AreaHoverArrayName);
  vtkGetStringMacro(AreaHoverArrayName);
  ///@}

  ///@{
  /**
   * Whether to show area labels.  Default is off.
   */
  virtual void SetAreaLabelVisibility(bool vis);
  virtual bool GetAreaLabelVisibility();
  vtkBooleanMacro(AreaLabelVisibility, bool);
  ///@}

  ///@{
  /**
   * The text property for the area labels.
   */
  virtual void SetAreaLabelTextProperty(vtkTextProperty* tp);
  virtual vtkTextProperty* GetAreaLabelTextProperty();
  ///@}

  ///@{
  /**
   * Whether to show edge labels.  Default is off.
   */
  virtual void SetGraphEdgeLabelVisibility(bool vis) { this->SetGraphEdgeLabelVisibility(vis, 0); }
  virtual void SetGraphEdgeLabelVisibility(bool vis, int idx);
  virtual bool GetGraphEdgeLabelVisibility() { return this->GetGraphEdgeLabelVisibility(0); }
  virtual bool GetGraphEdgeLabelVisibility(int idx);
  vtkBooleanMacro(GraphEdgeLabelVisibility, bool);
  ///@}

  ///@{
  /**
   * The array to use for coloring vertices.  Default is "color".
   */
  void SetAreaColorArrayName(const char* name);
  const char* GetAreaColorArrayName();
  ///@}

  ///@{
  /**
   * Whether to color vertices.  Default is off.
   */
  virtual void SetColorAreasByArray(bool vis);
  virtual bool GetColorAreasByArray();
  vtkBooleanMacro(ColorAreasByArray, bool);
  ///@}

  ///@{
  /**
   * The array to use for coloring edges.  Default is "color".
   */
  virtual void SetGraphEdgeColorArrayName(const char* name)
  {
    this->SetGraphEdgeColorArrayName(name, 0);
  }
  virtual void SetGraphEdgeColorArrayName(const char* name, int idx);
  virtual const char* GetGraphEdgeColorArrayName() { return this->GetGraphEdgeColorArrayName(0); }
  virtual const char* GetGraphEdgeColorArrayName(int idx);
  ///@}

  /**
   * Set the color to be the spline fraction
   */
  virtual void SetGraphEdgeColorToSplineFraction() { this->SetGraphEdgeColorToSplineFraction(0); }
  virtual void SetGraphEdgeColorToSplineFraction(int idx);

  ///@{
  /**
   * Whether to color edges.  Default is off.
   */
  virtual void SetColorGraphEdgesByArray(bool vis) { this->SetColorGraphEdgesByArray(vis, 0); }
  virtual void SetColorGraphEdgesByArray(bool vis, int idx);
  virtual bool GetColorGraphEdgesByArray() { return this->GetColorGraphEdgesByArray(0); }
  virtual bool GetColorGraphEdgesByArray(int idx);
  vtkBooleanMacro(ColorGraphEdgesByArray, bool);
  ///@}

  ///@{
  /**
   * The name of the array whose value appears when the mouse hovers
   * over a graph edge.
   */
  virtual void SetGraphHoverArrayName(const char* name) { this->SetGraphHoverArrayName(name, 0); }
  virtual void SetGraphHoverArrayName(const char* name, int idx);
  virtual const char* GetGraphHoverArrayName() { return this->GetGraphHoverArrayName(0); }
  virtual const char* GetGraphHoverArrayName(int idx);
  ///@}

  ///@{
  /**
   * Set the region shrink percentage between 0.0 and 1.0.
   */
  virtual void SetShrinkPercentage(double value);
  virtual double GetShrinkPercentage();
  ///@}

  ///@{
  /**
   * Set the bundling strength.
   */
  virtual void SetGraphBundlingStrength(double strength)
  {
    this->SetGraphBundlingStrength(strength, 0);
  }
  virtual void SetGraphBundlingStrength(double strength, int idx);
  virtual double GetGraphBundlingStrength() { return this->GetGraphBundlingStrength(0); }
  virtual double GetGraphBundlingStrength(int idx);
  ///@}

  ///@{
  /**
   * Sets the spline type for the graph edges.
   * vtkSplineGraphEdges::CUSTOM uses a vtkCardinalSpline.
   * vtkSplineGraphEdges::BSPLINE uses a b-spline.
   * The default is BSPLINE.
   */
  virtual void SetGraphSplineType(int type, int idx);
  virtual int GetGraphSplineType(int idx);
  ///@}

  ///@{
  /**
   * The layout strategy for producing spatial regions for the tree.
   */
  virtual void SetAreaLayoutStrategy(vtkAreaLayoutStrategy* strategy);
  virtual vtkAreaLayoutStrategy* GetAreaLayoutStrategy();
  ///@}

  ///@{
  /**
   * The filter for converting areas to polydata. This may e.g. be
   * vtkTreeMapToPolyData or vtkTreeRingToPolyData.
   * The filter must take a vtkTree as input and produce vtkPolyData.
   */
  virtual void SetAreaToPolyData(vtkPolyDataAlgorithm* areaToPoly);
  vtkGetObjectMacro(AreaToPolyData, vtkPolyDataAlgorithm);
  ///@}

  ///@{
  /**
   * Whether the area represents radial or rectangular coordinates.
   */
  vtkSetMacro(UseRectangularCoordinates, bool);
  vtkGetMacro(UseRectangularCoordinates, bool);
  vtkBooleanMacro(UseRectangularCoordinates, bool);
  ///@}

  ///@{
  /**
   * The mapper for rendering labels on areas. This may e.g. be
   * vtkDynamic2DLabelMapper or vtkTreeMapLabelMapper.
   */
  virtual void SetAreaLabelMapper(vtkLabeledDataMapper* mapper);
  vtkGetObjectMacro(AreaLabelMapper, vtkLabeledDataMapper);
  ///@}

  /**
   * Apply the theme to this view.
   */
  void ApplyViewTheme(vtkViewTheme* theme) override;

  ///@{
  /**
   * Visibility of scalar bar actor for edges.
   */
  virtual void SetEdgeScalarBarVisibility(bool b);
  virtual bool GetEdgeScalarBarVisibility();
  ///@}

protected:
  vtkRenderedTreeAreaRepresentation();
  ~vtkRenderedTreeAreaRepresentation() override;

  ///@{
  /**
   * Called by the view to add/remove this representation.
   */
  bool AddToView(vtkView* view) override;
  bool RemoveFromView(vtkView* view) override;
  ///@}

  vtkSelection* ConvertSelection(vtkView* view, vtkSelection* sel) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  void PrepareForRendering(vtkRenderView* view) override;

  bool ValidIndex(int idx);

  void UpdateHoverHighlight(vtkView* view, int x, int y);

  std::string GetHoverStringInternal(vtkSelection* sel) override;

  vtkSmartPointer<vtkWorldPointPicker> Picker;
  vtkSmartPointer<vtkApplyColors> ApplyColors;
  vtkSmartPointer<vtkTreeLevelsFilter> TreeLevels;
  vtkSmartPointer<vtkVertexDegree> VertexDegree;
  vtkSmartPointer<vtkTreeFieldAggregator> TreeAggregation;
  vtkSmartPointer<vtkAreaLayout> AreaLayout;
  vtkSmartPointer<vtkPolyDataMapper> AreaMapper;
  vtkSmartPointer<vtkActor> AreaActor;
  vtkSmartPointer<vtkActor2D> AreaLabelActor;
  vtkSmartPointer<vtkPolyData> HighlightData;
  vtkSmartPointer<vtkPolyDataMapper> HighlightMapper;
  vtkSmartPointer<vtkActor> HighlightActor;
  vtkPolyDataAlgorithm* AreaToPolyData;
  vtkLabeledDataMapper* AreaLabelMapper;
  vtkSmartPointer<vtkScalarBarWidget> EdgeScalarBar;
  vtkSmartPointer<vtkPointSetToLabelHierarchy> AreaLabelHierarchy;
  vtkSmartPointer<vtkPolyData> EmptyPolyData;

  vtkSetStringMacro(AreaSizeArrayNameInternal);
  vtkGetStringMacro(AreaSizeArrayNameInternal);
  char* AreaSizeArrayNameInternal;
  vtkSetStringMacro(AreaColorArrayNameInternal);
  vtkGetStringMacro(AreaColorArrayNameInternal);
  char* AreaColorArrayNameInternal;
  vtkSetStringMacro(AreaLabelArrayNameInternal);
  vtkGetStringMacro(AreaLabelArrayNameInternal);
  char* AreaLabelArrayNameInternal;
  vtkSetStringMacro(AreaLabelPriorityArrayNameInternal);
  vtkGetStringMacro(AreaLabelPriorityArrayNameInternal);
  char* AreaLabelPriorityArrayNameInternal;
  vtkSetStringMacro(GraphEdgeColorArrayNameInternal);
  vtkGetStringMacro(GraphEdgeColorArrayNameInternal);
  char* GraphEdgeColorArrayNameInternal;
  vtkGetStringMacro(AreaHoverTextInternal);
  vtkSetStringMacro(AreaHoverTextInternal);
  char* AreaHoverTextInternal;
  char* AreaHoverArrayName;

  bool UseRectangularCoordinates;

private:
  vtkRenderedTreeAreaRepresentation(const vtkRenderedTreeAreaRepresentation&) = delete;
  void operator=(const vtkRenderedTreeAreaRepresentation&) = delete;

  class Internals;
  Internals* Implementation;
};

VTK_ABI_NAMESPACE_END
#endif
