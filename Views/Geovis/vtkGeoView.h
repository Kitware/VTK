/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoView.h

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
 * @class   vtkGeoView
 * @brief   A 3D geospatial view.
 *
 *
 * vtkGeoView is a 3D globe view. The globe may contain a multi-resolution
 * geometry source (vtkGeoTerrain), multiple multi-resolution image sources
 * (vtkGeoAlignedImageRepresentation), as well as other representations such
 * as vtkRenderedGraphRepresentation. At a minimum, the view must have a terrain
 * and one image representation. The view uses vtkGeoInteractorStyle to orbit,
 * zoom, and tilt the view, and contains a vtkCompassWidget for manipulating
 * the camera.
 *
 * Each terrain or image representation contains a vtkGeoSource subclass which
 * generates geometry or imagery at multiple resolutions. As the camera
 * position changes, the terrain and/or image representations may ask its
 * vtkGeoSource to refine the geometry. This refinement is performed on a
 * separate thread, and the data is added to the view when it becomes available.
 *
 * @sa
 * vtkGeoTerrain vtkGeoAlignedImageRepresentation vtkGeoSource
*/

#ifndef vtkGeoView_h
#define vtkGeoView_h

#include "vtkViewsGeovisModule.h" // For export macro
#include "vtkRenderView.h"

class vtkActor;
class vtkAssembly;
class vtkGeoAlignedImageRepresentation;
class vtkGeoInteractorStyle;
class vtkGeoTerrain;
class vtkGlobeSource;
class vtkImageData;
class vtkPolyDataMapper;
class vtkViewTheme;

class VTKVIEWSGEOVIS_EXPORT vtkGeoView : public vtkRenderView
{
public:
  static vtkGeoView *New();
  vtkTypeMacro(vtkGeoView, vtkRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Adds an image representation with a simple terrain model using
   * the image in the specified file as the globe terrain.
   */
  vtkGeoAlignedImageRepresentation* AddDefaultImageRepresentation(vtkImageData* image);

  /**
   * Prepares the view for rendering.
   */
  virtual void PrepareForRendering();

  /**
   * Rebuild low-res earth source; call after (re)setting origin.
   */
  void BuildLowResEarth( double origin[3] );

  //@{
  /**
   * Whether the view locks the heading when panning.
   * Default is off.
   */
  virtual void SetLockHeading(bool lock);
  virtual bool GetLockHeading();
  vtkBooleanMacro(LockHeading, bool);
  //@}

  /**
   * Convenience method for obtaining the internal interactor style.
   */
  vtkGeoInteractorStyle* GetGeoInteractorStyle();

  /**
   * Method to change the interactor style.
   */
  virtual void SetGeoInteractorStyle(vtkGeoInteractorStyle* style);

  //@{
  /**
   * The terrain (geometry) model for this earth view.
   */
  virtual void SetTerrain(vtkGeoTerrain* terrain);
  vtkGetObjectMacro(Terrain, vtkGeoTerrain);
  //@}

  /**
   * Update and render the view.
   */
  virtual void Render();

protected:
  vtkGeoView();
  ~vtkGeoView();

  vtkGlobeSource*    LowResEarthSource;
  vtkPolyDataMapper* LowResEarthMapper;
  vtkActor*          LowResEarthActor;
  vtkAssembly*       Assembly;
  vtkGeoTerrain*     Terrain;

  int                UsingMesaDrivers;

private:
  vtkGeoView(const vtkGeoView&) VTK_DELETE_FUNCTION;
  void operator=(const vtkGeoView&) VTK_DELETE_FUNCTION;
};

#endif

