/*=========================================================================

  Program:   OSCAR 
  Module:    RenderW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Description:
---------------------------------------------------------------------------
This file is part of the vis library

- Ken Martin

=========================================================================*/
#ifndef __vlRenderWindow_hh
#define __vlRenderWindow_hh

#include "Object.hh"
#include "RenderC.hh"

class vlRenderWindow : public vlObject
{
public:
  vlRendererCollection Renderers;
  char name[80];

public:
  vlRenderWindow();
  virtual char *GetClassName() {return "vlRenderWindow";};
  void AddRenderers(vlRenderer *);
  virtual void Render();
  virtual void Frame() = 0;
  virtual void Start() = 0;
  virtual vlRenderer  *MakeRenderer() = 0;
  virtual vlActor     *MakeActor() = 0;
  virtual vlLight     *MakeLight() = 0;
  virtual vlCamera    *MakeCamera() = 0;
};

#endif
