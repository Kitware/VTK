/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRenderWindow.h
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
// .NAME vtkMesaRenderWindow - Mesa rendering window
// .SECTION Description
// vtkMesaRenderWindow is a concrete implementation of the abstract class
// vtkRenderWindow. It uses the mangle openGL namespace mgl, so Mesa render 
// windows can be created in the same program as openGL render windows.

#ifndef __vtkMesaRenderWindow_h
#define __vtkMesaRenderWindow_h

#include "MangleMesaInclude/gl_mangle.h"
#include <MangleMesaInclude/gl.h>



#include <stdlib.h>
#include "vtkRenderWindow.h"

class vtkIdList;

class VTK_RENDERING_EXPORT vtkMesaRenderWindow : public vtkRenderWindow
{
protected:
  int MultiSamples;
  long OldMonitorSetting;

public:
  vtkTypeRevisionMacro(vtkMesaRenderWindow,vtkRenderWindow);
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
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *,
                            int front);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBARGBA... 
  virtual float *GetRGBAPixelData(int x,int y,int x2,int y2,int front);
  virtual void SetRGBAPixelData(int x,int y,int x2,int y2,float *,int front,
                                int blend=0);
  virtual void ReleaseRGBAPixelData(float *data);
  virtual unsigned char *GetRGBACharPixelData(int x,int y,int x2,int y2,
                                              int front);
  virtual void SetRGBACharPixelData(int x,int y,int x2,int y2,unsigned char *,
                                    int front, int blend=0);  

  // Description:
  // Set/Get the zbuffer data from an image
  virtual float *GetZbufferData( int x1, int y1, int x2, int y2 );
  virtual void SetZbufferData( int x1, int y1, int x2, int y2, float *buffer );

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
  // Initialize OpenGL for this window.
  virtual void OpenGLInit();
 
protected:
  vtkMesaRenderWindow();
  ~vtkMesaRenderWindow();

  vtkIdList *TextureResourceIds;
private:
  vtkMesaRenderWindow(const vtkMesaRenderWindow&);  // Not implemented.
  void operator=(const vtkMesaRenderWindow&);  // Not implemented.
};

#endif
