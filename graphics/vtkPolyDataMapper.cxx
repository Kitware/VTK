/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper.cxx
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
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"

#ifdef VTK_USE_GLR
#include "vtkGLPolyDataMapper.h"
#endif
#ifdef VTK_USE_OGLR
#include "vtkOpenGLPolyDataMapper.h"
#endif
#ifdef VTK_USE_SBR
#include "vtkStarbasePolyDataMapper.h"
#endif
#ifdef VTK_USE_XGLR
#include "vtkXGLPolyDataMapper.h"
#endif
#ifdef _WIN32
#include "vtkOpenGLPolyDataMapper.h"
#endif
// return the correct type of PolyDataMapper 
vtkPolyDataMapper *vtkPolyDataMapper::New()
{
  char *temp = vtkRenderWindow::GetRenderLibrary();
  
#ifdef VTK_USE_SBR
  if (!strcmp("Starbase",temp)) return vtkStarbasePolyDataMapper::New();
#endif
#ifdef VTK_USE_GLR
  if (!strcmp("GL",temp)) return vtkGLPolyDataMapper::New();
#endif
#ifdef VTK_USE_OGLR
  if (!strcmp("OpenGL",temp)) return vtkOpenGLPolyDataMapper::New();
#endif
#ifdef _WIN32
  if (!strcmp("Win32OpenGL",temp)) return vtkOpenGLPolyDataMapper::New();
#endif
#ifdef VTK_USE_XGLR
  if (!strcmp("XGL",temp)) return vtkXGLPolyDataMapper::New();
#endif
  
  return new vtkPolyDataMapper;
}

void vtkPolyDataMapper::SetInput(vtkPolyData *in)
{
  if (in != this->Input )
    {
    vtkDebugMacro(<<" setting Input to " << (void *)in);
    this->Input = (vtkDataSet *) in;
    this->Modified();
    }
}

//
// Return bounding box of data
//
float *vtkPolyDataMapper::GetBounds()
{
  static float bounds[] = {-1.0,1.0, -1.0,1.0, -1.0,1.0};

  if ( ! this->Input ) 
    return bounds;
  else
    {
    this->Input->Update();
    return this->Input->GetBounds();
    }
}


