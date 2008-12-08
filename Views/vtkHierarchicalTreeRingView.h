/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalTreeRingView.h
  
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

// .NAME vtkHierarchicalTreeRingView - Accepts a graph and a hierarchy - currently
// a tree - and provides a hierarchy-aware display.  Currently, this means
// displaying the hierarchy using a tree ring layout, then rendering the graph
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
// Thanks to Jason Shepherd for implementing this class

#ifndef __vtkHierarchicalTreeRingView_h
#define __vtkHierarchicalTreeRingView_h

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.
#include "vtkRenderView.h"

class vtkActor;
class vtkActor2D;
class vtkAlgorithmOutput;
class vtkCoordinate;
class vtkDynamic2DLabelMapper;
class vtkEdgeCenters;
class vtkExtractSelectedGraph;
class vtkGraph;
class vtkGraphHierarchicalBundle;
class vtkGraphMapper;
class vtkTransferAttributes;
class vtkKdTreeSelector;
class vtkLookupTable;
class vtkPolyDataMapper;
class vtkSelection;
class vtkSelectionLink;
class vtkSplineFilter;
class vtkTexture;
class vtkTree;
class vtkTreeFieldAggregator;
class vtkVertexDegree;
class vtkViewTheme;
class vtkHardwareSelector;
class vtkTreeRingLayout;
class vtkTreeRingPointLayout;
class vtkTreeRingReversedLayoutStrategy;
class vtkTreeRingToPolyData;

class VTK_VIEWS_EXPORT vtkHierarchicalTreeRingView : public vtkRenderView
{
public:
  static vtkHierarchicalTreeRingView *New();
  vtkTypeRevisionMacro(vtkHierarchicalTreeRingView, vtkRenderView);
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
  // The array to use for vertex labeling priority.  Default is "VertexDegree".
  void SetLabelPriorityArrayName(const char* name);

  // Description:
  // The array to use for edge labeling.  Default is "label".
  void SetEdgeLabelArrayName(const char* name);
  const char* GetEdgeLabelArrayName();

  // Description:
  // The name of the array whose value appears when the mouse hovers
  // over a rectangle in the treemap.
  // This must be a string array.
  void SetHoverArrayName(const char* name);
  const char* GetHoverArrayName();

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
//  const char* GetVertexColorArrayName();

  // Description:
  // Set the log spacing for the interior point layout.
  void SetInteriorLogSpacingFactor(double spacing);

  // Description:
  // The array to use for coloring edges.  Default is "color".
  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();
  
  // Description:
  // Set the color to be the spline fraction
  void SetEdgeColorToSplineFraction();

  // Description:
  // Set the shrink percentage on each of the sectors
  void SetSectorShrinkFactor(double value);

  // Description:
  // Whether to color edges.  Default is off.
  void SetColorEdges(bool vis);
  bool GetColorEdges();
  void ColorEdgesOn();
  void ColorEdgesOff();
  
  // Description:
  // Set the bundling strength.
  void SetBundlingStrength(double strength);

  // Description:
  // Set the root angles for laying out the hierarchy
  void SetRootAngles( double start, double end );
 
  // Description:
  // Retrieve the graph and tree representations.
  virtual vtkDataRepresentation* GetGraphRepresentation()
    { return this->GetRepresentation(1, 0); }
  virtual vtkDataRepresentation* GetTreeRepresentation()
    { return this->GetRepresentation(0, 0); }

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
  vtkHierarchicalTreeRingView();
  ~vtkHierarchicalTreeRingView();

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
  
  //BTX
  // Processing objects
  vtkSmartPointer<vtkCoordinate>                   Coordinate;
  vtkSmartPointer<vtkGraphHierarchicalBundle>      HBundle;
  vtkSmartPointer<vtkSplineFilter>                 Spline;
  vtkSmartPointer<vtkVertexDegree>                 VertexDegree;
  vtkSmartPointer<vtkVertexDegree>                 GraphVertexDegree;
  vtkSmartPointer<vtkEdgeCenters>                  EdgeCenters;
  vtkSmartPointer<vtkTreeFieldAggregator>          TreeAggregation;
  vtkSmartPointer<vtkTransferAttributes>           TransferAttributes;
  
  // Representation objects
  vtkSmartPointer<vtkDynamic2DLabelMapper>         EdgeLabelMapper;
  vtkSmartPointer<vtkActor2D>                      EdgeLabelActor;
  vtkSmartPointer<vtkPolyDataMapper>               GraphEdgeMapper;
  vtkSmartPointer<vtkActor>                        GraphEdgeActor;
  vtkSmartPointer<vtkDataRepresentation>           TreeVisibilityRepresentation;
  
  // TreeRing objects
  vtkSmartPointer<vtkTreeRingLayout>               TreeRingLayout;
  vtkSmartPointer<vtkTreeRingReversedLayoutStrategy> TreeRingLayoutStrategy;
  vtkSmartPointer<vtkTreeRingPointLayout>          TreeRingPointLayout;
  vtkSmartPointer<vtkTreeRingToPolyData>           TreeRingMapper;
  vtkSmartPointer<vtkPolyDataMapper>               TreeRingMapper2;
  vtkSmartPointer<vtkActor>                        TreeRingActor;
  vtkSmartPointer<vtkDynamic2DLabelMapper>         TreeRingLabelMapper;
  vtkSmartPointer<vtkActor2D>                      TreeRingLabelActor;
  
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
  // The bundling strength.
  float BundlingStrength;
  double InteriorLogSpacing;

  // Description:
  // The indices of the graph and tree representations.
  int GraphRepresentationIndex;
  int TreeRepresentationIndex;
  
  vtkHierarchicalTreeRingView(const vtkHierarchicalTreeRingView&);  // Not implemented.
  void operator=(const vtkHierarchicalTreeRingView&);  // Not implemented.
};

#endif
