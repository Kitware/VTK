/*=========================================================================

  Program:   Visualization Library
  Module:    XInter.cc
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file
or its contents may be copied, reproduced or altered in any way
without the express written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
#include <stdio.h>
#include <stdlib.h>
#include <X11/X.h>
#include <X11/keysym.h>
#include "XInter.hh"
#include "Actor.hh"
#include <X11/Shell.h>

typedef struct
{
  Visual	*visual;
  int	depth;
} OptionsRec;
OptionsRec	Options;

XtResource resources[] =
{
	{"visual", "Visual", XtRVisual, sizeof (Visual *),
	XtOffsetOf (OptionsRec, visual), XtRImmediate, NULL},
	{"depth", "Depth", XtRInt, sizeof (int),
	XtOffsetOf (OptionsRec, depth), XtRImmediate, NULL},
};

XrmOptionDescRec Desc[] =
{
	{"-visual", "*visual", XrmoptionSepArg, NULL},
	{"-depth", "*depth", XrmoptionSepArg, NULL}
};


// states
#define VLXI_START 0
#define VLXI_ROTATE 1


// Description:
// Construct object so that light follows camera motion.
vlXInteractiveRenderer::vlXInteractiveRenderer()
{
  this->State = VLXI_START;
}

vlXInteractiveRenderer::~vlXInteractiveRenderer()
{
}

void  vlXInteractiveRenderer::Start()
{
  XtAppMainLoop(this->App);
}

// Description:
// Begin processing keyboard strokes.
void vlXInteractiveRenderer::Initialize()
{
  Display *display;
  Window      window;
  static int any_initialized = 0;
  static XtAppContext app;
  vlXRenderWindow *ren;
  int depth;
  Colormap cmap;
  Visual  *vis;
  int *size;
  int *position;
  int argc = 0;

  // make sure we have a RenderWindow and camera
  if ( ! this->RenderWindow)
    {
    vlErrorMacro(<<"No renderer defined!");
    return;
    }

  /* do initialization stuff if not initialized yet */
  if (!any_initialized)
    {
    XtToolkitInitialize();
    app = XtCreateApplicationContext();
    any_initialized = 1;
    }
  this->App = app;

  display = XtOpenDisplay(this->App,NULL,"VL","vl",NULL,0,&argc,NULL);

  // get the info we need from the RenderingWindow
  ren = (vlXRenderWindow *)(this->RenderWindow);
  ren->SetDisplayId(display);
  depth   = ren->GetDesiredDepth();
  cmap    = ren->GetDesiredColormap();
  vis     = ren->GetDesiredVisual();
  size    = ren->GetSize();
  position= ren->GetPosition();

  this->top = XtVaAppCreateShell(this->RenderWindow->Name,"vl",
				 applicationShellWidgetClass,
				 display,
				 XtNdepth, depth,
				 XtNcolormap, cmap,
				 XtNvisual, vis,
				 XtNx, position[0],
				 XtNy, position[1],
				 XtNwidth, size[0],
				 XtNheight, size[1],
				 XtNmappedWhenManaged, 0,
				 NULL);

  XtRealizeWidget(this->top);

  /* add callback */
  XSync(display,False);
  ren->SetWindowId(XtWindow(this->top));
  XtAddEventHandler(this->top,
		    KeyPressMask | ButtonPressMask | ExposureMask |
		    StructureNotifyMask | ButtonReleaseMask,
		    False,vlXInteractiveRendererCallback,(XtPointer)this);
  this->UpdateSize(size[0],size[1]);
}

void vlXInteractiveRenderer::PrintSelf(ostream& os, vlIndent indent)
{
  if (this->ShouldIPrint(vlXInteractiveRenderer::GetClassName()))
    {
    vlInteractiveRenderer::PrintSelf(os,indent);
    }
}


void  vlXInteractiveRenderer::UpdateSize(int x,int y)
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0])&&(y != this->Size[1]))
    {
    this->RenderWindow->SetSize(x,y);
    }

  this->Size[0] = x;
  this->Size[1] = y;
  this->Center[0] = x/2.0;
  this->Center[1] = y/2.0;
  this->DeltaAzimuth = -20.0/x;
  this->DeltaElevation = 20.0/y;
}
 
