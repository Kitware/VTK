/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRayCastImageDisplayHelper.h
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

// .NAME vtkMesaRayCastImageDisplayHelper - Mesa subclass that draws the image to the screen
// .SECTION Description
// This is the concrete implementation of a ray cast image display helper -
// a helper class responsible for drawing the image to the screen.

// .SECTION see also
// vtkRayCastImageDisplayHelper

#ifndef __vtkMesaRayCastImageDisplayHelper_h
#define __vtkMesaRayCastImageDisplayHelper_h

#include "vtkRayCastImageDisplayHelper.h"

class vtkVolume;
class vtkRenderer;

class VTK_RENDERING_EXPORT vtkMesaRayCastImageDisplayHelper : public vtkRayCastImageDisplayHelper
{
public:
  static vtkMesaRayCastImageDisplayHelper *New();
  vtkTypeRevisionMacro(vtkMesaRayCastImageDisplayHelper,vtkRayCastImageDisplayHelper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  void RenderTexture( vtkVolume *vol, vtkRenderer *ren,
                      int imageMemorySize[2],
                      int imageViewportSize[2],
                      int imageInUseSize[2],
                      int imageOrigin[2],
                      float requestedDepth,
                      unsigned char *image );

protected:
  vtkMesaRayCastImageDisplayHelper();
  ~vtkMesaRayCastImageDisplayHelper();

private:
  vtkMesaRayCastImageDisplayHelper(const vtkMesaRayCastImageDisplayHelper&);  // Not implemented.
  void operator=(const vtkMesaRayCastImageDisplayHelper&);  // Not implemented.
};

#endif

