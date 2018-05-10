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

/**
 * @class   vtkScalarBarRepresentation
 * @brief   represent scalar bar for vtkScalarBarWidget
 *
 *
 *
 * This class represents a scalar bar for a vtkScalarBarWidget.  This class
 * provides support for interactively placing a scalar bar on the 2D overlay
 * plane.  The scalar bar is defined by an instance of vtkScalarBarActor.
 *
 * One specialty of this class is that if the scalar bar is moved near enough
 * to an edge, it's orientation is flipped to match that edge.
 *
 * @sa
 * vtkScalarBarWidget vtkWidgetRepresentation vtkScalarBarActor
 *
*/

#ifndef vtkScalarBarRepresentation_h
#define vtkScalarBarRepresentation_h

#include "vtkInteractionWidgetsModule.h" // For export macro
#include "vtkBorderRepresentation.h"

class vtkScalarBarActor;

class VTKINTERACTIONWIDGETS_EXPORT vtkScalarBarRepresentation : public vtkBorderRepresentation
{
public:
  vtkTypeMacro(vtkScalarBarRepresentation, vtkBorderRepresentation);
  void PrintSelf(ostream &os, vtkIndent indent) override;
  static vtkScalarBarRepresentation *New();

  //@{
  /**
   * The prop that is placed in the renderer.
   */
  vtkGetObjectMacro(ScalarBarActor, vtkScalarBarActor);
  virtual void SetScalarBarActor(vtkScalarBarActor *);
  //@}

  //@{
  /**
   * Satisfy the superclass' API.
   */
  void BuildRepresentation() override;
  void WidgetInteraction(double eventPos[2]) override;
  void GetSize(double size[2]) override
    {size[0]=2.0; size[1]=2.0;}
  //@}

  //@{
  /**
   * These methods are necessary to make this representation behave as
   * a vtkProp.
   */
  vtkTypeBool GetVisibility() override;
  void SetVisibility(vtkTypeBool) override;
  void GetActors2D(vtkPropCollection *collection) override;
  void ReleaseGraphicsResources(vtkWindow *window) override;
  int RenderOverlay(vtkViewport*) override;
  int RenderOpaqueGeometry(vtkViewport*) override;
  int RenderTranslucentPolygonalGeometry(vtkViewport*) override;
  vtkTypeBool HasTranslucentPolygonalGeometry() override;
  //@}

  //@{
  /**
   * If true, the orientation will be updated based on the widget's position.
   * Default is true.
   */
  vtkSetMacro(AutoOrient, bool)
  vtkGetMacro(AutoOrient, bool)
  //@}

  //@{
  /**
   * Get/Set the orientation.
   */
  void SetOrientation(int orient);
  int GetOrientation();
  //@}

protected:
  vtkScalarBarRepresentation();
  ~vtkScalarBarRepresentation() override;

  /**
   * Change horizontal <--> vertical orientation, rotate the corners of the
   * bar to preserve size, and swap the resize handle locations.
   */
  void SwapOrientation();

  vtkScalarBarActor *ScalarBarActor;
  bool AutoOrient;

private:
  vtkScalarBarRepresentation(const vtkScalarBarRepresentation &) = delete;
  void operator=(const vtkScalarBarRepresentation &) = delete;
};

#endif //vtkScalarBarRepresentation_h
