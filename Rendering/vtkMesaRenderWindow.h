/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRenderWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMesaRenderWindow - Mesa rendering window
// .SECTION Description
// vtkMesaRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. It uses the mangle openGL namespace mgl, so Mesa render 
// windows can be created in the same program as openGL render windows.

#ifndef __vtkMesaRenderWindow_h
#define __vtkMesaRenderWindow_h

#include "vtkRenderWindow.h"

#include "MangleMesaInclude/gl_mangle.h" // Needed for GLuint
#include <MangleMesaInclude/gl.h> // Needed for GLuint

class vtkIdList;

class VTK_RENDERING_EXPORT vtkMesaRenderWindow : public vtkRenderWindow
{
protected:
  int MultiSamples;
  long OldMonitorSetting;

public:
  vtkTypeMacro(vtkMesaRenderWindow,vtkRenderWindow);
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
  virtual int GetZbufferData( int x1, int y1, int x2, int y2, float *buffer );
  virtual int GetZbufferData( int x1, int y1, int x2, int y2, 
                              vtkFloatArray* z );
  virtual int SetZbufferData( int x1, int y1, int x2, int y2, float *buffer );
  virtual int SetZbufferData( int x1, int y1, int x2, int y2, 
                              vtkFloatArray *buffer );

  // Description:
  // Make this window the current Mesa context.
  void MakeCurrent() = 0;
  
  // Description:
  // Register a texture name with this render window.
  void RegisterTextureResource (GLuint id);

  // Description:
  // Get the size of the depth buffer.
  int GetDepthBufferSize();
  
  // Description:
  // Get the size of the color buffer.
  // Returns 0 if not able to determine otherwise sets R G B and A into buffer.
  int GetColorBufferSizes(int *rgba);

  // Description:
  // Initialize OpenGL for this window.
  virtual void OpenGLInit();
 
protected:
  vtkMesaRenderWindow();
  ~vtkMesaRenderWindow();

  vtkIdList *TextureResourceIds;

  int GetPixelData(int x,int y,int x2,int y2,int front, unsigned char* data);
  int GetRGBAPixelData(int x,int y,int x2,int y2, int front, float* data);
  int GetRGBACharPixelData(int x,int y,int x2,int y2, int front,
                           unsigned char* data);

private:
  vtkMesaRenderWindow(const vtkMesaRenderWindow&);  // Not implemented.
  void operator=(const vtkMesaRenderWindow&);  // Not implemented.
};

#endif
