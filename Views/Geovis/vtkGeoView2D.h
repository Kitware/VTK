/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoView2D.h

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
/**
 * @class   vtkGeoView2D
 * @brief   A 2D geospatial view.
 *
 *
 * vtkGeoView is a 2D globe view. The globe may contain a multi-resolution
 * geometry source (vtkGeoTerrain2D), multiple multi-resolution image sources
 * (vtkGeoAlignedImageRepresentation), as well as other representations such
 * as vtkGeoGraphRepresentation2D. At a minimum, the view must have a terrain
 * and one image representation. By default, you may select in the view with
 * the left mouse button, pan with the middle button, and zoom with the right
 * mouse button or scroll wheel.
 *
 * Each terrain or image representation contains a vtkGeoSource subclass which
 * generates geometry or imagery at multiple resolutions. As the camera
 * position changes, the terrain and/or image representations may ask its
 * vtkGeoSource to refine the geometry. This refinement is performed on a
 * separate thread, and the data is added to the view when it becomes available.
 *
 * @sa
 * vtkGeoTerrain2D vtkGeoAlignedImageRepresentation vtkGeoSource
*/

#ifndef vtkGeoView2D_h
#define vtkGeoView2D_h

#include "vtkViewsGeovisModule.h" // For export macro
#include "vtkRenderView.h"

class vtkAssembly;
class vtkGeoTerrain2D;
class vtkViewTheme;

class VTKVIEWSGEOVIS_EXPORT vtkGeoView2D : public vtkRenderView
{
public:
  static vtkGeoView2D *New();
  vtkTypeMacro(vtkGeoView2D,vtkRenderView);
  virtual void PrintSelf( ostream& os, vtkIndent indent );

  vtkGeoView2D();
  ~vtkGeoView2D();

  vtkGetObjectMacro(Surface, vtkGeoTerrain2D);
  virtual void SetSurface(vtkGeoTerrain2D* surf);

  /**
   * Returns the transform associated with the surface.
   */
  virtual vtkAbstractTransform* GetTransform();

  /**
   * Apply the view theme to this view.
   */
  virtual void ApplyViewTheme(vtkViewTheme* theme);

  /**
   * Update and render the view.
   */
  virtual void Render();

protected:
  vtkGeoTerrain2D* Surface;
  vtkAssembly* Assembly;

  virtual void PrepareForRendering();

private:
  vtkGeoView2D(const vtkGeoView2D&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoView2D&) VTK_DELETE_FUNCTION;
};

#endif
