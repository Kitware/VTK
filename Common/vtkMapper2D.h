/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper2D.h
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
// .NAME vtkMapper2D
// .SECTION Description
// vtkMapper2D is an abstract class which defines the interface for objects
// which render two dimensional actors (vtkActor2D).

// .SECTION See Also
// vtkActor2D

#ifndef __vtkMapper2D_h
#define __vtkMapper2D_h

#include "vtkAbstractMapper.h"
#include "vtkWindow.h"

class vtkViewport;
class vtkActor2D;

class VTK_COMMON_EXPORT vtkMapper2D : public vtkAbstractMapper
{
public:
  static vtkMapper2D* New();

  vtkTypeRevisionMacro(vtkMapper2D,vtkAbstractMapper);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  virtual void RenderOverlay(vtkViewport*, vtkActor2D*) {};
  virtual void RenderOpaqueGeometry(vtkViewport*, vtkActor2D*) {};
  virtual void RenderTranslucentGeometry(vtkViewport*, vtkActor2D*) {};

protected:
  vtkMapper2D() {};
  ~vtkMapper2D() {};

private:
  vtkMapper2D(const vtkMapper2D&);  // Not implemented.
  void operator=(const vtkMapper2D&);  // Not implemented.
};

#endif



