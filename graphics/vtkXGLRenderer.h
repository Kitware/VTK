/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXGLRenderer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkXGLRenderer - Suns XGL renderer
// .SECTION Description
// vtkXGLRenderer is a concrete implementation of the abstract class
// vtkRenderer. vtkXGLRenderer interfaces to Suns XGL graphics library.

#ifndef __vtkXGLRenderer_h
#define __vtkXGLRenderer_h

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vtkRenderer.h"
#include <xgl/xgl.h>
#include "vtkXGLRenderWindow.h"

#define VTK_MAX_LIGHTS 12

class VTK_EXPORT vtkXGLRenderer : public vtkRenderer
{
protected:
  Xgl_light XglrLights[VTK_MAX_LIGHTS];
  int NumberOfLightsBound;
  Xgl_3d_ctx Context;

public:
  vtkXGLRenderer();
  static vtkXGLRenderer *New() {return new vtkXGLRenderer;};
  const char *GetClassName() {return "vtkXGLRenderer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void Render(void);

  int UpdateActors(void);
  int UpdateVolumes(void);
  int UpdateCameras(void);
  int UpdateLights(void);

  Xgl_3d_ctx *GetContext() {return &(this->Context);};
  Xgl_win_ras  *GetRaster() 
  {return ((vtkXGLRenderWindow *)(this->GetRenderWindow()))->GetRaster();};
  Xgl_light *GetLightArray() {return this->XglrLights;};
};

#endif
