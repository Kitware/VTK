/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXGLRenderWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkXGLRenderWindow - Suns XGL rendering window
// .SECTION Description
// vtkXGLRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkXGLRenderer interfaces to Suns XGL graphics library.

#ifndef __vtkXGLRenderWindow_h
#define __vtkXGLRenderWindow_h

#include <stdlib.h>
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vtkXRenderWindow.h"
#include <xgl/xgl.h>

class VTK_EXPORT vtkXGLRenderWindow : public vtkXRenderWindow
{
public:
  vtkXGLRenderWindow();
  ~vtkXGLRenderWindow();
  static vtkXGLRenderWindow *New() {return new vtkXGLRenderWindow;};
  const char *GetClassName() {return "vtkXGLRenderWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void Start(void);
  void Frame(void);
  void WindowInitialize(void);
  void Initialize(void);
  virtual void SetFullScreen(int);
  void WindowRemap(void);
  void PrefFullScreen(void);
  void SetSize(int,int);
  void SetSize(int a[2]) {this->SetSize(a[0],a[1]);};

  // stereo rendering stuff
  virtual void StereoUpdate();
  int GetRemapWindow();
  
  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *, int front);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBARGBA... 
  virtual float *GetRGBAPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetRGBAPixelData(int x,int y,int x2,int y2, float *, int front);

  // Description:
  // Set/Get the zbuffer data from an image
  virtual float *GetZbufferData( int x1, int y1, int x2, int y2 );
  virtual void SetZbufferData( int x1, int y1, int x2, int y2, float *buffer);

  // Xwindow stuff
  int      GetDesiredDepth();
  Colormap GetDesiredColormap();
  Visual  *GetDesiredVisual();
  int      CreateXWindow(Display *,int x,int y,int w,int h,int depth,
			 char name[80]);
  Xgl_3d_ctx *GetContext() {return &(this->Context);};
  Xgl_win_ras *GetRaster() {return &(this->WindowRaster);};
  Xgl_ras *GetRas;
  Xgl_ras *SetRas;
  Xgl_3d_ctx GetCtx;

protected:
  Xgl_3d_ctx Context;
  Xgl_win_ras  WindowRaster;
};

#endif
