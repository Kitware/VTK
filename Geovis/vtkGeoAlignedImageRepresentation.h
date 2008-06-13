/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImageRepresentation.h

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

// .NAME vtkGeoAlignedImageRepresentation - Earth with a background image.
// .SECTION vtkGeoAlignedImageRepresentation renders the terrain with a 
// background image.  It interfaces with the vtkGeoTerrain 
// and vtkGeoImageCache to get the data it requires to make the model.
// This representation assumes that the terrain and image caches share the
// same tree structure.

// Eventually, socket activity will indicate that new data is available.
// For now, I am supplying a non blocking method that checks for new data.

// .SECTION See Also
   
#ifndef __vtkGeoAlignedImageRepresentation_h
#define __vtkGeoAlignedImageRepresentation_h

#include "vtkGeoRepresentation.h"

#include "vtkSmartPointer.h" // for SP
#include "vtkAssembly.h" // for SP
#include "vtkGeoAlignedImage.h" // for SP

class vtkRenderer;
class vtkGeoCamera;

class VTK_GEOVIS_EXPORT vtkGeoAlignedImageRepresentation : public vtkGeoRepresentation
{
public:
  static vtkGeoAlignedImageRepresentation *New();
  vtkTypeRevisionMacro(vtkGeoAlignedImageRepresentation, vtkGeoRepresentation);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // This constructs the best model possible given the data currently available.
  // The request will be a separate non blocking call.
  bool Update(vtkGeoCamera* cam);
  
  // Description:
  // This actor contains the actor which will render the earth.
  vtkAssembly *GetActor() { return this->Actor; }
  
  // Description:
  // This is the terrain that has the polydata models.  It is set by the user
  // because multiple representations share the same terrain model.
  void SetTerrain(vtkGeoTerrain *terrain) 
    { this->Terrain = terrain;}
  vtkGeoTerrain* GetTerrain() { return this->Terrain;}
  
  // Description:
  // This cache supplies the background images to use as texture maps.
  void SetImage(vtkGeoAlignedImage *image)
    { this->Image = image;}
  vtkGeoAlignedImage* GetImage() { return this->Image;}

  // Decription:
  // Adds the representation to the view.  This is called from
  // vtkView::AddRepresentation().
  virtual bool AddToView(vtkView* view);
  
  // Decription:
  // Removes the representation to the view.  This is called from
  // vtkView::RemoveRepresentation().
  virtual bool RemoveFromView(vtkView* view);
  
  // Description:
  // This is to clean up actors, mappers, textures and other rendering object
  // before the renderer and render window destruct.  It allows all graphics
  // resources to be released cleanly.  Without this, the application 
  // may crash on exit.
  void ExitCleanup();

protected:
  vtkGeoAlignedImageRepresentation();
  ~vtkGeoAlignedImageRepresentation();

//BTX
  vtkSmartPointer<vtkAssembly> Actor;
  vtkSmartPointer<vtkGeoTerrain> Terrain;
  vtkSmartPointer<vtkGeoAlignedImage> Image;
//ETX

private:
  vtkGeoAlignedImageRepresentation(const vtkGeoAlignedImageRepresentation&);  // Not implemented.
  void operator=(const vtkGeoAlignedImageRepresentation&);  // Not implemented.
};

#endif
