/*=========================================================================

  Program:   Visualization Library
  Module:    RenderM.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlRenderMaster - create device specific render window
// .SECTION Description
// vlRenderMaster is used to create device specific rendering window.
// vlRenderMaster interfaces with the operating system to determine
// which type of rendering library to use. If the environment variable
// VL_RENDERER is set, then that rendering library is used. Otherwise
// the internal software rendering library kgl is used.

#ifndef __vlRenderMaster_hh
#define __vlRenderMaster_hh

#include "Object.hh"
#include "RenderW.hh"
#include "Interact.hh"

class vlRenderMaster : public vlObject
{
 public:
  vlRenderMaster();
  char *GetClassName() {return "vlRenderMaster";};
  void PrintSelf(ostream& os, vlIndent indent);

  vlRenderWindow *MakeRenderWindow(char *ren);
  vlRenderWindow *MakeRenderWindow(void);
};

#endif