void  vlXInteractiveRenderer::StartRotate()
{
  if (this->State != VLXI_START) return;

  this->State = VLXI_ROTATE;
  XtAppAddTimeOut(this->App,10,vlXInteractiveRendererTimer,(XtPointer)this);
}

void  vlXInteractiveRenderer::EndRotate()
{
  if (this->State != VLXI_ROTATE) return;

  this->State = VLXI_START;
}

void vlXInteractiveRendererCallback(Widget w,XtPointer client_data, 
				    XEvent *event, Boolean *ctd)
{
  vlXInteractiveRenderer *me;

  me = (vlXInteractiveRenderer *)client_data;

  switch (event->type) 
    {
    case Expose : me->GetRenderWindow()->Render(); break;
      
    case ConfigureNotify : 
      me->UpdateSize(((XConfigureEvent *)event)->width,
		     ((XConfigureEvent *)event)->height); 
      me->GetRenderWindow()->Render(); 
      break;

    case ButtonPress : 
      {
      switch (((XButtonEvent *)event)->button)
	{
	case Button1 : me->StartRotate(); break;
	}
      }
      break;

    case ButtonRelease : 
      {
      switch (((XButtonEvent *)event)->button)
	{
	case Button1 : me->EndRotate(); break;
	}
      }
      break;

    case KeyPress :
      {
      KeySym ks;
      static char buffer[20];

      XLookupString((XKeyEvent *)event,buffer,20,&ks,NULL);
      switch (ks)
	{
	case XK_e : exit(1); break;
	case XK_w :
	  {
	  vlRendererCollection *rc;
	  vlActorCollection *ac;
	  vlRenderer *aren;
	  vlActor *anActor;
	  
	  rc = me->RenderWindow->GetRenderers();
	  
	  for (rc->InitTraversal(); aren = rc->GetNextItem(); )
	    {
	    ac = aren->GetActors();
	    for (ac->InitTraversal(); anActor = ac->GetNextItem(); )
	      {
	      anActor->GetProperty()->SetWireframe();
	      }
	    }
	  
	  me->RenderWindow->Render();
	  }
	  break;

	case XK_s :
	  {
	  vlRendererCollection *rc;
	  vlActorCollection *ac;
	  vlRenderer *aren;
	  vlActor *anActor;
	  
	  rc = me->RenderWindow->GetRenderers();
	  
	  for (rc->InitTraversal(); aren = rc->GetNextItem(); )
	    {
	    ac = aren->GetActors();
	    for (ac->InitTraversal(); anActor = ac->GetNextItem(); )
	      {
	      anActor->GetProperty()->SetSurface();
	      }
	    }
	  
	  me->RenderWindow->Render();
	  }
	  break;
        }
      }
      break;
    }
}

void vlXInteractiveRendererTimer(XtPointer client_data,XtIntervalId *id)
{
  vlXInteractiveRenderer *me;
  Window root,child;
  int root_x,root_y;
  int x,y;
  float xf,yf;
  unsigned int keys;

  me = (vlXInteractiveRenderer *)client_data;

  switch (me->State)
    {
    case VLXI_ROTATE :
      // get the pointer position
      XQueryPointer(XtDisplay(me->top),XtWindow(me->top),
		    &root,&child,&root_x,&root_y,&x,&y,&keys);
      xf = (x - me->Center[0]) * me->DeltaAzimuth;
      yf = (y - me->Center[1]) * me->DeltaElevation;
      me->Camera->Azimuth(xf);
      me->Camera->Elevation(yf);
      me->Camera->OrthogonalizeViewUp();
      if ( me->Light && me->LightFollowCamera )
	{
	me->Light->SetPosition(me->Camera->GetPosition());
	}
      me->RenderWindow->Render();
      XtAppAddTimeOut(me->App,10,vlXInteractiveRendererTimer,client_data);
      break;

    }
}  
