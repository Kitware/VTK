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
#include "Renderer.hh"

class vlRenderMaster : public vlObject
{
 public:
  vlRenderMaster();
  vlRenderer *MakeRenderer(char *ren);
  vlRenderer *MakeRenderer(void);
};

#endif
