/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaImageMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaImageMapper - 2D image display support for Mesa
// .SECTION Description
// vtkMesaImageMapper is a concrete subclass of vtkImageMapper that
// renders images under Mesa

// .SECTION See Also
// vtkImageMapper

#ifndef __vtkMesaImageMapper_h
#define __vtkMesaImageMapper_h


#include "vtkImageMapper.h"
class vtkActor2D;


class VTK_RENDERING_EXPORT vtkMesaImageMapper : public vtkImageMapper
{
public:
  static vtkMesaImageMapper *New();
  vtkTypeMacro(vtkMesaImageMapper,vtkImageMapper);
  virtual void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Handle the render method.
  void RenderOpaqueGeometry(vtkViewport* viewport, vtkActor2D* actor) {
    this->RenderStart(viewport,actor);}

  // Description:
  // Called by the Render function in vtkImageMapper.  Actually draws
  // the image to the screen.
  void RenderData(vtkViewport* viewport, vtkImageData* data, 
                  vtkActor2D* actor);

protected:
  vtkMesaImageMapper();
  ~vtkMesaImageMapper();

private:
  vtkMesaImageMapper(const vtkMesaImageMapper&);  // Not implemented.
  void operator=(const vtkMesaImageMapper&);  // Not implemented.
};


#endif









