/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoGraphRepresentation2D.h

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
// .NAME vtkGeoGraphRepresentation2D - Displays a graph on a 2D earth view.
//
// .SECTION Description
// vtkGeoGraphRepresentation2D is used to show a graph in a 2D geo view.
// Set the graph input with SetInputConnection(), set the projection of the
// view with SetTransform(), then add the representation to a vtkGeoView2D.
//
// .SECTION See Also
// vtkGeoView2D

#ifndef __vtkGeoGraphRepresentation2D_h
#define __vtkGeoGraphRepresentation2D_h

#include "vtkDataRepresentation.h"
#include "vtkSmartPointer.h" // for ivars

class vtkActor;
class vtkActor2D;
class vtkAlgorithmOutput;
class vtkDataObject;
class vtkDynamic2DLabelMapper;
class vtkEdgeCenters;
class vtkEdgeLayout;
class vtkEdgeLayoutStrategy;
class vtkExtractSelectedGraph;
class vtkGeoAssignCoordinates;
class vtkGraphMapper;
class vtkGraphToPolyData;
class vtkLabeledDataMapper;
class vtkLabelPlacer;
class vtkLabelSizeCalculator;
class vtkPerturbCoincidentVertices;
class vtkPointSetToLabelHierarchy;
class vtkPolyDataMapper;
class vtkSelection;
class vtkAbstractTransform;
class vtkView;
class vtkViewTheme;

class VTK_GEOVIS_EXPORT vtkGeoGraphRepresentation2D : public vtkDataRepresentation
{
public:
  static vtkGeoGraphRepresentation2D *New();
  vtkTypeRevisionMacro(vtkGeoGraphRepresentation2D, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The array to use for vertex labeling.  Default is "label".
  virtual void SetVertexLabelArrayName(const char* name);
  virtual const char* GetVertexLabelArrayName();

  // Description:
  // Whether to show vertex labels.
  virtual void SetVertexLabelVisibility(bool b);
  virtual bool GetVertexLabelVisibility();
  vtkBooleanMacro(VertexLabelVisibility, bool);

  // Description:
  // The point array holding the latitude.
  virtual void SetLatitudeArrayName(const char* name);
  virtual const char* GetLatitudeArrayName();

  // Description:
  // The point array holding the longitude.
  virtual void SetLongitudeArrayName(const char* name);
  virtual const char* GetLongitudeArrayName();

  // Description:
  // The size of the vertex labels in pixels.
  void SetVertexLabelFontSize(int size);
  int GetVertexLabelFontSize();

  // Description:
  // Whether to color vertices using a data array.
  void SetColorVertices(bool b);
  bool GetColorVertices();
  vtkBooleanMacro(ColorVertices, bool);

  // Description:
  // The data array to use to color vertices.
  void SetVertexColorArrayName(const char* name);
  const char* GetVertexColorArrayName();

  // Description:
  // Whether to show edge labels.
  void SetEdgeLabelVisibility(bool b);
  bool GetEdgeLabelVisibility();
  vtkBooleanMacro(EdgeLabelVisibility, bool);

  // Description:
  // The data array to use for labelling edges.
  void SetEdgeLabelArrayName(const char* name);
  const char* GetEdgeLabelArrayName();

  // Description:
  // The edge layout strategy to use.
  // The default is vtkArcParallelEdgeStrategy.
  virtual void SetEdgeLayoutStrategy(vtkEdgeLayoutStrategy* strategy);
  virtual vtkEdgeLayoutStrategy* GetEdgeLayoutStrategy();
  virtual void SetEdgeLayoutStrategyToArcParallel();
  virtual void SetEdgeLayoutStrategyToPassThrough();

  // Description:
  // The size of edge labels in pixels.
  void SetEdgeLabelFontSize(int size);
  int GetEdgeLabelFontSize();

  // Description:
  // Whether to color edges using a data array.
  void SetColorEdges(bool b);
  bool GetColorEdges();
  vtkBooleanMacro(ColorEdges, bool);

  // Description:
  // The data array to use for coloring edges.
  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();

  // Description:
  // The transform used in vtkGeoAssignCoordinates to transform
  // the vertex locations from lat/long to world coordinates.
  void SetTransform(vtkAbstractTransform* trans);
  vtkAbstractTransform* GetTransform();

  // Description:
  // Apply a theme to this view.
  void ApplyViewTheme(vtkViewTheme* theme);

  // Description:
  // Called by the view when the renderer is about to render.
  virtual void PrepareForRendering();

  // Description:
  // When on uses vtkLabelPlacer otherwise use older vtkDynamic2DLabelMapper, 
  // which has O(n^2) preprocessing time.
  void SetUseLabelHierarchy(bool b);
  vtkGetMacro(UseLabelHierarchy, bool);
  vtkBooleanMacro(UseLabelHierarchy, bool);
  
protected:
  vtkGeoGraphRepresentation2D();
  ~vtkGeoGraphRepresentation2D();

  // Description:
  // Sets the input connections for this representation.
  virtual void SetupInputConnections();

  // Decription:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().
  virtual bool AddToView(vtkView* view);

  // Decription:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().
  virtual bool RemoveFromView(vtkView* view);

  // Description:
  // Convert the selection to a type appropriate for sharing with other
  // representations through vtkSelectionLink.
  // If the selection cannot be applied to this representation, returns NULL.
  virtual vtkSelection* ConvertSelection(vtkView* view, vtkSelection* selection);

  //BTX
  // Description:
  // Internal pipeline objects.
  vtkSmartPointer<vtkGeoAssignCoordinates>       AssignCoordinates;
  vtkSmartPointer<vtkPerturbCoincidentVertices>  PerturbCoincidentVertices;
  vtkSmartPointer<vtkLabelSizeCalculator>        LabelSize;
  vtkSmartPointer<vtkPointSetToLabelHierarchy>   LabelHierarchy;
  vtkSmartPointer<vtkLabelPlacer>                LabelPlacer;
  vtkSmartPointer<vtkLabeledDataMapper>          LabelMapper;
  vtkSmartPointer<vtkActor2D>                    LabelActor;
  vtkSmartPointer<vtkDynamic2DLabelMapper>       DynamicLabelMapper;
  vtkSmartPointer<vtkActor2D>                    DynamicLabelActor;
  vtkSmartPointer<vtkEdgeLayout>                 EdgeLayout;
  vtkSmartPointer<vtkGraphMapper>                GraphMapper;
  vtkSmartPointer<vtkActor>                      GraphActor;
  vtkSmartPointer<vtkGraphToPolyData>            GraphToPolyData;
  vtkSmartPointer<vtkEdgeCenters>                EdgeCenters;
  vtkSmartPointer<vtkDynamic2DLabelMapper>       EdgeLabelMapper;
  vtkSmartPointer<vtkActor2D>                    EdgeLabelActor;
  vtkSmartPointer<vtkExtractSelectedGraph>       ExtractSelection;
  vtkSmartPointer<vtkGraphMapper>                SelectionMapper;
  vtkSmartPointer<vtkActor>                      SelectionActor;
  //ETX

  bool UseLabelHierarchy;

private:
  vtkGeoGraphRepresentation2D(const vtkGeoGraphRepresentation2D&);  // Not implemented.
  void operator=(const vtkGeoGraphRepresentation2D&);  // Not implemented.
};

#endif
