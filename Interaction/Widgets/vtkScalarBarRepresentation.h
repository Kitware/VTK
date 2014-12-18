/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarRepresentation.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkScalarBarRepresentation - represent scalar bar for vtkScalarBarWidget
//
// .SECTION Description
//
// This class represents a scalar bar for a vtkScalarBarWidget.  This class
// provides support for interactively placing a scalar bar on the 2D overlay
// plane.  The scalar bar is defined by an instance of vtkScalarBarActor.
//
// One specialty of this class is that if the scalar bar is moved near enough
// to an edge, it's orientation is flipped to match that edge.
//
// .SECTION See Also
// vtkScalarBarWidget vtkWidgetRepresentation vtkScalarBarActor
//

#ifndef vtkScalarBarRepresentation_h
#define vtkScalarBarRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderRepresentation.h"

class vtkScalarBarActor;

class VTKINTERACTIONWIDGETS_EXPORT vtkScalarBarRepresentation : public vtkBorderRepresentation
{
public:
  vtkTypeMacro(vtkScalarBarRepresentation, vtkBorderRepresentation);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkScalarBarRepresentation *New();

  // Description:
  // The prop that is placed in the renderer.
  vtkGetObjectMacro(ScalarBarActor, vtkScalarBarActor);
  virtual void SetScalarBarActor(vtkScalarBarActor *);

  // Description:
  // Satisfy the superclass' API.
  virtual void BuildRepresentation();
  virtual void WidgetInteraction(double eventPos[2]);
  virtual void GetSize(double size[2])
    {size[0]=2.0; size[1]=2.0;}

  // Description:
  // These methods are necessary to make this representation behave as
  // a vtkProp.
  virtual int GetVisibility();
  virtual void SetVisibility(int);
  virtual void GetActors2D(vtkPropCollection *collection);
  virtual void ReleaseGraphicsResources(vtkWindow *window);
  virtual int RenderOverlay(vtkViewport*);
  virtual int RenderOpaqueGeometry(vtkViewport*);
  virtual int RenderTranslucentPolygonalGeometry(vtkViewport*);
  virtual int HasTranslucentPolygonalGeometry();

  // Description:
  // Get/Set the orientation.
  void SetOrientation(int orient);
  int GetOrientation();

protected:
  vtkScalarBarRepresentation();
  ~vtkScalarBarRepresentation();

  vtkScalarBarActor *ScalarBarActor;
private:
  vtkScalarBarRepresentation(const vtkScalarBarRepresentation &); // Not implemented
  void operator=(const vtkScalarBarRepresentation &);   // Not implemented
};

#endif //vtkScalarBarRepresentation_h
