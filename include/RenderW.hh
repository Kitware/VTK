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
  char Name[80];
  int   Size[2];
  int   Position[2];
  int   Borders;
  int   FullScreen;
  int   OldScreen[5];
  int   Mapped;
  int   DoubleBuffer;

public:
  vlRenderWindow();
  char *GetClassName() {return "vlRenderWindow";};
  void AddRenderers(vlRenderer *);
  virtual void Render();
  virtual void Frame() = 0;
  virtual void Start() = 0;
  virtual vlRenderer  *MakeRenderer() = 0;
  virtual vlActor     *MakeActor() = 0;
  virtual vlLight     *MakeLight() = 0;
  virtual vlCamera    *MakeCamera() = 0;
  virtual int *GetPosition() = 0;
  virtual int *GetSize() = 0;
  virtual void SetSize(int,int) = 0;
  virtual void SetSize(int a[2]);

  virtual void SetFullScreen(int) = 0;
  vlGetMacro(FullScreen,int);
  vlBooleanMacro(FullScreen,int);

  vlSetMacro(Borders,int);
  vlGetMacro(Borders,int);
  vlBooleanMacro(Borders,int);

  vlSetMacro(Mapped,int);
  vlGetMacro(Mapped,int);
  vlBooleanMacro(Mapped,int);

  vlSetMacro(DoubleBuffer,int);
  vlGetMacro(DoubleBuffer,int);
  vlBooleanMacro(DoubleBuffer,int);
};

#endif
