/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOglrPolyMapper.h
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
// .NAME vtkOglrPolyMapper - a PolyMapper for the OpenGL library
// .SECTION Description
// vtkOglrPolyMapper is a subclass of vtkPolyMapperDevice. 
// vtkOglrPolyMapper is a geometric PolyMapper for the OpenGL 
// rendering library.

#ifndef __vtkOglrPolyMapper_h
#define __vtkOglrPolyMapper_h

#include <stdlib.h>

#ifdef _WIN32
#include <afxwin.h>
#endif
#include <GL/gl.h>

#include "vtkPolyMapperDevice.h"

class vtkOglrRenderer;
class vtkProperty;

class VTK_EXPORT vtkOglrPolyMapper : public vtkPolyMapperDevice
{
 public:
  vtkOglrPolyMapper();
  char *GetClassName() {return "vtkOglrPolyMapper";};

  void Build(vtkPolyData *, vtkColorScalars *);
  void Draw(vtkRenderer *ren, vtkActor *a);
  
  //BTX  begine tcl exclude
  GLenum GetLmcolorMode(vtkProperty *prop);
  //ETX
};

#endif
