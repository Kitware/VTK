/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOpenGLRenderWindow - OpenGL rendering window
// .SECTION Description
// vtkOpenGLRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. vtkOpenGLRenderer interfaces to the OpenGL graphics
// library. Application programmers should normally use vtkRenderWindow
// instead of the OpenGL specific version.

#ifndef __vtkOpenGLRenderWindow_h
#define __vtkOpenGLRenderWindow_h

#include "vtkRenderWindow.h"

#include "vtkOpenGL.h" // Needed for GLuint.

class vtkIdList;

class VTK_RENDERING_EXPORT vtkOpenGLRenderWindow : public vtkRenderWindow
{
protected:
  int MultiSamples;
  long OldMonitorSetting;

public:
  vtkTypeRevisionMacro(vtkOpenGLRenderWindow,vtkRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the maximum number of multisamples
  static void SetGlobalMaximumNumberOfMultiSamples(int val);
  static int  GetGlobalMaximumNumberOfMultiSamples();

  // Description:
  // Set / Get the number of multisamples to use for hardware antialiasing.
  vtkSetMacro(MultiSamples,int);
  vtkGetMacro(MultiSamples,int);

  // Description:
  // Update system if needed due to stereo rendering.
  virtual void StereoUpdate();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2,int front);
  virtual int GetPixelData(int x,int y,int x2,int y2, int front,
                           vtkUnsignedCharArray*);
  virtual int SetPixelData(int x,int y,int x2,int y2,unsigned char *,
                           int front);
  virtual int SetPixelData(int x,int y,int x2,int y2, vtkUnsignedCharArray*,
                           int front);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBARGBA... 
  virtual float *GetRGBAPixelData(int x,int y,int x2,int y2,int front);
  virtual int GetRGBAPixelData(int x,int y,int x2,int y2, int front,
                               vtkFloatArray* data);
  virtual int SetRGBAPixelData(int x,int y,int x2,int y2,float *,int front,
                               int blend=0);
  virtual int SetRGBAPixelData(int x,int y,int x2,int y2, vtkFloatArray*,
                               int front, int blend=0);
  virtual void ReleaseRGBAPixelData(float *data);
  virtual unsigned char *GetRGBACharPixelData(int x,int y,int x2,int y2,
                                              int front);
  virtual int GetRGBACharPixelData(int x,int y,int x2,int y2, int front,
                                   vtkUnsignedCharArray* data);
  virtual int SetRGBACharPixelData(int x,int y,int x2,int y2,unsigned char *,
                                   int front, int blend=0);  
  virtual int SetRGBACharPixelData(int x,int y,int x2,int y2,
                                   vtkUnsignedCharArray *,
                                   int front, int blend=0);  

  // Description:
  // Set/Get the zbuffer data from an image
  virtual float *GetZbufferData( int x1, int y1, int x2, int y2 );
  virtual int GetZbufferData( int x1, int y1, int x2, int y2, float* z );
  virtual int GetZbufferData( int x1, int y1, int x2, int y2,
                              vtkFloatArray* z );
  virtual int SetZbufferData( int x1, int y1, int x2, int y2, float *buffer );
  virtual int SetZbufferData( int x1, int y1, int x2, int y2,
                              vtkFloatArray *buffer );

  // Description:
  // Make this window the current OpenGL context.
  void MakeCurrent() = 0;
  
  // Description:
  // Register a texture name with this render window.
  void RegisterTextureResource (GLuint id);

  // Description:
  // Get the size of the depth buffer.
  int GetDepthBufferSize();
  
  // Description:
  // Initialize OpenGL for this window.
  virtual void OpenGLInit();

protected:
  vtkOpenGLRenderWindow();
  ~vtkOpenGLRenderWindow();

  vtkIdList *TextureResourceIds;

  int GetPixelData(int x,int y,int x2,int y2,int front, unsigned char* data);
  int GetRGBAPixelData(int x,int y,int x2,int y2, int front, float* data);
  int GetRGBACharPixelData(int x,int y,int x2,int y2, int front,
                           unsigned char* data);

private:
  vtkOpenGLRenderWindow(const vtkOpenGLRenderWindow&);  // Not implemented.
  void operator=(const vtkOpenGLRenderWindow&);  // Not implemented.
};

#endif
