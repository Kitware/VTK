/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaVolumeTextureMapper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaVolumeTextureMapper2D - Abstract class for a volume mapper

// .SECTION Description
// vtkMesaVolumeTextureMapper2D renders a volume using 2D texture mapping.


// .SECTION see also
// vtkVolumeMapper

#ifndef __vtkMesaVolumeTextureMapper2D_h
#define __vtkMesaVolumeTextureMapper2D_h

#include "vtkVolumeTextureMapper2D.h"

class VTK_VOLUMERENDERING_EXPORT vtkMesaVolumeTextureMapper2D : public vtkVolumeTextureMapper2D
{
public:
  vtkTypeMacro(vtkMesaVolumeTextureMapper2D,vtkVolumeTextureMapper2D);
  void PrintSelf( ostream& os, vtkIndent indent );

  static vtkMesaVolumeTextureMapper2D *New();
  
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
  vtkMesaVolumeTextureMapper2D();
  ~vtkMesaVolumeTextureMapper2D();

private:
  vtkMesaVolumeTextureMapper2D(const vtkMesaVolumeTextureMapper2D&);  // Not implemented.
  void operator=(const vtkMesaVolumeTextureMapper2D&);  // Not implemented.
};


#endif


