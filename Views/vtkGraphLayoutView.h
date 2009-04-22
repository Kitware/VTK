/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphLayoutView.h

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
// .NAME vtkGraphLayoutView - Lays out and displays a graph
//
// .SECTION Description
// vtkGraphLayoutView performs graph layout and displays a vtkGraph.
// You may color and label the vertices and edges using fields in the graph.
// If coordinates are already assigned to the graph vertices in your graph,
// set the layout strategy to PassThrough in this view. The default layout
// is Fast2D which is fast but not that good, for better layout set the
// layout to Simple2D or ForceDirected. There are also tree and circle
// layout strategies. :)
// 
// .SEE ALSO
// vtkFast2DLayoutStrategy
// vtkSimple2DLayoutStrategy
// vtkForceDirectedLayoutStrategy
//
// .SECTION Thanks
// Thanks a bunch to the holographic unfolding pattern.

#ifndef __vtkGraphLayoutView_h
#define __vtkGraphLayoutView_h

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.
#include "vtkRenderView.h"

class vtkActor;
class vtkActor2D;
class vtkArcParallelEdgeStrategy;
class vtkCircularLayoutStrategy;
class vtkClustering2DLayoutStrategy;
class vtkConeLayoutStrategy;
class vtkCoordinate;
class vtkCommunity2DLayoutStrategy;
class vtkConstrained2DLayoutStrategy;
class vtkDynamic2DLabelMapper;
class vtkEdgeCenters;
class vtkEdgeLayout;
class vtkEdgeLayoutStrategy;
class vtkExtractSelectedGraph;
class vtkFast2DLayoutStrategy;
class vtkForceDirectedLayoutStrategy;
class vtkGraphLayout;
class vtkGraphLayoutStrategy;
class vtkGraphMapper;
class vtkGraphToPolyData;
class vtkKdTreeSelector;
class vtkLookupTable;
class vtkPassThroughEdgeStrategy;
class vtkPassThroughLayoutStrategy;
class vtkPerturbCoincidentVertices;
class vtkPolyDataMapper;
class vtkRandomLayoutStrategy;
class vtkScalarBarWidget;
class vtkSelectionLink;
class vtkSimple2DLayoutStrategy;
class vtkTexture;
class vtkVertexDegree;
class vtkVertexGlyphFilter;
class vtkViewTheme;
class vtkHardwareSelector;



