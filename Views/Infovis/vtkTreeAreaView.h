/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeAreaView.h

  -------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkTreeAreaView - Accepts a graph and a hierarchy - currently
// a tree - and provides a hierarchy-aware display.  Currently, this means
// displaying the hierarchy using a tree ring layout, then rendering the graph
// vertices as leaves of the tree with curved graph edges between leaves.
//
// .SECTION Description
// Takes a graph and a hierarchy (currently a tree) and lays out the graph
// vertices based on their categorization within the hierarchy.
//
// .SEE ALSO
// vtkGraphLayoutView
//
// .SECTION Thanks
// Thanks to Jason Shepherd for implementing this class

#ifndef __vtkTreeAreaView_h
#define __vtkTreeAreaView_h

#include "vtkViewsInfovisModule.h" // For export macro
#include "vtkRenderView.h"

class vtkAreaLayoutStrategy;
class vtkGraph;
class vtkLabeledDataMapper;
class vtkPolyDataAlgorithm;
class vtkRenderedTreeAreaRepresentation;
class vtkTree;

class VTKVIEWSINFOVIS_EXPORT vtkTreeAreaView : public vtkRenderView
{
public:
  static vtkTreeAreaView *New();
  vtkTypeMacro(vtkTreeAreaView, vtkRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the tree and graph representations to the appropriate input ports.
  vtkDataRepresentation* SetTreeFromInputConnection(vtkAlgorithmOutput* conn);
  vtkDataRepresentation* SetTreeFromInput(vtkTree* input);
  vtkDataRepresentation* SetGraphFromInputConnection(vtkAlgorithmOutput* conn);
  vtkDataRepresentation* SetGraphFromInput(vtkGraph* input);

  // Description:
  // The array to use for area labeling.  Default is "label".
  void SetAreaLabelArrayName(const char* name);
  const char* GetAreaLabelArrayName();

  // Description:
  // The array to use for area sizes. Default is "size".
  void SetAreaSizeArrayName(const char* name);
  const char* GetAreaSizeArrayName();

  // Description:
  // The array to use for area labeling priority.
  // Default is "GraphVertexDegree".
  void SetLabelPriorityArrayName(const char* name);
  const char* GetLabelPriorityArrayName();

  // Description:
  // The array to use for edge labeling.  Default is "label".
  void SetEdgeLabelArrayName(const char* name);
  const char* GetEdgeLabelArrayName();

  // Description:
  // The name of the array whose value appears when the mouse hovers
  // over a rectangle in the treemap.
  // This must be a string array.
  void SetAreaHoverArrayName(const char* name);
  const char* GetAreaHoverArrayName();

  // Description:
  // Whether to show area labels.  Default is off.
  void SetAreaLabelVisibility(bool vis);
  bool GetAreaLabelVisibility();
  vtkBooleanMacro(AreaLabelVisibility, bool);

  // Description:
  // Whether to show edge labels.  Default is off.
  void SetEdgeLabelVisibility(bool vis);
  bool GetEdgeLabelVisibility();
  vtkBooleanMacro(EdgeLabelVisibility, bool);

  // Description:
  // The array to use for coloring vertices.  Default is "color".
  void SetAreaColorArrayName(const char* name);
  const char* GetAreaColorArrayName();

  // Description:
  // Whether to color vertices.  Default is off.
  void SetColorAreas(bool vis);
  bool GetColorAreas();
  vtkBooleanMacro(ColorAreas, bool);

  // Description:
  // The array to use for coloring edges.  Default is "color".
  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();

  // Description:
  // Set the color to be the spline fraction
  void SetEdgeColorToSplineFraction();

  // Description:
  // Set the region shrink percentage between 0.0 and 1.0.
  void SetShrinkPercentage(double value);
  double GetShrinkPercentage();

  // Description:
  // Whether to color edges.  Default is off.
  void SetColorEdges(bool vis);
  bool GetColorEdges();
  vtkBooleanMacro(ColorEdges, bool);

  // Description:
  // Set the bundling strength.
  void SetBundlingStrength(double strength);
  double GetBundlingStrength();

  // Description:
  // The size of the font used for area labeling
  virtual void SetAreaLabelFontSize(const int size);
  virtual int GetAreaLabelFontSize();

  // Description:
  // The size of the font used for edge labeling
  virtual void SetEdgeLabelFontSize(const int size);
  virtual int GetEdgeLabelFontSize();

  // Description:
  // The layout strategy for producing spatial regions for the tree.
  virtual void SetLayoutStrategy(vtkAreaLayoutStrategy* strategy);
  virtual vtkAreaLayoutStrategy* GetLayoutStrategy();

  // Description:
  // Whether the area represents radial or rectangular coordinates.
  virtual void SetUseRectangularCoordinates(bool rect);
  virtual bool GetUseRectangularCoordinates();
  vtkBooleanMacro(UseRectangularCoordinates, bool);

  // Description:
  // Visibility of scalar bar actor for edges.
  virtual void SetEdgeScalarBarVisibility(bool b);
  virtual bool GetEdgeScalarBarVisibility();

protected:
  vtkTreeAreaView();
  ~vtkTreeAreaView();

  // Description:
  // The filter for converting areas to polydata. This may e.g. be
  // vtkTreeMapToPolyData or vtkTreeRingToPolyData.
  // The filter must take a vtkTree as input and produce vtkPolyData.
  virtual void SetAreaToPolyData(vtkPolyDataAlgorithm* areaToPoly);
  virtual vtkPolyDataAlgorithm* GetAreaToPolyData();

  // Description:
  // The mapper for rendering labels on areas. This may e.g. be
  // vtkDynamic2DLabelMapper or vtkTreeMapLabelMapper.
  virtual void SetAreaLabelMapper(vtkLabeledDataMapper* mapper);
  virtual vtkLabeledDataMapper* GetAreaLabelMapper();

  // Description:
  // Overrides behavior in vtkView to create a vtkRenderedGraphRepresentation
  // by default.
  virtual vtkDataRepresentation* CreateDefaultRepresentation(vtkAlgorithmOutput* conn);
  virtual vtkRenderedTreeAreaRepresentation* GetTreeAreaRepresentation();

private:
  vtkTreeAreaView(const vtkTreeAreaView&);  // Not implemented.
  void operator=(const vtkTreeAreaView&);  // Not implemented.
};

#endif
