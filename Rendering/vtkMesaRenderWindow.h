/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMesaRenderWindow.h
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
  vtkTypeMacro(vtkMesaRenderWindow,vtkRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkMesaRenderWindow *New();
  
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