class VTK_VIEWS_EXPORT vtkGraphLayoutView : public vtkRenderView
{
public:
  static vtkGraphLayoutView *New();
  vtkTypeRevisionMacro(vtkGraphLayoutView, vtkRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
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
  // Whether to color edges.  Default is off.
  void SetColorEdges(bool vis);
  bool GetColorEdges();
  void ColorEdgesOn();
  void ColorEdgesOff();

  // Description:
  // The array to use for coloring edges.
  void SetEnabledEdgesArrayName(const char* name);
  const char* GetEnabledEdgesArrayName();
  
  // Description:
  // Whether to color edges.  Default is off.
  void SetEnableEdgesByArray(bool vis);
  int GetEnableEdgesByArray();

  // Description:
  // The array to use for coloring vertices. 
  void SetEnabledVerticesArrayName(const char* name);
  const char* GetEnabledVerticesArrayName();
  
  // Description:
  // Whether to color vertices.  Default is off.
  void SetEnableVerticesByArray(bool vis);
  int GetEnableVerticesByArray();

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
  // The layout strategy to use when performing the graph layout.
  // The possible strings are:
  //  - "Random"         Randomly places vertices in a box.
  //  - "Force Directed" A layout in 3D or 2D simulating forces on edges.
  //  - "Simple 2D"      A simple 2D force directed layout.
  //  - "Clustering 2D"  A 2D force directed layout that's just like
  //                     simple 2D but uses some techniques to cluster better.
  //  - "Community 2D"   A linear-time 2D layout that's just like
  //                    Fast 2D but looks for and uses a community 
  //                    array to 'accentuate' clusters.
  //  - "Fast 2D"       A linear-time 2D layout.
  //  - "Pass Through"  Use locations assigned to the input.
  //  - "Circular"      Places vertices uniformly on a circle.
  //  - "Cone"          Places vertices using a conical tree strategy.
  // Default is "Simple 2D".
  void SetLayoutStrategy(const char* name);
  void SetLayoutStrategyToRandom()
    { this->SetLayoutStrategy("Random"); }
  void SetLayoutStrategyToCone()
    { this->SetLayoutStrategy("Cone"); }
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
  const char* GetLayoutStrategyName()
    { return this->GetLayoutStrategyNameInternal(); }

  // Description:
  // The layout strategy to use when performing the graph layout.
  // This signature allows an application to create a layout
  // object directly and simply set the pointer through this method.
  vtkGetObjectMacro(LayoutStrategy,vtkGraphLayoutStrategy);
  void SetLayoutStrategy(vtkGraphLayoutStrategy *s);

  // Description:
  // The layout strategy to use when performing the edge layout.
  // The possible strings are:
  //   "Arc Parallel"   - Arc parallel edges and self loops.
  //   "Pass Through"   - Use edge routes assigned to the input.
  // Default is "Arc Parallel".
  void SetEdgeLayoutStrategy(const char* name);
  void SetEdgeLayoutStrategyToArcParallel()
    { this->SetEdgeLayoutStrategy("Arc Parallel"); }
  void SetEdgeLayoutStrategyToPassThrough()
    { this->SetEdgeLayoutStrategy("Pass Through"); }
  const char* GetEdgeLayoutStrategyName()
    { return this->GetEdgeLayoutStrategyNameInternal(); }

  // Description:
  // The layout strategy to use when performing the edge layout.
  // This signature allows an application to create a layout
  // object directly and simply set the pointer through this method.
  vtkGetObjectMacro(EdgeLayoutStrategy,vtkEdgeLayoutStrategy);
  void SetEdgeLayoutStrategy(vtkEdgeLayoutStrategy *s);

  // Description:
  // Set the number of iterations per refresh (defaults to all)
  // In other words, the default is to do the entire layout
  // and then do a visual refresh. Changing this variable
  // to something like '1', will enable an application to
  // see the layout as it progresses.
  void SetIterationsPerLayout(int iterations);
  
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

  // Description:
  // Whether the scalar bar for edges is visible.  Default is off.
  void SetEdgeScalarBarVisibility(bool vis);

  // Description:
  // Whether the scalar bar for vertices is visible.  Default is off.
  void SetVertexScalarBarVisibility(bool vis);

  // Description:
  // Reset the camera based on the bounds of the selected region.
  void ZoomToSelection();

  // Description:
  // Is the graph layout complete? This method is useful
  // for when the strategy is iterative and the application
  // wants to show the iterative progress of the graph layout
  // See Also: UpdateLayout();
  virtual int IsLayoutComplete();
  
  // Description:
  // This method is useful for when the strategy is iterative 
  // and the application wants to show the iterative progress 
  // of the graph layout. The application would have something like
  // while(!IsLayoutComplete())
  //   {
  //   UpdateLayout();
  //   }
  // See Also: IsLayoutComplete();
  virtual void UpdateLayout();

protected:
  vtkGraphLayoutView();
  ~vtkGraphLayoutView();

  // Description:
  // Called to process the user event from the interactor style.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);
  
  // Description:
  // Connects the algorithm output to the internal pipeline.
  // This view only supports a single representation.
  virtual void AddInputConnection( int port, int item,
    vtkAlgorithmOutput* conn,
    vtkAlgorithmOutput* selectionConn);
  
  // Description:
  // Removes the algorithm output from the internal pipeline.
  virtual void RemoveInputConnection( int port, int item,
    vtkAlgorithmOutput* conn,
    vtkAlgorithmOutput* selectionConn);
  
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
  // Used to store the layout strategy name
  vtkGetStringMacro(EdgeLayoutStrategyNameInternal);
  vtkSetStringMacro(EdgeLayoutStrategyNameInternal);
  char* EdgeLayoutStrategyNameInternal;
  
  // Description:
  // Used to store the current edge layout strategy
  vtkEdgeLayoutStrategy* EdgeLayoutStrategy;
  
  // Description:
  // Used to store the preferred edge layout strategy
  // when the graph layout has been been set to PassThrough
  // so that we can change it back when needed.
  vtkEdgeLayoutStrategy* EdgeLayoutPreference;

  // Description:
  // Used to store the icon array name
  vtkGetStringMacro(IconArrayNameInternal);
  vtkSetStringMacro(IconArrayNameInternal);
  char* IconArrayNameInternal;
  
  //BTX
  // Used for coordinate conversion
  vtkSmartPointer<vtkCoordinate>                   Coordinate;

  // Representation objects
  vtkSmartPointer<vtkGraphLayout>                  GraphLayout;
  vtkSmartPointer<vtkRandomLayoutStrategy>         RandomStrategy;
  vtkSmartPointer<vtkConeLayoutStrategy>           ConeStrategy;
  vtkSmartPointer<vtkForceDirectedLayoutStrategy>  ForceDirectedStrategy;
  vtkSmartPointer<vtkSimple2DLayoutStrategy>       Simple2DStrategy;
  vtkSmartPointer<vtkClustering2DLayoutStrategy>   Clustering2DStrategy;
  vtkSmartPointer<vtkCommunity2DLayoutStrategy>    Community2DStrategy;
  vtkSmartPointer<vtkConstrained2DLayoutStrategy>  Constrained2DStrategy;
  vtkSmartPointer<vtkFast2DLayoutStrategy>         Fast2DStrategy;
  vtkSmartPointer<vtkPassThroughLayoutStrategy>    PassThroughStrategy;
  vtkSmartPointer<vtkCircularLayoutStrategy>       CircularStrategy;
  vtkSmartPointer<vtkEdgeLayout>                   EdgeLayout;
  vtkSmartPointer<vtkArcParallelEdgeStrategy>      ArcParallelStrategy;
  vtkSmartPointer<vtkPassThroughEdgeStrategy>      PassThroughEdgeStrategy;
  vtkSmartPointer<vtkPerturbCoincidentVertices>    PerturbCoincidentVertices;
  vtkSmartPointer<vtkVertexDegree>                 VertexDegree;
  vtkSmartPointer<vtkEdgeCenters>                  EdgeCenters;
  vtkSmartPointer<vtkActor>                        GraphActor;
  vtkSmartPointer<vtkGraphMapper>                  GraphMapper;
  vtkSmartPointer<vtkDynamic2DLabelMapper>         VertexLabelMapper;
  vtkSmartPointer<vtkActor2D>                      VertexLabelActor;
  vtkSmartPointer<vtkDynamic2DLabelMapper>         EdgeLabelMapper;
  vtkSmartPointer<vtkActor2D>                      EdgeLabelActor;
  vtkSmartPointer<vtkScalarBarWidget>              VertexScalarBar;
  vtkSmartPointer<vtkScalarBarWidget>              EdgeScalarBar;
  
  // Selection objects
  vtkSmartPointer<vtkKdTreeSelector>               KdTreeSelector;
  vtkSmartPointer<vtkHardwareSelector>             HardwareSelector;
  vtkSmartPointer<vtkExtractSelectedGraph>         ExtractSelectedGraph;
  vtkSmartPointer<vtkActor>                        SelectedGraphActor;
  vtkSmartPointer<vtkGraphMapper>                  SelectedGraphMapper;

  // Actor for edge selection
  vtkSmartPointer<vtkGraphToPolyData>              EdgeSelectionPoly;
  vtkSmartPointer<vtkPolyDataMapper>               EdgeSelectionMapper;
  vtkSmartPointer<vtkActor>                        EdgeSelectionActor;
  //ETX

private:
  vtkGraphLayoutView(const vtkGraphLayoutView&);  // Not implemented.
  void operator=(const vtkGraphLayoutView&);  // Not implemented.
};

#endif
