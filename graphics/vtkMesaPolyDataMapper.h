/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaPolyDataMapper.h
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
// .NAME vtkMesaPolyDataMapper - a PolyDataMapper for the Mesa library
// .SECTION Description
// vtkMesaPolyDataMapper is a subclass of vtkPolyDataMapper.
// vtkMesaPolyDataMapper is a geometric PolyDataMapper for the Mesa 
// rendering library.

#ifndef __vtkMesaPolyDataMapper_h
#define __vtkMesaPolyDataMapper_h

#include "vtkPolyDataMapper.h"
#include <stdlib.h>

#ifdef VTK_MANGLE_MESA
#define USE_MGL_NAMESPACE
#include "mesagl.h"
#else
#include "GL/gl.h"
#endif

class vtkProperty;
class vtkRenderWindow;
class vtkMesaRenderer;

class VTK_EXPORT vtkMesaPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkMesaPolyDataMapper *New();
  vtkTypeMacro(vtkMesaPolyDataMapper,vtkPolyDataMapper);

  // Description:
  // Implement superclass render method.
  void Render(vtkRenderer *ren, vtkActor *a);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Draw method for Mesa.
  virtual void Draw(vtkRenderer *ren, vtkActor *a);
  
  //BTX  begin tcl exclude
  
  // Description:
  // Get the lmcolor property, this is a pretty important little 
  // function.  It determines how vertex colors will be handled  
  // in gl.  When a PolyDataMapper has vertex colors it will use this 
  // method to determine what lmcolor mode to set.               
  GLenum GetLmcolorMode(vtkProperty *prop);
  //ETX

protected:
  vtkMesaPolyDataMapper();
  ~vtkMesaPolyDataMapper();
  vtkMesaPolyDataMapper(const vtkMesaPolyDataMapper&) {};
  void operator=(const vtkMesaPolyDataMapper&) {};

  int ListId;
  vtkRenderWindow *RenderWindow;   // RenderWindow used for the previous render
};

#endif
