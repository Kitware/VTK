/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoLineRepresentation.h

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
// .NAME vtkGeoLineRepresentation - Displays a geometric dataset on a globe.
//
// .SECTION Description
// vtkGeoLineRepresentation is used to show a geometric dataset in a geo view.
// The representation uses a vtkGeometryFilter to convert the dataset to
// polygonal data (e.g. volumetric data is converted to its external surface).
// The representation may then be added to a vtkRenderView (normally vtkGeoView).

#ifndef __vtkGeoLineRepresentation_h
#define __vtkGeoLineRepresentation_h

#include "vtkDataRepresentation.h"
#include "vtkSmartPointer.h" // For ivars

class vtkAbstractTransform;
class vtkActor;
class vtkAlgorithmOutput;
class vtkDataObject;
class vtkExtractSelection;
class vtkGeoArcs;
class vtkGeoAssignCoordinates;
class vtkGeoSampleArcs;
class vtkGeometryFilter;
class vtkPolyDataMapper;
class vtkSelection;
class vtkVertexGlyphFilter;
class vtkView;

class VTK_GEOVIS_EXPORT vtkGeoLineRepresentation : public vtkDataRepresentation
{
public:
  static vtkGeoLineRepresentation *New();
  vtkTypeRevisionMacro(vtkGeoLineRepresentation, vtkDataRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // The point array holding the latitude.
  virtual void SetLatitudeArrayName(const char *name);
  virtual const char* GetLatitudeArrayName();
  
  // Description:
  // The point array holding the longitude.
  virtual void SetLongitudeArrayName(const char *name);
  virtual const char* GetLongitudeArrayName();
  
  // Description:
  // Whether to show points along the lines.
  // This includes interpolated vertices.
  virtual void SetPointVisibility(bool b);
  virtual bool GetPointVisibility();
  vtkBooleanMacro(PointVisibility, bool);
  
  // Description:
  // Called by the view when the renderer is about to render.
  virtual void PrepareForRendering();
  
  // Description:
  // If on, uses LatitudeArrayName and LongitudeArrayName to
  // move values in data arrays into the points of the data set.
  // Turn off if the lattitude and longitude are already in
  // the points.x
  virtual void SetCoordinatesInArrays(bool b);
  virtual bool GetCoordinatesInArrays();
  vtkBooleanMacro(CoordinatesInArrays, bool);

  // Description:
  // The transform to use for transforming lat/long points into
  // world coordinates. If null, use spherical world model (default).
  virtual void SetTransform(vtkAbstractTransform* transform);
  virtual vtkAbstractTransform* GetTransform();
  
protected:
  vtkGeoLineRepresentation();
  ~vtkGeoLineRepresentation();
  
  // Description:
  // Sets the input pipeline connections for this representation.
  virtual void SetupInputConnections();
  
  // Decription:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().
  virtual bool AddToView(vtkView *view);
  
  // Decription:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().
  virtual bool RemoveFromView(vtkView *view);
  
  // Description:
  // Convert the selection to a type appropriate for sharing with other
  // representations through vtkSelectionLink.
  // If the selection cannot be applied to this representation, returns NULL.
  virtual vtkSelection *ConvertSelection(vtkView *view, vtkSelection *selection);
  
  //BTX

  // Description:
  // Internal pipeline objects.
  vtkSmartPointer<vtkGeometryFilter>       GeometryFilter;
  vtkSmartPointer<vtkGeoAssignCoordinates> AssignCoordinates;
  vtkSmartPointer<vtkGeoSampleArcs>        GeoSampleArcs;
  vtkSmartPointer<vtkPolyDataMapper>       Mapper;
  vtkSmartPointer<vtkActor>                Actor;
  vtkSmartPointer<vtkExtractSelection>     ExtractSelection;
  vtkSmartPointer<vtkGeometryFilter>       SelectionGeometryFilter;
  vtkSmartPointer<vtkGeoAssignCoordinates> SelectionAssignCoords;
  vtkSmartPointer<vtkGeoSampleArcs>        SelectionGeoSampleArcs;
  vtkSmartPointer<vtkPolyDataMapper>       SelectionMapper;
  vtkSmartPointer<vtkActor>                SelectionActor;
  vtkSmartPointer<vtkVertexGlyphFilter>    VertexGlyphFilter;
  vtkSmartPointer<vtkPolyDataMapper>       VertexMapper;
  vtkSmartPointer<vtkActor>                VertexActor;

  //ETX

  bool                     CoordinatesInArrays;
  
private:
  vtkGeoLineRepresentation(const vtkGeoLineRepresentation&);  // Not implemented.
  void operator=(const vtkGeoLineRepresentation&);  // Not implemented.
};

#endif
