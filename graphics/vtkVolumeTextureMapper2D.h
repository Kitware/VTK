/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVolumeTextureMapper2D.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkVolumeTextureMapper2D - Abstract class for a volume mapper

// .SECTION Description
// vtkVolumeTextureMapper2D renders a volume using 2D texture mapping.


// .SECTION see also
// vtkVolumeMapper

#ifndef __vtkVolumeTextureMapper2D_h
#define __vtkVolumeTextureMapper2D_h

#include "vtkVolumeTextureMapper.h"

class VTK_EXPORT vtkVolumeTextureMapper2D : public vtkVolumeTextureMapper
{
public:
  const char *GetClassName() {return "vtkVolumeTextureMapper2D";};
  void PrintSelf( ostream& os, vtkIndent index );

  static vtkVolumeTextureMapper2D *New();
  
//BTX

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
  // Render the volume
  virtual void Render(vtkRenderer *ren, vtkVolume *vol) {};

  virtual void RenderRectangle( float v[12], float t[8],
				unsigned char *texture, int size[2]) {};

  void ApplyProperties( unsigned char *texture,
			int size[2], int tsize[2] ); 

//ETX


protected:
  vtkVolumeTextureMapper2D();
  ~vtkVolumeTextureMapper2D();
  vtkVolumeTextureMapper2D(const vtkVolumeTextureMapper2D&) {};
  void operator=(const vtkVolumeTextureMapper2D&) {};

  void InitializeRender( vtkRenderer *ren, vtkVolume *vol );

  void GenerateTexturesAndRenderRectangles();

  int  MajorDirection;
};


#endif


