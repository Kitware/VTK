/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeLayoutView.h

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
// .NAME vtkTreeLayoutView - Lays out and displays a tree.
//
// .SECTION Description
// vtkTreeLayoutView displays a tree in radial or standard "top-down" format.
// You may specify the vertex labels and colors.

#ifndef __vtkTreeLayoutView_h
#define __vtkTreeLayoutView_h

#include "vtkRenderView.h"

class vtkActor;
class vtkActor2D;
class vtkCoordinate;
class vtkDynamic2DLabelMapper;
class vtkExtractSelectedGraph;
class vtkGraphLayout;
class vtkGraphToPolyData;
class vtkInteractorStyleRubberBand2D;
class vtkKdTreeSelector;
class vtkLookupTable;
class vtkPolyDataMapper;
class vtkTreeLayoutStrategy;
class vtkVertexGlyphFilter;
class vtkViewTheme;
class vtkHardwareSelector;

class VTK_VIEWS_EXPORT vtkTreeLayoutView : public vtkRenderView
{
public:
  static vtkTreeLayoutView *New();
  vtkTypeRevisionMacro(vtkTreeLayoutView, vtkRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // The array to use for labeling.  Default is "label".
  void SetLabelArrayName(const char* name);
  const char* GetLabelArrayName();
  
  // Description:
  // Whether to show labels.  Default is off.
  void SetLabelVisibility(bool vis);
  bool GetLabelVisibility();
  void LabelVisibilityOn();
  void LabelVisibilityOff();
  
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
  // The sweep angle of the tree.
  // For a standard tree layout, this should be between 0 and 180.
  // For a radial tree layout, this can be between 0 and 360.
  void SetAngle(double angle);
  double GetAngle();

  // Description:
  // If set, the tree is laid out with levels on concentric circles 
  // around the root. If unset (default), the tree is laid out with 
  // levels on horizontal lines.
  void SetRadial(bool radial);
  bool GetRadial();
  void RadialOn();
  void RadialOff();

  // Description:
  // The spacing of tree levels. Levels near zero give more space
  // to levels near the root, while levels near one (the default)
  // create evenly-spaced levels.
  void SetLogSpacingValue(double value);
  double GetLogSpacingValue();

  // Description:
  // The spacing of leaves.  Levels near one evenly space leaves
  // with no gaps between subtrees.  Levels near zero creates
  // large gaps between subtrees.
  void SetLeafSpacing(double value);
  double GetLeafSpacing();

  // Description:
  // Get/Set the array to use to determine the distance from the
  // root.
  const char* GetDistanceArrayName();
  void SetDistanceArrayName(const char* name);

  // Description:
  // Sets up interactor style.
  virtual void SetupRenderWindow(vtkRenderWindow* win);

  // Description:
  // Apply the theme to this view.
  virtual void ApplyViewTheme(vtkViewTheme* theme);

protected:
  vtkTreeLayoutView();
  ~vtkTreeLayoutView();

  // Description:
  // Called to process the user event from the interactor style.
  virtual void ProcessEvents(vtkObject* caller, unsigned long eventId, 
    void* callData);
  
  // Description:
  // Connects the algorithm output to the internal pipeline.
  // This view only supports a single representation.
  virtual void AddInputConnection(
    vtkAlgorithmOutput* conn,
    vtkAlgorithmOutput* selectionConn);
  
  // Description:
  // Removes the algorithm output from the internal pipeline.
  virtual void RemoveInputConnection(
    vtkAlgorithmOutput* conn,
    vtkAlgorithmOutput* selectionConn);
  
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
  
  // Used for coordinate conversion
  vtkCoordinate* Coordinate;

  // Representation objects
  vtkGraphLayout*           GraphLayout;
  vtkTreeLayoutStrategy*    TreeStrategy;
  vtkGraphToPolyData*       GraphToPolyData;
  vtkVertexGlyphFilter*     VertexGlyph;
  vtkPolyDataMapper*        VertexMapper;
  vtkLookupTable*           VertexColorLUT;
  vtkActor*                 VertexActor;
  vtkPolyDataMapper*        OutlineMapper;
  vtkActor*                 OutlineActor;
  vtkPolyDataMapper*        EdgeMapper;
  vtkLookupTable*           EdgeColorLUT;
  vtkActor*                 EdgeActor;
  vtkDynamic2DLabelMapper*  LabelMapper;
  vtkActor2D*               LabelActor;
  
  // Selection objects
  vtkKdTreeSelector*        KdTreeSelector;
  vtkHardwareSelector*      HardwareSelector;
  vtkExtractSelectedGraph*  ExtractSelectedGraph;
  vtkGraphToPolyData*       SelectionToPolyData;
  vtkVertexGlyphFilter*     SelectionVertexGlyph;
  vtkPolyDataMapper*        SelectionVertexMapper;
  vtkActor*                 SelectionVertexActor;
  vtkPolyDataMapper*        SelectionEdgeMapper;
  vtkActor*                 SelectionEdgeActor;

private:
  vtkTreeLayoutView(const vtkTreeLayoutView&);  // Not implemented.
  void operator=(const vtkTreeLayoutView&);  // Not implemented.
};

#endif
