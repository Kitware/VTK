/*=========================================================================

  Program:   OSCAR 
  Module:    RenderM.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#ifndef __vlRenderMaster_hh
#define __vlRenderMaster_hh

#include "Object.hh"
#include "RenderW.hh"

class vlRenderMaster : public vlObject
{
 public:
  vlRenderMaster();
  virtual char *GetClassName() {return "vlRenderMaster";};
  vlRenderWindow *MakeRenderWindow(char *ren);
  vlRenderWindow *MakeRenderWindow(void);
};

#endif
