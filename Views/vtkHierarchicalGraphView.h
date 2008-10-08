/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalGraphView.h
  
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

// .NAME vtkHierarchicalGraphView - Accepts a graph and a hierarchy - currently
// a tree - and provides a hierarchy-aware display.  Currently, this means
// displaying the hierarchy using a tree layout, then rendering the graph
// vertices as leaves of the tree with curved graph edges between leaves.
//
// .SECTION Description
// Takes a graph and a hierarchy (currently a tree) and lays out the graph 
// vertices based on their categorization within the hierarchy.
//
// .SEE ALSO
// vtkTreeLayoutView
// vtkGraphLayoutView
//
// .SECTION Thanks
// Thanks to the turtle with jets for feet, without you this class wouldn't 
// have been possible.

#ifndef __vtkHierarchicalGraphView_h
#define __vtkHierarchicalGraphView_h

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.
#include "vtkRenderView.h"

class vtkActor;
class vtkActor2D;
class vtkAlgorithmOutput;
class vtkCircularLayoutStrategy;
class vtkCoordinate;
class vtkCosmicTreeLayoutStrategy;
class vtkTreeLayoutStrategy;
class vtkDynamic2DLabelMapper;
class vtkEdgeCenters;
class vtkExtractSelectedGraph;
class vtkGlyph3D;
class vtkGraph;
class vtkGraphHierarchicalBundle;
class vtkGraphLayout;
class vtkGraphLayoutStrategy;
class vtkGraphMapper;
class vtkGraphToPolyData;
class vtkGraphToTree;
class vtkKdTreeSelector;
class vtkLookupTable;
class vtkPassThroughLayoutStrategy;
class vtkPolyDataMapper;
class vtkSelection;
class vtkSelectionLink;
class vtkSplineFilter;
class vtkTexture;
class vtkTree;
class vtkTreeFieldAggregator;
class vtkVertexDegree;
class vtkVertexGlyphFilter;
class vtkViewTheme;
class vtkHardwareSelector;


