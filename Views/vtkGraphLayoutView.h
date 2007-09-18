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
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkGraphLayoutView - Lays out and displays a graph
//
// .SECTION Description
// vtkGraphLayoutView performs graph layout and displays a vtkGraph.
// You may color and label the vertices and edges using fields in the graph.
// If coordinates are already assigned to the graph vertices in your graph,
// set the layout strategy to PassThrough in this view.
//
// .SECTION Thanks
// Thanks to Brian Wylie from Sandia National Laboratories for implementing
// the first version of this class.

#ifndef __vtkGraphLayoutView_h
#define __vtkGraphLayoutView_h

#include "vtkRenderView.h"

class vtkActor;
class vtkActor2D;
class vtkCircularLayoutStrategy;
class vtkCoordinate;
class vtkDynamic2DLabelMapper;
class vtkExtractSelectedGraph;
class vtkFast2DLayoutStrategy;
class vtkForceDirectedLayoutStrategy;
class vtkGraphLayout;
class vtkGraphToPolyData;
class vtkKdTreeSelector;
class vtkLookupTable;
class vtkPassThroughLayoutStrategy;
class vtkPolyDataMapper;
class vtkRandomLayoutStrategy;
class vtkSelectionLink;
class vtkSimple2DLayoutStrategy;
class vtkClustering2DLayoutStrategy;
class vtkCommunity2DLayoutStrategy;
class vtkVertexDegree;
class vtkCellCenters;
class vtkVertexGlyphFilter;
class vtkViewTheme;
class vtkVisibleCellSelector;

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
  // Whether to show labels.  Default is off.
  void SetColorVertices(bool vis);
  bool GetColorVertices();
  void ColorVerticesOn();
  void ColorVerticesOff();
  
  // Description:
  // The array to use for coloring edges.  Default is "color".
  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();
  
  // Description:
  // Whether to show labels.  Default is off.
  void SetColorEdges(bool vis);
  bool GetColorEdges();
  void ColorEdgesOn();
  void ColorEdgesOff();
  
  // Description:
  // The layout strategy to use when performing the graph layout.
  // The possible strings are:
  //   "Random"         - Randomly places vertices in a box.
  //   "Force Directed" - A layout in 3D or 2D simulating forces on edges.
  //   "Simple 2D"      - A simple 2D force directed layout.
  //   "Clustering 2D"  - A 2D force directed layout that's just like
  //                    - simple 2D but uses some techniques to cluster better.
  //   "Fast 2D"        - A linear-time 2D layout.
  //   "Pass Through"   - Use locations assigned to the input.
  //   "Circular"       - Places vertices uniformly on a circle.
  // Default is "Simple 2D".
  void SetLayoutStrategy(const char* name);
  void SetLayoutStrategyToRandom()        { this->SetLayoutStrategy("Random"); }
  void SetLayoutStrategyToForceDirected() { this->SetLayoutStrategy("Force Directed"); }
  void SetLayoutStrategyToSimple2D()      { this->SetLayoutStrategy("Simple 2D"); }
  void SetLayoutStrategyToClustering2D()  { this->SetLayoutStrategy("Cluster 2D"); }
  void SetLayoutStrategyToCommunity2D()   { this->SetLayoutStrategy("Community 2D"); }
  void SetLayoutStrategyToFast2D()        { this->SetLayoutStrategy("Fast 2D"); }
  void SetLayoutStrategyToPassThrough()   { this->SetLayoutStrategy("Pass Through"); }
  void SetLayoutStrategyToCircular()      { this->SetLayoutStrategy("Circular"); }
  const char* GetLayoutStrategy()         { return this->GetLayoutStrategyInternal(); }

  // Description:
  // Set the number of iterations per refresh (defaults to all)
  // In other words, the default is to do the entire layout
  // and then do a visual refresh. Changing this variable
  // to something like '1', will enable an application to
  // see the layout as it progresses.
  void SetIterationsPerLayout(int iterations);
  
  
  // Description:
  // The array used for populating the selection list
  void SetSelectionArrayName(const char* name);
  const char* GetSelectionArrayName();

  // Description:
  // Sets up interactor style.
  virtual void SetupRenderWindow(vtkRenderWindow* win);
  
  // Description:
  // Apply the theme to this view.
  virtual void ApplyViewTheme(vtkViewTheme* theme);

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
  virtual void AddInputConnection(vtkAlgorithmOutput* conn);
  
  // Description:
  // Removes the algorithm output from the internal pipeline.
  virtual void RemoveInputConnection(vtkAlgorithmOutput* conn);
  
  // Description:
  // Connects the selection link to the internal pipeline.
  virtual void SetSelectionLink(vtkSelectionLink* link);
  
  // Decsription:
  // Prepares the view for rendering.
  virtual void PrepareForRendering();

  // Description:
  // May a display coordinate to a world coordinate on the x-y plane.  
  void MapToXYPlane(double displayX, double displayY, double &x, double &y);
  
  // Description:
  // Used to store the vertex and edge color array names
  vtkGetStringMacro(VertexColorArrayNameInternal);
  vtkSetStringMacro(VertexColorArrayNameInternal);
  vtkGetStringMacro(EdgeColorArrayNameInternal);
  vtkSetStringMacro(EdgeColorArrayNameInternal);
  char* VertexColorArrayNameInternal;
  char* EdgeColorArrayNameInternal;

  // Description:
  // Used to store the layout strategy name
  vtkGetStringMacro(LayoutStrategyInternal);
  vtkSetStringMacro(LayoutStrategyInternal);
  char* LayoutStrategyInternal;
  
  // Description:
  // Used to store the selection array name
  vtkGetStringMacro(SelectionArrayNameInternal);
  vtkSetStringMacro(SelectionArrayNameInternal);
  char* SelectionArrayNameInternal;
  
  // Used for coordinate conversion
  vtkCoordinate* Coordinate;

  // Representation objects
  vtkGraphLayout*                  GraphLayout;
  vtkRandomLayoutStrategy*         RandomStrategy;
  vtkForceDirectedLayoutStrategy*  ForceDirectedStrategy;
  vtkSimple2DLayoutStrategy*       Simple2DStrategy;
  vtkClustering2DLayoutStrategy*   Clustering2DStrategy;
  vtkCommunity2DLayoutStrategy*    Community2DStrategy;
  vtkFast2DLayoutStrategy*         Fast2DStrategy;
  vtkPassThroughLayoutStrategy*    PassThroughStrategy;
  vtkCircularLayoutStrategy*       CircularStrategy;
  vtkVertexDegree*                 VertexDegree;
  vtkCellCenters*                  CellCenters;
  vtkGraphToPolyData*              GraphToPolyData;
  vtkVertexGlyphFilter*            VertexGlyph;
  vtkPolyDataMapper*               VertexMapper;
  vtkLookupTable*                  VertexColorLUT;
  vtkActor*                        VertexActor;
  vtkPolyDataMapper*               OutlineMapper;
  vtkActor*                        OutlineActor;
  vtkPolyDataMapper*               EdgeMapper;
  vtkLookupTable*                  EdgeColorLUT;
  vtkActor*                        EdgeActor;
  vtkDynamic2DLabelMapper*         VertexLabelMapper;
  vtkActor2D*                      VertexLabelActor;
  vtkDynamic2DLabelMapper*         EdgeLabelMapper;
  vtkActor2D*                      EdgeLabelActor;
  
  // Selection objects
  vtkKdTreeSelector*        KdTreeSelector;
  vtkVisibleCellSelector*   VisibleCellSelector;
  vtkExtractSelectedGraph*  ExtractSelectedGraph;
  vtkGraphToPolyData*       SelectionToPolyData;
  vtkVertexGlyphFilter*     SelectionVertexGlyph;
  vtkPolyDataMapper*        SelectionVertexMapper;
  vtkActor*                 SelectionVertexActor;
  vtkPolyDataMapper*        SelectionEdgeMapper;
  vtkActor*                 SelectionEdgeActor;

private:
  vtkGraphLayoutView(const vtkGraphLayoutView&);  // Not implemented.
  void operator=(const vtkGraphLayoutView&);  // Not implemented.
};

#endif
