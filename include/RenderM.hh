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
#ifndef __vlRenderMaster_hh
#define __vlRenderMaster_hh

#include "Object.hh"
#include "RenderW.hh"

class vlRenderMaster : public vlObject
{
 public:
  vlRenderMaster();
  char *GetClassName() {return "vlRenderMaster";};
  // no ivars so no printself  void PrintSelf(ostream& os, vlIndent indent);
  vlRenderWindow *MakeRenderWindow(char *ren);
  vlRenderWindow *MakeRenderWindow(void);
};

#endif
