/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRayCastImageDisplayHelper.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkRayCastImageDisplayHelper - helper class that draws the image to the screen
// .SECTION Description
// This is a helper class for drawing images created from ray casting on the screen.
// This is the abstract device-independent superclass.

// .SECTION see also
// vtkVolumeRayCastMapper vtkUnstructuredGridVolumeRayCastMapper
// vtkOpenGLRayCastImageDisplayHelper

#ifndef __vtkRayCastImageDisplayHelper_h
#define __vtkRayCastImageDisplayHelper_h

#include "vtkObject.h"

class vtkVolume;
class vtkRenderer;

class VTK_RENDERING_EXPORT vtkRayCastImageDisplayHelper : public vtkObject
{
public:
  static vtkRayCastImageDisplayHelper *New();
  vtkTypeRevisionMacro(vtkRayCastImageDisplayHelper,vtkObject);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  virtual void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                              int imageMemorySize[2],
                              int imageViewportSize[2],
                              int imageInUseSize[2],
                              int imageOrigin[2],
                              float requestedDepth,
                              unsigned char *image ) = 0;

protected:
  vtkRayCastImageDisplayHelper();
  ~vtkRayCastImageDisplayHelper();

private:
  vtkRayCastImageDisplayHelper(const vtkRayCastImageDisplayHelper&);  // Not implemented.
  void operator=(const vtkRayCastImageDisplayHelper&);  // Not implemented.
};

#endif

