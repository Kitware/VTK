/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkQuartzImageMapper.h
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
// .NAME vtkQuartzImageMapper - 2D image display support for Quartz windows
// .SECTION Description
// vtkQuartzImageMapper is a concrete subclass of vtkImageMapper that
// renders images under Quartz.

// .SECTION See Also
// vtkImageMapper

#ifndef __vtkQuartzImageMapper_h
#define __vtkQuartzImageMapper_h


#include "vtkImageMapper.h"
class vtkImageActor2D;


class VTK_RENDERING_EXPORT vtkQuartzImageMapper : public vtkImageMapper
{
public:
  static vtkQuartzImageMapper *New();
  vtkTypeRevisionMacro(vtkQuartzImageMapper,vtkImageMapper);
  
  // Description:
  // Handle the render method.
  void RenderOverlay(vtkViewport* viewport, vtkActor2D* actor) {
    this->RenderStart(viewport,actor);}

  // Description:
  // Called by the Render function in vtkImageMapper.  Actually draws
  // the image to the screen.
  void RenderData(vtkViewport* viewport, vtkImageData* data, 
                  vtkActor2D* actor);

  unsigned char *DataOut;       // the data in the DIBSection
  void *HBitmap;                        // our handle to the DIBSection

protected:
  vtkQuartzImageMapper();
  ~vtkQuartzImageMapper();

private:
  vtkQuartzImageMapper(const vtkQuartzImageMapper&) {};  // Not implemented.
  void operator=(const vtkQuartzImageMapper&) {};  // Not implemented.
};


#endif









