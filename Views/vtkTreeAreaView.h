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
// vtkTreeLayoutView
// vtkGraphLayoutView
//
// .SECTION Thanks
// Thanks to Jason Shepherd for implementing this class

#ifndef __vtkTreeAreaView_h
#define __vtkTreeAreaView_h

#include "vtkSmartPointer.h"    // Required for smart pointer internal ivars.
#include "vtkRenderView.h"

class vtkActor;
class vtkActor2D;
class vtkAlgorithmOutput;
class vtkAreaLayout;
class vtkAreaLayoutStrategy;
class vtkConvertSelection;
class vtkCoordinate;
class vtkDynamic2DLabelMapper;
class vtkEdgeCenters;
class vtkExtractSelectedGraph;
class vtkExtractSelectedPolyDataIds;
class vtkGraph;
class vtkGraphHierarchicalBundle;
class vtkGraphMapper;
class vtkTransferAttributes;
class vtkKdTreeSelector;
class vtkLabeledDataMapper;
class vtkLookupTable;
class vtkPolyDataAlgorithm;
class vtkPolyDataMapper;
class vtkSelection;
class vtkSelectionLink;
class vtkSplineFilter;
class vtkTexture;
class vtkTree;
class vtkTreeFieldAggregator;
class vtkTreeLevelsFilter;
class vtkVertexDegree;
class vtkViewTheme;
class vtkHardwareSelector;

class VTK_VIEWS_EXPORT vtkTreeAreaView : public vtkRenderView
{
public:
  static vtkTreeAreaView *New();
  vtkTypeRevisionMacro(vtkTreeAreaView, vtkRenderView);
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

  // Description:
  // The array to use for area labeling priority.
  // Default is "GraphVertexDegree".
  void SetLabelPriorityArrayName(const char* name);

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
  void AreaLabelVisibilityOn();
  void AreaLabelVisibilityOff();

  // Description:
  // Whether to show edge labels.  Default is off.
  void SetEdgeLabelVisibility(bool vis);
  bool GetEdgeLabelVisibility();
  void EdgeLabelVisibilityOn();
  void EdgeLabelVisibilityOff();

  // Description:
  // The array to use for coloring vertices.  Default is "color".
  void SetAreaColorArrayName(const char* name);

  // Description:
  // Whether to color vertices.  Default is off.
  void SetColorVertices(bool vis);
  bool GetColorVertices();
  vtkBooleanMacro(ColorVertices, bool);

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
  // The filter for converting areas to polydata. This may e.g. be
  // vtkTreeMapToPolyData or vtkTreeRingToPolyData.
  // The filter must take a vtkTree as input and produce vtkPolyData.
  virtual void SetAreaToPolyData(vtkPolyDataAlgorithm* areaToPoly);
  vtkGetObjectMacro(AreaToPolyData, vtkPolyDataAlgorithm);

  // Description:
  // Whether the area represents radial or rectangular coordinates.
  virtual void SetUseRectangularCoordinates(bool rect);
  virtual bool GetUseRectangularCoordinates();
  vtkBooleanMacro(UseRectangularCoordinates, bool);

  // Description:
  // The mapper for rendering labels on areas. This may e.g. be
  // vtkDynamic2DLabelMapper or vtkTreeMapLabelMapper.
  virtual void SetAreaLabelMapper(vtkLabeledDataMapper* mapper);
  vtkGetObjectMacro(AreaLabelMapper, vtkLabeledDataMapper);

protected:
  vtkTreeAreaView();
  ~vtkTreeAreaView();

  // Description:
  // Called to process the user event from the interactor style.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId,
                             void* callData);

  // Description:
  // Connects the algorithm output to the internal pipelines.
  virtual void AddInputConnection(int port, int item,
                                  vtkAlgorithmOutput* conn,
                                  vtkAlgorithmOutput* selectConn);

  // Description:
  // Removes the algorithm output from the internal pipeline.
  virtual void RemoveInputConnection(int port, int item,
                                     vtkAlgorithmOutput* conn,
                                     vtkAlgorithmOutput* selectConn);

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
  vtkSmartPointer<vtkTreeLevelsFilter>             TreeLevels;

  // Representation objects
  vtkSmartPointer<vtkDynamic2DLabelMapper>         EdgeLabelMapper;
  vtkSmartPointer<vtkActor2D>                      EdgeLabelActor;
  vtkSmartPointer<vtkPolyDataMapper>               GraphEdgeMapper;
  vtkSmartPointer<vtkActor>                        GraphEdgeActor;

  // Area objects
  vtkSmartPointer<vtkAreaLayout>                   AreaLayout;
  vtkSmartPointer<vtkPolyDataMapper>               AreaMapper;
  vtkSmartPointer<vtkActor>                        AreaActor;
  vtkSmartPointer<vtkActor2D>                      AreaLabelActor;
  vtkPolyDataAlgorithm*                            AreaToPolyData;
  vtkLabeledDataMapper*                            AreaLabelMapper;

  // Graph edge selection objects
  vtkSmartPointer<vtkSelection>                    EmptySelection;
  vtkSmartPointer<vtkKdTreeSelector>               KdTreeSelector;
  vtkSmartPointer<vtkHardwareSelector>             HardwareSelector;
  vtkSmartPointer<vtkExtractSelectedGraph>         ExtractSelectedGraph;
  vtkSmartPointer<vtkGraphHierarchicalBundle>      SelectedGraphHBundle;
  vtkSmartPointer<vtkSplineFilter>                 SelectedGraphSpline;
  vtkSmartPointer<vtkActor>                        SelectedGraphActor;
  vtkSmartPointer<vtkPolyDataMapper>               SelectedGraphMapper;

  // Area selection objects
  vtkSmartPointer<vtkConvertSelection>             ConvertSelection;
  vtkSmartPointer<vtkExtractSelectedPolyDataIds>   ExtractSelectedAreas;
  vtkSmartPointer<vtkPolyDataMapper>               SelectedAreaMapper;
  vtkSmartPointer<vtkActor>                        SelectedAreaActor;
  //ETX

private:
  vtkTreeAreaView(const vtkTreeAreaView&);  // Not implemented.
  void operator=(const vtkTreeAreaView&);  // Not implemented.
};

#endif
