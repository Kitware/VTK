/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLVolumeTextureMapper2D.h
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
// .NAME vtkOpenGLVolumeTextureMapper2D - Abstract class for a volume mapper

// .SECTION Description
// vtkOpenGLVolumeTextureMapper2D renders a volume using 2D texture mapping.


// .SECTION see also
// vtkVolumeMapper

#ifndef __vtkOpenGLVolumeTextureMapper2D_h
#define __vtkOpenGLVolumeTextureMapper2D_h

#include "vtkVolumeTextureMapper2D.h"

class VTK_RENDERING_EXPORT vtkOpenGLVolumeTextureMapper2D : public vtkVolumeTextureMapper2D
{
public:
  vtkTypeRevisionMacro(vtkOpenGLVolumeTextureMapper2D,vtkVolumeTextureMapper2D);
  void PrintSelf( ostream& os, vtkIndent index );

  static vtkOpenGLVolumeTextureMapper2D *New();
  
//BTX

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol);

  void RenderQuads( int count, float *v, float *t,
                    unsigned char *texture, int size[2], int reverseFlag);

//ETX

protected:
  vtkOpenGLVolumeTextureMapper2D();
  ~vtkOpenGLVolumeTextureMapper2D();

private:
  vtkOpenGLVolumeTextureMapper2D(const vtkOpenGLVolumeTextureMapper2D&);  // Not implemented.
  void operator=(const vtkOpenGLVolumeTextureMapper2D&);  // Not implemented.
};


#endif


