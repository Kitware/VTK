/*=========================================================================

  Program:   Visualization Toolkit
  Module:    RenderM.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkRenderMaster - create device specific render window
// .SECTION Description
// vtkRenderMaster is used to create device specific rendering window.
// vtkRenderMaster interfaces with the operating system to determine
// which type of rendering library to use. If the environment variable
// VTK_RENDERER is set, then that rendering library is used. Otherwise
// the internal software rendering library kgl is used.

#ifndef __vtkRenderMaster_hh
#define __vtkRenderMaster_hh

#include "Object.hh"
#include "RenderW.hh"
#include "Interact.hh"

class vtkRenderMaster : public vtkObject
{
 public:
  vtkRenderMaster();
  char *GetClassName() {return "vtkRenderMaster";};
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkRenderWindow *MakeRenderWindow(char *ren);
  vtkRenderWindow *MakeRenderWindow(void);
};

#endif
