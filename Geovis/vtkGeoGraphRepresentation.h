/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoGraphRepresentation.h

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
// .NAME vtkGeoGraphRepresentation - Displays a geometric dataset on a globe.
//
// .SECTION Description
// vtkGeoGraphRepresentation is used to show a geometric dataset in a geo view.
// The representation uses a vtkGeometryFilter to convert the dataset to
// polygonal data (e.g. volumetric data is converted to its external surface).
// The representation may then be added to a vtkRenderView (normally vtkGeoView).

#ifndef __vtkGeoGraphRepresentation_h
#define __vtkGeoGraphRepresentation_h

#include "vtkDataRepresentation.h"

class vtkActor;
class vtkActor2D;
class vtkAlgorithmOutput;
class vtkCellCenters;
class vtkDataObject;
class vtkExtractSelectedGraph;
class vtkGeoArcs;
class vtkGeoAssignCoordinates;
class vtkGeometryFilter;
class vtkGraphToPolyData;
class vtkLabeledDataMapper;
class vtkMaskPoints;
class vtkPolyDataMapper;
class vtkSelection;
class vtkSelectVisiblePoints;
class vtkTransformPolyDataFilter;
class vtkVertexGlyphFilter;
class vtkView;
class vtkViewTheme;

class VTK_GEOVIS_EXPORT vtkGeoGraphRepresentation : public vtkDataRepresentation
{
public:
  static vtkGeoGraphRepresentation *New();
  vtkTypeRevisionMacro(vtkGeoGraphRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Sets the input pipeGraph connection to this representation.
  virtual void SetInputConnection(vtkAlgorithmOutput* conn);
  
  // Description:
  // Plugs the selection link into the internal pipeGraph.
  virtual void SetSelectionLink(vtkSelectionLink* link);
  
  // Description:
  // The array to use for vertex labeling.  Default is "label".
  virtual void SetVertexLabelArrayName(const char* name);
  virtual const char* GetVertexLabelArrayName();

  // Description:
  // Whether to show vertex labels.
  virtual void SetVertexLabelVisibility(bool b);
  virtual bool GetVertexLabelVisibility();
  virtual void VertexLabelVisibilityOn() { this->SetVertexLabelVisibility(true); }
  virtual void VertexLabelVisibilityOff() { this->SetVertexLabelVisibility(false); }
  
  // Description:
  // Sets the explode factor for the geo arcs.
  virtual void SetExplodeFactor(double factor);
  virtual double GetExplodeFactor();
  
  // Description:
  // The number of subdivisions per arc.
  virtual void SetNumberOfSubdivisions(int num);
  virtual int GetNumberOfSubdivisions();
  
  // Description:
  // The point array holding the latitude.
  virtual void SetLatitudeArrayName(const char* name);
  virtual const char* GetLatitudeArrayName();
  
  // Description:
  // The point array holding the longitude.
  virtual void SetLongitudeArrayName(const char* name);
  virtual const char* GetLongitudeArrayName();

  void SetVertexLabelFontSize(int size);
  int GetVertexLabelFontSize();

  void SetColorVertices(bool b);
  bool GetColorVertices();
  vtkBooleanMacro(ColorVertices, bool);

  void SetVertexColorArrayName(const char* name);
  const char* GetVertexColorArrayName();

  void SetEdgeLabelVisibility(bool b);
  bool GetEdgeLabelVisibility();
  vtkBooleanMacro(EdgeLabelVisibility, bool);

  void SetEdgeLabelArrayName(const char* name);
  const char* GetEdgeLabelArrayName();

  void SetEdgeLabelFontSize(int size);
  int GetEdgeLabelFontSize();

  void SetColorEdges(bool b);
  bool GetColorEdges();
  vtkBooleanMacro(ColorEdges, bool);

  void SetEdgeColorArrayName(const char* name);
  const char* GetEdgeColorArrayName();

  void ApplyViewTheme(vtkViewTheme* theme);

  // Description:
  // Called by the view when the renderer is about to render.
  virtual void PrepareForRendering();
  
protected:
  vtkGeoGraphRepresentation();
  ~vtkGeoGraphRepresentation();
  
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
  
  // Description:
  // Internal pipeline objects.
  vtkGeoAssignCoordinates*    AssignCoordinates;
  vtkGraphToPolyData*         GraphToPolyData;
  vtkGeoArcs*                 GeoArcs;
  vtkCellCenters*             EdgeCellCenters;
  vtkLabeledDataMapper*       EdgeLabelMapper;
  vtkActor2D*                 EdgeLabelActor;
  vtkMaskPoints*              EdgeLabelMaskPoints;
  vtkSelectVisiblePoints*     EdgeLabelSelectVisiblePoints;
  vtkTransformPolyDataFilter* EdgeLabelTransform;
  vtkPolyDataMapper*          EdgeMapper;
  vtkActor*                   EdgeActor;
  vtkVertexGlyphFilter*       VertexGlyph;
  vtkPolyDataMapper*          VertexMapper;
  vtkActor*                   VertexActor;
  vtkPolyDataMapper*          OutlineMapper;
  vtkActor*                   OutlineActor;
  vtkLabeledDataMapper*       VertexLabelMapper;
  vtkActor2D*                 VertexLabelActor;
  vtkMaskPoints*              VertexLabelMaskPoints;
  vtkSelectVisiblePoints*     VertexLabelSelectVisiblePoints;
  vtkTransformPolyDataFilter* VertexLabelTransform;
  vtkExtractSelectedGraph*    ExtractSelection;
  vtkGraphToPolyData*         SelectionToPolyData;
  vtkGeoAssignCoordinates*    SelectionAssignCoords;
  vtkGeoArcs*                 SelectionGeoArcs;
  vtkPolyDataMapper*          SelectionMapper;
  vtkActor*                   SelectionActor;
  vtkVertexGlyphFilter*       SelectionVertexGlyph;
  vtkPolyDataMapper*          SelectionVertexMapper;
  vtkActor*                   SelectionVertexActor;

  vtkGetStringMacro(VertexColorArrayNameInternal);
  vtkSetStringMacro(VertexColorArrayNameInternal);
  char* VertexColorArrayNameInternal;
  vtkGetStringMacro(EdgeColorArrayNameInternal);
  vtkSetStringMacro(EdgeColorArrayNameInternal);
  char* EdgeColorArrayNameInternal;
  
private:
  vtkGeoGraphRepresentation(const vtkGeoGraphRepresentation&);  // Not implemented.
  void operator=(const vtkGeoGraphRepresentation&);  // Not implemented.
};

#endif
