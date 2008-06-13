/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGeoAlignedImage.h

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

// .NAME vtkGeoAlignedImage - 
// .SECTION Description I wanted to hide the normal vtkCamera API

// .SECTION See Also
   
#ifndef __vtkGeoAlignedImage_h
#define __vtkGeoAlignedImage_h

#include <vtkstd/vector> // vector
#include <vtkstd/stack> // stack
#include "vtkSmartPointer.h" // for SP
#include "vtkGeoAlignedImageCache.h"


class vtkGeoTerrain;
class vtkAssembly;
class vtkGeoPatch;

class VTK_GEOVIS_EXPORT vtkGeoAlignedImage : public vtkObject
{
public:
  static vtkGeoAlignedImage *New();
  vtkTypeRevisionMacro(vtkGeoAlignedImage, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This builds the image from the latest request using the image patches
  // currently available.  It returns true if the model changes.
  bool Update(vtkGeoTerrain* terrain);

  // Description:
  // Add the actors that render the terrain image pairs to the assembly.
  void UpdateAssembly(vtkAssembly* assembly);
  
  // Description:
  // This terrain object gets its terrain patches from this cache.
  // The user needs to set this.
  void SetCache(vtkGeoAlignedImageCache* cache);
  vtkGeoAlignedImageCache* GetCache();  

  // Description:
  // This is to clean up actors, mappers, textures and other rendering object
  // before the renderer and render window destruct.  It allows all graphics
  // resources to be released cleanly.  Without this, the application 
  // may crash on exit.
  void ExitCleanup();

protected:
  vtkGeoAlignedImage();
  ~vtkGeoAlignedImage();

  // Returns 0 if index is out of range.
  vtkGeoPatch* GetPatch(int idx);

  vtkGeoPatch* GetNewPatchFromHeap();
  void ReturnPatchToHeap(vtkGeoPatch* patch);
  void DeletePatches();

//BTX
  vtkstd::vector<vtkGeoPatch* > Patches;
  vtkstd::stack<vtkGeoPatch* > PatchHeap;
  vtkSmartPointer<vtkGeoAlignedImageCache> Cache;
//ETX

private:
  vtkGeoAlignedImage(const vtkGeoAlignedImage&);  // Not implemented.
  void operator=(const vtkGeoAlignedImage&);  // Not implemented.
};

#endif
