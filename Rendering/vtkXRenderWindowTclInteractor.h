/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindowTclInteractor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXRenderWindowTclInteractor - a TCL event driven interface for a RenderWindow
// .SECTION Description
// vtkXRenderWindowTclInteractor is a convenience object that provides event
// bindings to common graphics functions. For example, camera and actor
// functions such as zoom-in/zoom-out, azimuth, roll, and pan. It is one of
// the window system specific subclasses of vtkRenderWindowInteractor. Please
// see vtkRenderWindowInteractor documentation for event bindings.
//
// .SECTION see also
// vtkRenderWindowInteractor vtkXRenderWindowInteractor vtkXRenderWindow

#ifndef __vtkXRenderWindowTclInteractor_h
#define __vtkXRenderWindowTclInteractor_h

#include "vtkXRenderWindowInteractor.h"

class VTK_RENDERING_EXPORT vtkXRenderWindowTclInteractor : public vtkXRenderWindowInteractor
{
public:
  static vtkXRenderWindowTclInteractor *New();
  vtkTypeRevisionMacro(vtkXRenderWindowTclInteractor,vtkXRenderWindowInteractor);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkXRenderWindowTclInteractor();
  ~vtkXRenderWindowTclInteractor();

private:
  vtkXRenderWindowTclInteractor(const vtkXRenderWindowTclInteractor&);  // Not implemented.
  void operator=(const vtkXRenderWindowTclInteractor&);  // Not implemented.
};

#endif
