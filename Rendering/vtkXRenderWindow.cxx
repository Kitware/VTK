/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXRenderWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkXRenderWindow.h"
#include "vtkXRenderWindowInteractor.h"
#include "vtkObjectFactory.h"

vtkXRenderWindow::vtkXRenderWindow()
{
  vtkDebugMacro(<< "vtkXRenderWindow::vtkXRenderWindow");
  this->DisplayId = (Display *)NULL;
  this->WindowId = (Window)NULL;
  this->ParentId = (Window)NULL;
  this->NextWindowId = (Window)NULL;
  this->ColorMap = (Colormap)NULL;
  this->ScreenSize[0] = 0;
  this->ScreenSize[1] = 0;
  this->OwnDisplay = 0;
  this->CursorHidden = 0;
}

vtkXRenderWindow::~vtkXRenderWindow()
{
  vtkDebugMacro(<< "vtkXRenderWindow::~vtkXRenderWindow");
  if (this->DisplayId)
    {
    XSync(this->DisplayId,0);
    }
  // if we create the display, we'll delete it
  if (this->OwnDisplay && this->DisplayId)
    {
    XCloseDisplay(this->DisplayId);
    this->DisplayId = NULL;
    }
}

int vtkXRenderWindowFoundMatch;

Bool vtkXRenderWindowPredProc(Display *vtkNotUsed(disp), XEvent *event, 
			      char *arg)
{
  Window win = (Window)arg;
  
  if ((((XAnyEvent *)event)->window == win) &&
      ((event->type == ButtonPress)))
    vtkXRenderWindowFoundMatch = 1;

  return 0;
  
}

void *vtkXRenderWindow::GetGenericContext()
{
  static GC gc = (GC) NULL; 

  if (!gc) gc = XCreateGC(this->DisplayId, this->WindowId, 0, 0);

  return (void *) gc;

}

int vtkXRenderWindow::GetEventPending()
{
  XEvent report;
  
  vtkXRenderWindowFoundMatch = 0;
  XCheckIfEvent(this->DisplayId, &report, vtkXRenderWindowPredProc, 
		(char *)this->WindowId);
  return vtkXRenderWindowFoundMatch;
}


// Get the size of the screen in pixels
int *vtkXRenderWindow::GetScreenSize()
{
  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    else
      {
      this->OwnDisplay = 1;
      }
    }

  this->ScreenSize[0] = 
    DisplayWidth(this->DisplayId, DefaultScreen(this->DisplayId));
  this->ScreenSize[1] = 
    DisplayHeight(this->DisplayId, DefaultScreen(this->DisplayId));

  return this->ScreenSize;
}

// Get the current size of the window in pixels.
int *vtkXRenderWindow::GetSize(void)
{  
  return this->Size;
}

// Get the position in screen coordinates (pixels) of the window.
int *vtkXRenderWindow::GetPosition(void)
{
  XWindowAttributes attribs;
  int x,y;
  Window child;
  
  // if we aren't mapped then just return the ivar 
  if (!this->Mapped)
    {
    return(this->Position);
    }

  //  Find the current window size 
  XGetWindowAttributes(this->DisplayId, this->WindowId, &attribs);
  x = attribs.x;
  y = attribs.y;

  XTranslateCoordinates(this->DisplayId,this->WindowId,
		 RootWindowOfScreen(ScreenOfDisplay(this->DisplayId,0)),
			x,y,&this->Position[0],&this->Position[1],&child);

  return this->Position;
}

// Get this RenderWindow's X display id.
Display *vtkXRenderWindow::GetDisplayId()
{
  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    this->OwnDisplay = 1;
    }
  vtkDebugMacro(<< "Returning DisplayId of " << (void *)this->DisplayId << "\n"); 

  return this->DisplayId;
}

// Get this RenderWindow's parent X window id.
Window vtkXRenderWindow::GetParentId()
{
  vtkDebugMacro(<< "Returning ParentId of " << (void *)this->ParentId << "\n");
  return this->ParentId;
}

// Get this RenderWindow's X window id.
Window vtkXRenderWindow::GetWindowId()
{
  vtkDebugMacro(<< "Returning WindowId of " << (void *)this->WindowId << "\n");
  return this->WindowId;
}

// Move the window to a new position on the display.
void vtkXRenderWindow::SetPosition(int x, int y)
{
  // if we aren't mapped then just set the ivars
  if (!this->Mapped)
    {
    if ((this->Position[0] != x)||(this->Position[1] != y))
      {
      this->Modified();
      }
    this->Position[0] = x;
    this->Position[1] = y;
    return;
    }

  XMoveWindow(this->DisplayId,this->WindowId,x,y);
  XSync(this->DisplayId,False);
}

