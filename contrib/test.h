/*=========================================================================

  Program:   Visualization Toolkit
  Module:    test.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1999 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkOpenGLOffscreenRenderWindow - Render OpenGL scene offscreen.
// .SECTION Description
// Renders the entire 3D scene without need of an on-screen window.  
// This is very useful for client-server visualization. 
// .SECTION Caveats
//  There are two ways of doing this.  The most generic method is to 
//  render to an offscreen XPixmap and copy the pixels back to the 
//  client application.  While this implementation completely conforms
//  to the GLX standard, it forces the rendering to occur in software.
//  The SGI GLXPbuffer extension can be used to take advantage of
//  any available hardware graphics acceleration on the client side.
//  However it is very sgi-specific and there is no guarantee that
//  you will be able to find a properly matching visual type (this
//  is my fault because the matching is pretty rudimentary right now).
//  
//  Unfortunately these two modes of operation must be selected at 
//  compilation time.  Use Pbuffers by defining USE_PBUFFER.
//  (actually PBUFFERS is the only thing working right now...
//  Also some debugging information could not use the vtkDebugMacro()
//  so you must #define DEBUG_OFFSCREEN in order to turn on verbose
//  debugging.
//
// .SECTION see also
// vtkOpenGLRenderWindow, vtkWin32OffscreenRenderWindow

#ifndef __vtkOpenGLOffscreenRenderWindow_h
#define __vtkOpenGLOffscreenRenderWindow_h

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vtkXRenderWindow.h"
#include "GL/glx.h"

class VTK_EXPORT vtkOpenGLOffscreenRenderWindow : public vtkXRenderWindow
{
protected:
  GLXContext ContextId;
  int MultiSamples;
  long OldMonitorSetting;
  Pixmap pixmap;
  void Destroy(void);
public:
  vtkOpenGLOffscreenRenderWindow();
  ~vtkOpenGLOffscreenRenderWindow();
  static void WritePPMXImage(char *filename,XImage *img);
  static void WritePPMPbuffer(char *filename,int width,int height,unsigned int *pbuffer);
  static vtkOpenGLOffscreenRenderWindow *New() {return new vtkOpenGLOffscreenRenderWindow;};
  const char *GetClassName() {return "vtkOpenGLOffscreenRenderWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  void Start(void);
  void Frame(void);
  void WindowConfigure(void);
  void WindowInitialize(void);
  void Initialize(void);
  virtual void SetFullScreen(int arg = 1);
  void WindowRemap(void);
  void PrefFullScreen(void);
  void SetSize(int,int);
  void SetPosition(int x,int y);
  int     *GetSize();
  virtual int      GetDesiredDepth();
  virtual Colormap GetDesiredColormap();
  virtual Visual  *GetDesiredVisual();
  XVisualInfo     *GetDesiredVisualInfo();

  vtkSetMacro(MultiSamples,int);
  vtkGetMacro(MultiSamples,int);

  // stereo rendering stuff
  virtual void StereoUpdate();

  // get contents of frame buffer as dlImage ...
  XImage *GetImage();
  void WriteImage(char *filename);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *,int front);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBARGBA... 
  virtual float *GetRGBAPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetRGBAPixelData(int x,int y,int x2,int y2,float *,int front);

  // Description:
  // Set/Get the zbuffer data from an image
  virtual float *GetZbufferData( int x1, int y1, int x2, int y2 );
  virtual void SetZbufferData( int x1, int y1, int x2, int y2, float *buffer );
  void MakeCurrent();
};

#endif
