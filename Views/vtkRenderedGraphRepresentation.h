/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderedGraphRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
// .NAME vtkRenderedGraphRepresentation - 
//
// .SECTION Description

#ifndef __vtkRenderedGraphRepresentation_h
#define __vtkRenderedGraphRepresentation_h

#include "vtkRenderedRepresentation.h"
#include "vtkSmartPointer.h" // for SP ivars

class vtkActor;
class vtkApplyColors;
class vtkApplyIcons;
class vtkEdgeCenters;
class vtkEdgeLayout;
class vtkEdgeLayoutStrategy;
class vtkGraphLayout;
class vtkGraphLayoutStrategy;
class vtkGraphToGlyphs;
class vtkGraphToPoints;
class vtkGraphToPolyData;
class vtkIconGlyphFilter;
class vtkInformation;
class vtkInformationVector;
class vtkLookupTable;
class vtkPerturbCoincidentVertices;
class vtkPointSetToLabelHierarchy;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkPolyDataMapper2D;
class vtkRemoveHiddenData;
class vtkRenderView;
class vtkScalarBarWidget;
class vtkScalarsToColors;
class vtkTextProperty;
class vtkTexturedActor2D;
class vtkTransformCoordinateSystems;
class vtkVertexDegree;
class vtkView;
class vtkViewTheme;

