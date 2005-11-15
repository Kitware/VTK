/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOrientedGlyphContourRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOrientedGlyphContourHandleRepresentation - point representation constrained to a 2D plane
// .SECTION Description
// This class is used to represent a vtkHandleWidget. It represents a
// position in 3D world coordinates that is constrained to a specified plane.
// The default look is to draw a white point when this widget is not selected
// or active, a thin green circle when it is highlighted, and a thicker cyan
// circle when it is active (being positioned). Defaults can be adjusted - but
// take care to define cursor geometry that makes sense for this widget.
// The geometry will be aligned on the constraining plane, with the plane
// normal aligned with the X axis of the geometry (similar behavior to
// vtkGlyph3D). 
//
// TODO: still need to work on 
// 1) translation when mouse is outside bounding planes
// 2) size of the widget
//
// .SECTION See Also
// vtkHandleRepresentation vtkHandleWidget


#ifndef __vtkOrientedGlyphContourRepresentation_h
#define __vtkOrientedGlyphContourRepresentation_h

#include "vtkContourRepresentation.h"

class vtkProperty;
class vtkActor;
class vtkPolyDataMapper;
class vtkPolyData;
class vtkGlyph3D;
class vtkPoints;
class vtkPolyData;

class VTK_WIDGETS_EXPORT vtkOrientedGlyphContourRepresentation : public vtkContourRepresentation
{
public:
  // Description:
  // Instantiate this class.
  static vtkOrientedGlyphContourRepresentation *New();

  // Description:
  // Standard methods for instances of this class.
  vtkTypeRevisionMacro(vtkOrientedGlyphContourRepresentation,vtkContourRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify the cursor shape. Keep in mind that the shape will be
  // aligned with the  constraining plane by orienting it such that
  // the x axis of the geometry lies along the normal of the plane.
  void SetCursorShape(vtkPolyData *cursorShape);
  vtkPolyData *GetCursorShape();

  // Description:
  // Specify the shape of the cursor (handle) when it is active.
  // This is the geometry that will be used when the mouse is
  // close to the handle or if the user is manipulating the handle.
  void SetActiveCursorShape(vtkPolyData *activeShape);
  vtkPolyData *GetActiveCursorShape();

  // Description:
  // This is the property used when the handle is not active 
  // (the mouse is not near the handle)
  vtkGetObjectMacro(Property,vtkProperty);
  
  // Description:
  // This is the property used when the user is interacting
  // with the handle.
  vtkGetObjectMacro(ActiveProperty,vtkProperty);
  
  // Description:
  // This is the property used by the lines.
  vtkGetObjectMacro(LinesProperty,vtkProperty);
  
  // Description:
  // Subclasses of vtkOrientedGlyphContourRepresentation must implement these methods. These
  // are the methods that the widget and its representation use to
  // communicate with each other.
  virtual void SetRenderer(vtkRenderer *ren);
  virtual void BuildRepresentation();
  virtual void StartWidgetInteraction(double eventPos[2]);
  virtual void WidgetInteraction(double eventPos[2]);
  virtual int ComputeInteractionState(int X, int Y, int modified=0);

  // Description:
  // Methods to make this class behave as a vtkProp.
  virtual void GetActors(vtkPropCollection *);
  virtual void ReleaseGraphicsResources(vtkWindow *);
  virtual int RenderOverlay(vtkViewport *viewport);
  virtual int RenderOpaqueGeometry(vtkViewport *viewport);
  virtual int RenderTranslucentGeometry(vtkViewport *viewport);
 
  // Description:
  // Get the points in this contour as a vtkPolyData. 
  virtual const vtkPolyData * const GetContourRepresentationAsPolyData() const;
  
protected:
  vtkOrientedGlyphContourRepresentation();
  ~vtkOrientedGlyphContourRepresentation();

  // Render the cursor
  vtkActor             *Actor;
  vtkPolyDataMapper    *Mapper;
  vtkGlyph3D           *Glypher;
  vtkActor             *ActiveActor;
  vtkPolyDataMapper    *ActiveMapper;
  vtkGlyph3D           *ActiveGlypher;
  vtkPolyData          *CursorShape;
  vtkPolyData          *ActiveCursorShape;
  vtkPolyData          *FocalData;
  vtkPoints            *FocalPoint;
  vtkPolyData          *ActiveFocalData;
  vtkPoints            *ActiveFocalPoint;

  vtkPolyData          *Lines;
  vtkPolyDataMapper    *LinesMapper;
  vtkActor             *LinesActor;
  
  // Support picking
  double LastPickPosition[3];
  double LastEventPosition[2];
  
  // Methods to manipulate the cursor
  void Translate(double eventPos[2]);
  void Scale(double eventPos[2]);

  // Properties used to control the appearance of selected objects and
  // the manipulator in general.
  vtkProperty   *Property;
  vtkProperty   *ActiveProperty;
  vtkProperty   *LinesProperty;
  void           CreateDefaultProperties();
  
  
  // Distance between where the mouse event happens and where the
  // widget is focused - maintain this distance during interaction.
  double InteractionOffset[2];

  void BuildLines();
  
private:
  vtkOrientedGlyphContourRepresentation(const vtkOrientedGlyphContourRepresentation&);  //Not implemented
  void operator=(const vtkOrientedGlyphContourRepresentation&);  //Not implemented
};

#endif
