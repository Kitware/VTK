/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaPolyDataMapper.h
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
// .NAME vtkMesaPolyDataMapper - a PolyDataMapper for the Mesa library
// .SECTION Description
// vtkMesaPolyDataMapper is a subclass of vtkPolyDataMapper.
// vtkMesaPolyDataMapper is a geometric PolyDataMapper for the Mesa 
// rendering library.

#ifndef __vtkMesaPolyDataMapper_h
#define __vtkMesaPolyDataMapper_h

#include "vtkPolyDataMapper.h"

class vtkProperty;
class vtkRenderWindow;
class vtkMesaRenderer;
class vtkTimerLog;

class VTK_RENDERING_EXPORT vtkMesaPolyDataMapper : public vtkPolyDataMapper
{
public:
  static vtkMesaPolyDataMapper *New();
  vtkTypeRevisionMacro(vtkMesaPolyDataMapper,vtkPolyDataMapper);

  // Description:
  // Implement superclass render method.
  virtual void RenderPiece(vtkRenderer *ren, vtkActor *a);

  // Description:
  // Release any graphics resources that are being consumed by this mapper.
  // The parameter window could be used to determine which graphic
  // resources to release.
  void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // Draw method for Mesa.
  virtual int Draw(vtkRenderer *ren, vtkActor *a);
  
protected:
  vtkMesaPolyDataMapper();
  ~vtkMesaPolyDataMapper();

  int ListId;
  vtkRenderWindow *RenderWindow;   // RenderWindow used for the previous render
private:
  vtkMesaPolyDataMapper(const vtkMesaPolyDataMapper&);  // Not implemented.
  void operator=(const vtkMesaPolyDataMapper&);  // Not implemented.
};

#endif
