/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaImageWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaImageWindow - Mesa Imageing window
// .SECTION Description
// vtkMesaImageWindow is a concrete implementation of the abstract
// class vtkImageWindow. vtkMesaImageer interfaces to the standard
// Mesa graphics library in the Windows/NT environment..

#ifndef __vtkMesaImageWindow_h
#define __vtkMesaImageWindow_h

#include "MangleMesaInclude/glx_mangle.h"
#include "MangleMesaInclude/glx.h"
#include "MangleMesaInclude/osmesa.h"

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vtkToolkits.h"
#include "vtkXImageWindow.h"



class VTK_RENDERING_EXPORT vtkMesaImageWindow : public vtkXImageWindow
{
public:
  static vtkMesaImageWindow *New();
  vtkTypeRevisionMacro(vtkMesaImageWindow,vtkXImageWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Initialize the window for rendering.
  virtual void MakeDefaultWindow();

  // Description:
  // Swap the front and back buffers if double buffering is being used.
  void SwapBuffers();

  // Description:
  // Flush the buffer and swap if necessary.
  void Frame();

  // Description:
  // Draw the contents of the window
  void Render();

  // Description:
  // Xwindow get set functions
  virtual void *GetGenericDisplayId() {return (void *)this->DisplayId;};
  virtual void *GetGenericWindowId();
  virtual void *GetGenericParentId()  {return (void *)this->ParentId;};
  virtual void *GetGenericContext();
  virtual void *GetGenericDrawable()  {return (void *)this->WindowId;};
  
  // Description:
  // Get the X properties of an ideal rendering window.
  virtual Colormap GetDesiredColormap();
  virtual Visual  *GetDesiredVisual();
  XVisualInfo     *GetDesiredVisualInfo();
  virtual int      GetDesiredDepth();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *,
                            int front);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBARGBA... 
  virtual float *GetRGBAPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetRGBAPixelData(int x,int y,int x2,int y2,float *,int front,
                                int blend=0);

  // Description:
  // Make this windows Mesa context the current context.
  void MakeCurrent();

  // Description:
  // Erase the window contents 
  virtual void EraseWindow() {this->vtkImageWindow::EraseWindow();};

  // Description:
  // Override the default implementation so that we can actively switch between
  // on and offscreen rendering.
  virtual void SetOffScreenRendering(int i);

protected:
  vtkMesaImageWindow();
  ~vtkMesaImageWindow();

  GLXContext ContextId;
  OSMesaContext OffScreenContextId;
  void *OffScreenWindow;
  int ScreenDoubleBuffer;
  int ScreenMapped;
private:
  vtkMesaImageWindow(const vtkMesaImageWindow&);  // Not implemented.
  void operator=(const vtkMesaImageWindow&);  // Not implemented.
};


#endif

