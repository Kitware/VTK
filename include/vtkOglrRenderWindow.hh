/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOglrRenderWindow.hh
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
// .NAME vtkOglrRenderWindow - OpenGL rendering window
// .SECTION Description
// vtkOglrRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkOglrRenderer interfaces to the OpenGL graphics library.

#ifndef __vtkOglrRenderWindow_hh
#define __vtkOglrRenderWindow_hh

#include <stdlib.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include "vtkXRenderWindow.hh"
#include "GL/glx.h"

class vtkOglrRenderWindow : public vtkXRenderWindow
{
protected:
  GLXContext ContextId;
  int MultiSamples;
  long OldMonitorSetting;

public:
  vtkOglrRenderWindow();
  ~vtkOglrRenderWindow();
  char *GetClassName() {return "vtkOglrRenderWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  vtkRenderer  *MakeRenderer();
  vtkLightDevice     *MakeLight();
  vtkCameraDevice    *MakeCamera();
  vtkActorDevice     *MakeActor();
  vtkTextureDevice   *MakeTexture();
  vtkPropertyDevice  *MakeProperty();
  vtkPolyMapperDevice *MakePolyMapper();

  void Start(void);
  void Frame(void);
  void WindowConfigure(void);
  void WindowInitialize(void);
  void Initialize(void);
  virtual void SetFullScreen(int);
  void WindowRemap(void);
  void PrefFullScreen(void);
  void SetSize(int,int);

  virtual int      GetDesiredDepth();
  virtual Colormap GetDesiredColormap();
  virtual Visual  *GetDesiredVisual();
  XVisualInfo     *GetDesiredVisualInfo();

  vtkSetMacro(MultiSamples,int);
  vtkGetMacro(MultiSamples,int);

  // stereo rendering stuff
  virtual void StereoUpdate();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *,int front);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBARGBA... 
  virtual unsigned char *GetRGBAPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetRGBAPixelData(int x,int y,int x2,int y2,unsigned char *,int front);

  // Description:
  // Set/Get the zbuffer data from an image
  virtual float *GetZbufferData( int x1, int y1, int x2, int y2 );
  virtual void SetZbufferData( int x1, int y1, int x2, int y2, float *buffer );

  void MakeCurrent();
};

#endif