class VTK_VIEWS_EXPORT vtkHierarchicalGraphView : public vtkRenderView
{
public:
  static vtkHierarchicalGraphView *New();
  vtkTypeRevisionMacro(vtkHierarchicalGraphView, vtkRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Set the tree and graph representations to the appropriate input ports.
  vtkDataRepresentation* SetHierarchyFromInputConnection(vtkAlgorithmOutput* conn);
  vtkDataRepresentation* SetHierarchyFromInput(vtkDataObject* input);
  vtkDataRepresentation* SetGraphFromInputConnection(vtkAlgorithmOutput* conn);
  vtkDataRepresentation* SetGraphFromInput(vtkDataObject* input);
  
  // Description:
  // The array to use for vertex labeling.  Default is "label".
  void SetVertexLabelArrayName(const char* name);
  const char* GetVertexLabelArrayName();
  
  // Description:
  // The array to use for edge labeling.  Default is "label".
  void SetEdgeLabelArrayName(const char* name);
  const char* GetEdgeLabelArrayName();
  
  // Description:
  // Whether to show vertex labels.  Default is off.
  void SetVertexLabelVisibility(bool vis);
  bool GetVertexLabelVisibility();
  void VertexLabelVisibilityOn();
  void VertexLabelVisibilityOff();
  
  // Description:
  // Whether to show edge labels.  Default is off.
  void SetEdgeLabelVisibility(bool vis);
  bool GetEdgeLabelVisibility();
  void EdgeLabelVisibilityOn();
  void EdgeLabelVisibilityOff();
  
  // Description:
  // The array to use for coloring vertices.  Default is "color".
  void SetVertexColorArrayName(const char* name);
  const char* GetVertexColorArrayName();
  
  // Description:
  // Whether to color vertices.  Default is off.
  void SetColorVertices(bool vis);
  bool GetColorVertices();
  void ColorVerticesOn();
  void ColorVerticesOff();
  
  // Description:
  // The array to use for coloring edges.  Default is "color".
  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();
  
  // Description:
  // Set the color to be the spline fraction
  void SetEdgeColorToSplineFraction();
  
  // Description:
  // Whether to color edges.  Default is off.
  void SetColorEdges(bool vis);
  bool GetColorEdges();
  void ColorEdgesOn();
  void ColorEdgesOff();
  
  // Description:
  // Set whether to use radial layout.
  void SetRadialLayout(bool radial);
  
  // Description:
  // Set the radial layout angle.
  void SetRadialAngle(int angle);
  
  // Description:
  // Set the log spacing for the layout.
  void SetLogSpacingFactor(double spacing);
  
  // Description:
  // Set the leaf spacing for the layout.
  void SetLeafSpacing(double spacing);
  
  // Description:
  // Set the bundling strength.
  void SetBundlingStrength(double strength);
   
  // Description:
  // Retrieve a special representation whose selection link controls
  // what tree nodes are visible. After extracting the selection,
  // the resulting graph must be a tree or an error will result.
  virtual vtkDataRepresentation* GetTreeVisibilityRepresentation();

  // Description:
  // Retrieve the graph and tree representations.
  virtual vtkDataRepresentation* GetGraphRepresentation()
    { return this->GetRepresentation(1, 0); }
  virtual vtkDataRepresentation* GetTreeRepresentation()
    { return this->GetRepresentation(0, 0); }

  // Description:
  // The icon sheet to use for textures.
  void SetIconTexture(vtkTexture *texture);

  // Description: 
  // Associate the icon at index "index" in the vtkTexture to all vertices
  // containing "type" as a value in the vertex attribute array specified by
  // IconArrayName.
  void AddIconType(char *type, int index);

  // Description:
  // Clear all icon mappings.
  void ClearIconTypes();

  // Description:
  // Each icon's size on the sheet.
  void SetIconSize(int *size);

  // Description:
  // Specify where the icons should be placed in relation to the vertex.
  // See vtkIconGlyphFilter.h for possible values.
  void SetIconAlignment(int alignment);

  // Description:
  // Whether icons are visible (default off).
  void SetIconVisibility(bool b);
  bool GetIconVisibility();
  vtkBooleanMacro(IconVisibility, bool);

  // Description:
  // The array used for assigning icons
  void SetIconArrayName(const char* name);
  const char* GetIconArrayName();
  
  // Description:
  // Whether the TREE edges are visible (default off).
  void SetTreeEdgeVisibility(bool b);
  bool GetTreeEdgeVisibility();
  void TreeEdgeVisibilityOn();
  void TreeEdgeVisibilityOff();
  
  // Description:
  // The array used for scaling (if ScaledGlyphs is ON)
  void SetScalingArrayName(const char* name);
  const char* GetScalingArrayName();
  
  // Description:
  // Whether to use scaled glyphs or not.  Default is off.
  void SetScaledGlyphs(bool arg);
  bool GetScaledGlyphs();
  vtkBooleanMacro(ScaledGlyphs, bool);
  
  // Description:
  // The layout strategy to use when performing the tree layout.
  // The possible strings are:
  //   "Tree"           - The regular tree layout.
  //   "CosmicTree"      - A tree layout using orbits (not well tested)
  // Default is "Tree".
  void SetLayoutStrategy(const char* name);
  void SetLayoutStrategyToTree()          { this->SetLayoutStrategy("Tree"); }
  void SetLayoutStrategyToCosmicTree()    { this->SetLayoutStrategy("CosmicTree"); }
  const char* GetLayoutStrategyName()     { return this->GetLayoutStrategyNameInternal(); }

  // Description:
  // The layout strategy to use when performing the graph layout.
  // This signature allows an application to create a layout
  // object directly and simply set the pointer through this method
  vtkGetObjectMacro(LayoutStrategy,vtkGraphLayoutStrategy);
  void SetLayoutStrategy(vtkGraphLayoutStrategy *s);
  
  // Description:
  // Sets up interactor style.
  virtual void SetupRenderWindow(vtkRenderWindow* win);
  
  // Description:
  // Apply the theme to this view.
  virtual void ApplyViewTheme(vtkViewTheme* theme);
  
  // Description:
  // The size of the font used for vertex labeling
  virtual void SetVertexLabelFontSize(const int size);
  virtual int GetVertexLabelFontSize();
  
  // Description:
  // The size of the font used for edge labeling
  virtual void SetEdgeLabelFontSize(const int size);
  virtual int GetEdgeLabelFontSize();

protected:
  vtkHierarchicalGraphView();
  ~vtkHierarchicalGraphView();

  // Description:
  // Called to process the user event from the interactor style.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);
  
  // Description:
  // Connects the algorithm output to the internal pipelines.
  virtual void AddInputConnection( int port, int item,
    vtkAlgorithmOutput* conn, vtkAlgorithmOutput* selectConn);

  // Description:
  // Removes the algorithm output from the internal pipeline.
  virtual void RemoveInputConnection( int port, int item,
    vtkAlgorithmOutput* conn, vtkAlgorithmOutput* selectConn);
  
  // Decsription:
  // Prepares the view for rendering.
  virtual void PrepareForRendering();

  // Description:
  // May a display coordinate to a world coordinate on the x-y plane.  
  void MapToXYPlane(double displayX, double displayY, double &x, double &y);
  
  // Description:
  // Used to store the layout strategy name
  vtkGetStringMacro(LayoutStrategyNameInternal);
  vtkSetStringMacro(LayoutStrategyNameInternal);
  char* LayoutStrategyNameInternal;
  
  // Description:
  // Used to store the current layout strategy
  vtkGraphLayoutStrategy* LayoutStrategy;
  
  // Description:
  // Used to store the icon array name
  vtkGetStringMacro(IconArrayNameInternal);
  vtkSetStringMacro(IconArrayNameInternal);
  char* IconArrayNameInternal;
  //BTX
  
  // Processing objects
  vtkSmartPointer<vtkCoordinate>                   Coordinate;
  vtkSmartPointer<vtkGraphLayout>                  GraphLayout;
  vtkSmartPointer<vtkCircularLayoutStrategy>       CircularStrategy;
  vtkSmartPointer<vtkTreeLayoutStrategy>           TreeStrategy;
  vtkSmartPointer<vtkCosmicTreeLayoutStrategy>     CosmicTreeStrategy;
  vtkSmartPointer<vtkPassThroughLayoutStrategy>    PassThroughStrategy;
  vtkSmartPointer<vtkGraphHierarchicalBundle>      HBundle;
  vtkSmartPointer<vtkSplineFilter>                 Spline;
  vtkSmartPointer<vtkVertexDegree>                 VertexDegree;
  vtkSmartPointer<vtkEdgeCenters>                  EdgeCenters;
  vtkSmartPointer<vtkTreeFieldAggregator>          TreeAggregation;
  vtkSmartPointer<vtkExtractSelectedGraph>         ExtractSelectedTree;
  vtkSmartPointer<vtkGraphMapper>                  SelectedTreeMapper;
  vtkSmartPointer<vtkActor>                        SelectedTreeActor;
  
  // Representation objects
  vtkSmartPointer<vtkActor>                        TreeActor;
  vtkSmartPointer<vtkGraphMapper>                  TreeMapper;
  vtkSmartPointer<vtkDynamic2DLabelMapper>         VertexLabelMapper;
  vtkSmartPointer<vtkActor2D>                      VertexLabelActor;
  vtkSmartPointer<vtkDynamic2DLabelMapper>         EdgeLabelMapper;
  vtkSmartPointer<vtkActor2D>                      EdgeLabelActor;
  vtkSmartPointer<vtkPolyDataMapper>               GraphEdgeMapper;
  vtkSmartPointer<vtkActor>                        GraphEdgeActor;
  vtkSmartPointer<vtkDataRepresentation>           TreeVisibilityRepresentation;
  
  // Selection objects
  vtkSmartPointer<vtkSelection>                    EmptySelection;
  vtkSmartPointer<vtkKdTreeSelector>               KdTreeSelector;
  vtkSmartPointer<vtkHardwareSelector>             HardwareSelector;
  vtkSmartPointer<vtkExtractSelectedGraph>         ExtractSelectedGraph;
  vtkSmartPointer<vtkGraphHierarchicalBundle>      SelectedGraphHBundle;
  vtkSmartPointer<vtkSplineFilter>                 SelectedGraphSpline;
  vtkSmartPointer<vtkActor>                        SelectedGraphActor;
  vtkSmartPointer<vtkPolyDataMapper>               SelectedGraphMapper;
  //ETX

private:

  // Description:
  // Whether to use radial layout.
  bool Radial;
  
  // Description:
  // The radial layout angle.
  int Angle;
  
  // Description:
  // The log spacing for the layout.
  float LogSpacing;
  
  // Description:
  // This determines if you have 'gaps'
  // between the leaves of different parents.
  float LeafSpacing;
  
  // Description:
  // The bundling strength.
  float BundlingStrength;

  // Description:
  // The indices of the graph and tree representations.
  int GraphRepresentationIndex;
  int TreeRepresentationIndex;
  
  vtkHierarchicalGraphView(const vtkHierarchicalGraphView&);  // Not implemented.
  void operator=(const vtkHierarchicalGraphView&);  // Not implemented.
};

#endif