// Sets the parent of the window that WILL BE created.
void vtkXRenderWindow::SetParentId(Window arg)
{
  if (this->ParentId)
    {
    vtkErrorMacro("ParentId is already set.");
    return;
    }
  
  vtkDebugMacro(<< "Setting ParentId to " << (void *)arg << "\n"); 

  this->ParentId = arg;
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkXRenderWindow::SetWindowId(Window arg)
{
  vtkDebugMacro(<< "Setting WindowId to " << (void *)arg << "\n"); 

  this->WindowId = arg;

  if (this->CursorHidden)
    {
    this->CursorHidden = 0;
    this->HideCursor();
    } 
}

// Set this RenderWindow's X window id to a pre-existing window.
void vtkXRenderWindow::SetWindowInfo(char *info)
{
  int tmp;
  
  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    else
      {
      this->OwnDisplay = 1;
      }
    }

  sscanf(info,"%i",&tmp);
 
  this->SetWindowId(tmp);
}

// Sets the X window id of the window that WILL BE created.
void vtkXRenderWindow::SetParentInfo(char *info)
{
  int tmp;
  
  // get the default display connection 
  if (!this->DisplayId)
    {
    this->DisplayId = XOpenDisplay((char *)NULL); 
    if (this->DisplayId == NULL) 
      {
      vtkErrorMacro(<< "bad X server connection.\n");
      }
    else
      {
      this->OwnDisplay = 1;
      }
    }

  sscanf(info,"%i",&tmp);
 
  this->SetParentId(tmp);
}

void vtkXRenderWindow::SetWindowId(void *arg)
{
  this->SetWindowId((Window)arg);
}
void vtkXRenderWindow::SetParentId(void *arg)
{
  this->SetParentId((Window)arg);
}


void vtkXRenderWindow::SetWindowName(char * name)
{
  XTextProperty win_name_text_prop;

  vtkRenderWindow::SetWindowName( name );

  if (this->Mapped)
    {
    if( XStringListToTextProperty( &name, 1, &win_name_text_prop ) == 0 )
      {
      XFree (win_name_text_prop.value);
      vtkWarningMacro(<< "Can't rename window"); 
      return;
      }
    
    XSetWMName( this->DisplayId, this->WindowId, &win_name_text_prop );
    XSetWMIconName( this->DisplayId, this->WindowId, &win_name_text_prop );
    XFree (win_name_text_prop.value);
    }
}


// Specify the X window id to use if a WindowRemap is done.
void vtkXRenderWindow::SetNextWindowId(Window arg)
{
  vtkDebugMacro(<< "Setting NextWindowId to " << (void *)arg << "\n"); 

  this->NextWindowId = arg;
}

// Set the X display id for this RenderWindow to use to a pre-existing 
// X display id.
void vtkXRenderWindow::SetDisplayId(Display  *arg)
{
  vtkDebugMacro(<< "Setting DisplayId to " << (void *)arg << "\n"); 

  this->DisplayId = arg;
  this->OwnDisplay = 0;
}
void vtkXRenderWindow::SetDisplayId(void *arg)
{
  this->SetDisplayId((Display *)arg);
  this->OwnDisplay = 0;
}


void vtkXRenderWindow::Render()
{
  XWindowAttributes attribs;

  // To avoid the expensive XGetWindowAttributes call,
  // compute size at the start of a render and use
  // the ivar other times.
  if (this->Mapped)
    {
    //  Find the current window size 
    XGetWindowAttributes(this->DisplayId, 
		                    this->WindowId, &attribs);

    this->Size[0] = attribs.width;
    this->Size[1] = attribs.height;
    }

  // Now do the superclass stuff
  this->vtkRenderWindow::Render();
}

void vtkXRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkRenderWindow::PrintSelf(os,indent);

  os << indent << "Color Map: " << this->ColorMap << "\n";
  os << indent << "Display Id: " << this->GetDisplayId() << "\n";
  os << indent << "Next Window Id: " << this->NextWindowId << "\n";
  os << indent << "Window Id: " << this->GetWindowId() << "\n";
}

vtkXRenderWindow * vtkXRenderWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkXRenderWindow");
  if(ret)
    {
    return (vtkXRenderWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.

  return (vtkXRenderWindow *)(vtkRenderWindow::New());
}

//----------------------------------------------------------------------------
void vtkXRenderWindow::HideCursor()
{
  static char blankBits[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  
  static XColor black = { 0, 0, 0, 0, 0, 0 };
 
  if (!this->DisplayId || !this->WindowId)
    {
    this->CursorHidden = 1;
    }
  else if (!this->CursorHidden)
    {
    Pixmap blankPixmap = XCreateBitmapFromData(this->DisplayId,
					       this->WindowId,
					       blankBits, 16, 16);
    
    Cursor blankCursor = XCreatePixmapCursor(this->DisplayId, blankPixmap,
					     blankPixmap, &black, &black,
					     7, 7);
    
    XDefineCursor(this->DisplayId, this->WindowId, blankCursor);
    
    XFreePixmap(this->DisplayId, blankPixmap);
    
    this->CursorHidden = 1;
    }
}

//----------------------------------------------------------------------------
void vtkXRenderWindow::ShowCursor()
{
  if (!this->DisplayId || !this->WindowId)
    {
    this->CursorHidden = 0;
    }
  else if (this->CursorHidden)
    {
    XUndefineCursor(this->DisplayId, this->WindowId);
    this->CursorHidden = 0;
    }
}				   