class VTK_VIEWS_EXPORT vtkRenderedGraphRepresentation : public vtkRenderedRepresentation
{
public:
  static vtkRenderedGraphRepresentation* New();
  vtkTypeMacro(vtkRenderedGraphRepresentation, vtkRenderedRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // ------------------------------------------------------------------------
  // Vertex labels

  virtual void SetVertexLabelArrayName(const char* name);
  virtual const char* GetVertexLabelArrayName();
  virtual void SetVertexLabelPriorityArrayName(const char* name);
  virtual const char* GetVertexLabelPriorityArrayName();
  virtual void SetVertexLabelVisibility(bool b);
  virtual bool GetVertexLabelVisibility();
  vtkBooleanMacro(VertexLabelVisibility, bool);
  virtual void SetVertexLabelTextProperty(vtkTextProperty* p);
  virtual vtkTextProperty* GetVertexLabelTextProperty();
  vtkSetStringMacro(VertexHoverArrayName);
  vtkGetStringMacro(VertexHoverArrayName);
  // Description:
  // Whether to hide the display of vertex labels during mouse interaction.  Default is off.
  vtkSetMacro(HideVertexLabelsOnInteraction, bool)
  vtkGetMacro(HideVertexLabelsOnInteraction, bool)
  vtkBooleanMacro(HideVertexLabelsOnInteraction, bool)

  // ------------------------------------------------------------------------
  // Edge labels

  virtual void SetEdgeLabelArrayName(const char* name);
  virtual const char* GetEdgeLabelArrayName();
  virtual void SetEdgeLabelPriorityArrayName(const char* name);
  virtual const char* GetEdgeLabelPriorityArrayName();
  virtual void SetEdgeLabelVisibility(bool b);
  virtual bool GetEdgeLabelVisibility();
  vtkBooleanMacro(EdgeLabelVisibility, bool);
  virtual void SetEdgeLabelTextProperty(vtkTextProperty* p);
  virtual vtkTextProperty* GetEdgeLabelTextProperty();
  vtkSetStringMacro(EdgeHoverArrayName);
  vtkGetStringMacro(EdgeHoverArrayName);
  // Description:
  // Whether to hide the display of edge labels during mouse interaction.  Default is off.
  vtkSetMacro(HideEdgeLabelsOnInteraction, bool)
  vtkGetMacro(HideEdgeLabelsOnInteraction, bool)
  vtkBooleanMacro(HideEdgeLabelsOnInteraction, bool)

  // ------------------------------------------------------------------------
  // Vertex icons

  virtual void SetVertexIconArrayName(const char* name);
  virtual const char* GetVertexIconArrayName();
  virtual void SetVertexIconPriorityArrayName(const char* name);
  virtual const char* GetVertexIconPriorityArrayName();
  virtual void SetVertexIconVisibility(bool b);
  virtual bool GetVertexIconVisibility();
  vtkBooleanMacro(VertexIconVisibility, bool);
  virtual void AddVertexIconType(const char* name, int type);
  virtual void ClearVertexIconTypes();
  virtual void SetUseVertexIconTypeMap(bool b);
  virtual bool GetUseVertexIconTypeMap();
  vtkBooleanMacro(UseVertexIconTypeMap, bool);
  virtual void SetVertexIconAlignment(int align);
  virtual int GetVertexIconAlignment();
  virtual void SetVertexSelectedIcon(int icon);
  virtual int GetVertexSelectedIcon();

  // Description:
  // Set the mode to one of
  // <ul>
  // <li>vtkApplyIcons::SELECTED_ICON - use VertexSelectedIcon
  // <li>vtkApplyIcons::SELECTED_OFFSET - use VertexSelectedIcon as offset
  // <li>vtkApplyIcons::ANNOTATION_ICON - use current annotation icon
  // <li>vtkApplyIcons::IGNORE_SELECTION - ignore selected elements
  // </ul>
  // The default is IGNORE_SELECTION.
  virtual void SetVertexIconSelectionMode(int mode);
  virtual int GetVertexIconSelectionMode();
  virtual void SetVertexIconSelectionModeToSelectedIcon()
    { this->SetVertexIconSelectionMode(0); }
  virtual void SetVertexIconSelectionModeToSelectedOffset()
    { this->SetVertexIconSelectionMode(1); }
  virtual void SetVertexIconSelectionModeToAnnotationIcon()
    { this->SetVertexIconSelectionMode(2); }
  virtual void SetVertexIconSelectionModeToIgnoreSelection()
    { this->SetVertexIconSelectionMode(3); }

  // ------------------------------------------------------------------------
  // Edge icons

  virtual void SetEdgeIconArrayName(const char* name);
  virtual const char* GetEdgeIconArrayName();
  virtual void SetEdgeIconPriorityArrayName(const char* name);
  virtual const char* GetEdgeIconPriorityArrayName();
  virtual void SetEdgeIconVisibility(bool b);
  virtual bool GetEdgeIconVisibility();
  vtkBooleanMacro(EdgeIconVisibility, bool);
  virtual void AddEdgeIconType(const char* name, int type);
  virtual void ClearEdgeIconTypes();
  virtual void SetUseEdgeIconTypeMap(bool b);
  virtual bool GetUseEdgeIconTypeMap();
  vtkBooleanMacro(UseEdgeIconTypeMap, bool);
  virtual void SetEdgeIconAlignment(int align);
  virtual int GetEdgeIconAlignment();

  // ------------------------------------------------------------------------
  // Vertex colors

  virtual void SetColorVerticesByArray(bool b);
  virtual bool GetColorVerticesByArray();
  vtkBooleanMacro(ColorVerticesByArray, bool);
  virtual void SetVertexColorArrayName(const char* name);
  virtual const char* GetVertexColorArrayName();

  // ------------------------------------------------------------------------
  // Edge colors

  virtual void SetColorEdgesByArray(bool b);
  virtual bool GetColorEdgesByArray();
  vtkBooleanMacro(ColorEdgesByArray, bool);
  virtual void SetEdgeColorArrayName(const char* name);
  virtual const char* GetEdgeColorArrayName();

  // ------------------------------------------------------------------------
  // Enabled vertices

  virtual void SetEnableVerticesByArray(bool b);
  virtual bool GetEnableVerticesByArray();
  vtkBooleanMacro(EnableVerticesByArray, bool);
  virtual void SetEnabledVerticesArrayName(const char* name);
  virtual const char* GetEnabledVerticesArrayName();

  // ------------------------------------------------------------------------
  // Enabled edges

  virtual void SetEnableEdgesByArray(bool b);
  virtual bool GetEnableEdgesByArray();
  vtkBooleanMacro(EnableEdgesByArray, bool);
  virtual void SetEnabledEdgesArrayName(const char* name);
  virtual const char* GetEnabledEdgesArrayName();

  virtual void SetEdgeVisibility(bool b);
  virtual bool GetEdgeVisibility();
  vtkBooleanMacro(EdgeVisibility, bool);

  // ------------------------------------------------------------------------
  // Vertex layout strategy

  // Description:
  // Set/get the graph layout strategy.
  virtual void SetLayoutStrategy(vtkGraphLayoutStrategy* strategy);
  virtual vtkGraphLayoutStrategy* GetLayoutStrategy();

  // Description:
  // Get/set the layout strategy by name.
  virtual void SetLayoutStrategy(const char* name);
  vtkGetStringMacro(LayoutStrategyName);

  // Description:
  // Set predefined layout strategies.
  void SetLayoutStrategyToRandom()
    { this->SetLayoutStrategy("Random"); }
  void SetLayoutStrategyToForceDirected()
    { this->SetLayoutStrategy("Force Directed"); }
  void SetLayoutStrategyToSimple2D()
    { this->SetLayoutStrategy("Simple 2D"); }
  void SetLayoutStrategyToClustering2D()
    { this->SetLayoutStrategy("Clustering 2D"); }
  void SetLayoutStrategyToCommunity2D()
    { this->SetLayoutStrategy("Community 2D"); }
  void SetLayoutStrategyToFast2D()
    { this->SetLayoutStrategy("Fast 2D"); }
  void SetLayoutStrategyToPassThrough()
    { this->SetLayoutStrategy("Pass Through"); }
  void SetLayoutStrategyToCircular()
    { this->SetLayoutStrategy("Circular"); }
  void SetLayoutStrategyToTree()
    { this->SetLayoutStrategy("Tree"); }
  void SetLayoutStrategyToCosmicTree()
    { this->SetLayoutStrategy("Cosmic Tree"); }
  void SetLayoutStrategyToCone()
    { this->SetLayoutStrategy("Cone"); }
  void SetLayoutStrategyToSpanTree()
    { this->SetLayoutStrategy("Span Tree"); }

  // Description:
  // Set the layout strategy to use coordinates from arrays.
  // The x array must be specified. The y and z arrays are optional.
  virtual void SetLayoutStrategyToAssignCoordinates(
    const char* xarr, const char* yarr = 0, const char* zarr = 0);

  // Description:
  // Set the layout strategy to a tree layout. Radial indicates whether to
  // do a radial or standard top-down tree layout. The angle parameter is the
  // angular distance spanned by the tree. Leaf spacing is a
  // value from 0 to 1 indicating how much of the radial layout should be
  // allocated to leaf nodes (as opposed to between tree branches). The log spacing value is a
  // non-negative value where > 1 will create expanding levels, < 1 will create
  // contracting levels, and = 1 makes all levels the same size. See
  // vtkTreeLayoutStrategy for more information.
  virtual void SetLayoutStrategyToTree(
    bool radial,
    double angle = 90,
    double leafSpacing = 0.9,
    double logSpacing = 1.0);

  // Description:
  // Set the layout strategy to a cosmic tree layout. nodeSizeArrayName is
  // the array used to size the circles (default is NULL, which makes leaf
  // nodes the same size). sizeLeafNodesOnly only uses the leaf node sizes,
  // and computes the parent size as the sum of the child sizes (default true).
  // layoutDepth stops layout at a certain depth (default is 0, which does the
  // entire tree). layoutRoot is the vertex that will be considered the root
  // node of the layout (default is -1, which will use the tree's root).
  // See vtkCosmicTreeLayoutStrategy for more information.
  virtual void SetLayoutStrategyToCosmicTree(
    const char* nodeSizeArrayName,
    bool sizeLeafNodesOnly = true,
    int layoutDepth = 0,
    vtkIdType layoutRoot = -1);

  // ------------------------------------------------------------------------
  // Edge layout strategy

  // Description:
  // Set/get the graph layout strategy.
  virtual void SetEdgeLayoutStrategy(vtkEdgeLayoutStrategy* strategy);
  virtual vtkEdgeLayoutStrategy* GetEdgeLayoutStrategy();
  void SetEdgeLayoutStrategyToArcParallel()
    { this->SetEdgeLayoutStrategy("Arc Parallel"); }
  void SetEdgeLayoutStrategyToPassThrough()
    { this->SetEdgeLayoutStrategy("Pass Through"); }

  // Description:
  // Set the edge layout strategy to a geospatial arced strategy
  // appropriate for vtkGeoView.
  virtual void SetEdgeLayoutStrategyToGeo(double explodeFactor = 0.2);

  // Description:
  // Set the edge layout strategy by name.
  virtual void SetEdgeLayoutStrategy(const char* name);
  vtkGetStringMacro(EdgeLayoutStrategyName);

  // ------------------------------------------------------------------------
  // Miscellaneous

  // Description:
  // Apply a theme to this representation.
  virtual void ApplyViewTheme(vtkViewTheme* theme);

  // Description:
  // Set the graph vertex glyph type.
  virtual void SetGlyphType(int type);
  virtual int GetGlyphType();

  // Description:
  // Set whether to scale vertex glyphs.
  virtual void SetScaling(bool b);
  virtual bool GetScaling();
  vtkBooleanMacro(Scaling, bool);

  // Description:
  // Set the glyph scaling array name.
  virtual void SetScalingArrayName(const char* name);
  virtual const char* GetScalingArrayName();

  // Description:
  // Vertex/edge scalar bar visibility.
  virtual void SetVertexScalarBarVisibility(bool b);
  virtual bool GetVertexScalarBarVisibility();
  virtual void SetEdgeScalarBarVisibility(bool b);
  virtual bool GetEdgeScalarBarVisibility();

  // Description:
  // Whether the current graph layout is complete.
  virtual bool IsLayoutComplete();

  // Description:
  // Performs another iteration on the graph layout.
  virtual void UpdateLayout();

  // Description:
  // Compute the bounding box of the selected subgraph.
  void ComputeSelectedGraphBounds( double bounds[6] );

protected:
  vtkRenderedGraphRepresentation();
  ~vtkRenderedGraphRepresentation();

  // Description:
  // Called by the view to add/remove this representation.
  virtual bool AddToView(vtkView* view);
  virtual bool RemoveFromView(vtkView* view);

  virtual void PrepareForRendering(vtkRenderView* view);
  
  virtual vtkSelection* ConvertSelection(vtkView* view, vtkSelection* sel);

  //BTX
  virtual vtkUnicodeString GetHoverTextInternal(vtkSelection* sel);
  //ETX

  // Description:
  // Connect inputs to internal pipeline.
  virtual int RequestData(
    vtkInformation* request,
    vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  //BTX
  // Description:
  // Internal filter classes.
  vtkSmartPointer<vtkApplyColors>          ApplyColors;
  vtkSmartPointer<vtkVertexDegree>         VertexDegree;
  vtkSmartPointer<vtkPolyData>             EmptyPolyData;
  vtkSmartPointer<vtkEdgeCenters>          EdgeCenters;
  vtkSmartPointer<vtkGraphToPoints>        GraphToPoints;
  vtkSmartPointer<vtkPointSetToLabelHierarchy> VertexLabelHierarchy;
  vtkSmartPointer<vtkPointSetToLabelHierarchy> EdgeLabelHierarchy;
  vtkSmartPointer<vtkGraphLayout>          Layout;
  vtkSmartPointer<vtkPerturbCoincidentVertices> Coincident;
  vtkSmartPointer<vtkEdgeLayout>           EdgeLayout;
  vtkSmartPointer<vtkGraphToPolyData>      GraphToPoly;
  vtkSmartPointer<vtkPolyDataMapper>       EdgeMapper;
  vtkSmartPointer<vtkActor>                EdgeActor;
  vtkSmartPointer<vtkGraphToGlyphs>        VertexGlyph;
  vtkSmartPointer<vtkPolyDataMapper>       VertexMapper;
  vtkSmartPointer<vtkActor>                VertexActor;
  vtkSmartPointer<vtkGraphToGlyphs>        OutlineGlyph;
  vtkSmartPointer<vtkPolyDataMapper>       OutlineMapper;
  vtkSmartPointer<vtkActor>                OutlineActor;
  vtkSmartPointer<vtkScalarBarWidget>      VertexScalarBar;
  vtkSmartPointer<vtkScalarBarWidget>      EdgeScalarBar;
  vtkSmartPointer<vtkRemoveHiddenData>     RemoveHiddenGraph;
  vtkSmartPointer<vtkApplyIcons>           ApplyVertexIcons;
  vtkSmartPointer<vtkGraphToPoints>        VertexIconPoints;
  vtkSmartPointer<vtkTransformCoordinateSystems> VertexIconTransform;
  vtkSmartPointer<vtkIconGlyphFilter>      VertexIconGlyph;
  vtkSmartPointer<vtkPolyDataMapper2D>     VertexIconMapper;
  vtkSmartPointer<vtkTexturedActor2D>      VertexIconActor;
  //ETX

  char* VertexHoverArrayName;
  char* EdgeHoverArrayName;

  vtkSetStringMacro(VertexColorArrayNameInternal);
  vtkGetStringMacro(VertexColorArrayNameInternal);
  char* VertexColorArrayNameInternal;

  vtkSetStringMacro(EdgeColorArrayNameInternal);
  vtkGetStringMacro(EdgeColorArrayNameInternal);
  char* EdgeColorArrayNameInternal;

  vtkSetStringMacro(ScalingArrayNameInternal);
  vtkGetStringMacro(ScalingArrayNameInternal);
  char* ScalingArrayNameInternal;

  vtkSetStringMacro(LayoutStrategyName);
  char* LayoutStrategyName;
  vtkSetStringMacro(EdgeLayoutStrategyName);
  char* EdgeLayoutStrategyName;
  bool HideVertexLabelsOnInteraction;
  bool HideEdgeLabelsOnInteraction;

private:
  vtkRenderedGraphRepresentation(const vtkRenderedGraphRepresentation&); // Not implemented
  void operator=(const vtkRenderedGraphRepresentation&);   // Not implemented
};

#endif

