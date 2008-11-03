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
// .NAME vtkGeoView - A geoview
// .SECTION Description
// vtkGeoView can contain vtkGeoRepresentations.

#ifndef __vtkGeoView_h
#define __vtkGeoView_h

#include "vtkRenderView.h"

class vtkActor;
class vtkGeoAlignedImageRepresentation;
class vtkGeoInteractorStyle;
class vtkGlobeSource;
class vtkPolyDataMapper;
class vtkRenderWindow;
class vtkViewTheme;

class VTK_GEOVIS_EXPORT vtkGeoView : public vtkRenderView
{
public:
  static vtkGeoView *New();
  vtkTypeRevisionMacro(vtkGeoView, vtkRenderView);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Adds an image representation with a simple terrain model using
  // the image in the specified file as the globe terrain.
  vtkGeoAlignedImageRepresentation* AddDefaultImageRepresentation(const char* filename);
  
  // Description:
  // Set up a render window to use this view.
  // The superclass adds the renderer to the render window.
  // Subclasses should override this to set interactor, etc.
  virtual void SetupRenderWindow(vtkRenderWindow* win);

  // Decsription:
  // Prepares the view for rendering.
  virtual void PrepareForRendering();
  
  // Description:
  // Rebuild low-res earth source; call after (re)setting origin.
  void BuildLowResEarth( double origin[3] );
  
  // Description:
  // Whether the view locks the heading when panning.
  // Default is off.
  virtual void SetLockHeading(bool lock);
  virtual bool GetLockHeading();
  vtkBooleanMacro(LockHeading, bool);

  // Description:
  // Apply a view theme to the view.
  virtual void ApplyViewTheme(vtkViewTheme* theme);

  // Description:
  // Convenience method for obtaining the internal interactor style.
  vtkGeoInteractorStyle* GetGeoInteractorStyle();

  // Description:
  // Method to change the interactor style.
  virtual void SetGeoInteractorStyle(vtkGeoInteractorStyle* style);
  
protected:
  vtkGeoView();
  ~vtkGeoView();
  
  vtkRenderWindow*   RenderWindow;
  vtkGlobeSource*    LowResEarthSource;
  vtkPolyDataMapper* LowResEarthMapper;
  vtkActor*          LowResEarthActor;

private:
  vtkGeoView(const vtkGeoView&);  // Not implemented.
  void operator=(const vtkGeoView&);  // Not implemented.
};

#endif

