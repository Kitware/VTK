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

#ifndef vtkOpenGLRenderWindow_h
#define vtkOpenGLRenderWindow_h

#include "vtkRenderingOpenGLModule.h" // For export macro
#include "vtkRenderWindow.h"
#include "vtkOpenGL.h" // Needed for GLuint.

class vtkIdList;
class vtkOpenGLExtensionManager;
class vtkOpenGLHardwareSupport;
class vtkTextureUnitManager;
class vtkStdString;

class VTKRENDERINGOPENGL_EXPORT vtkOpenGLRenderWindow : public vtkRenderWindow
{
public:
  vtkTypeMacro(vtkOpenGLRenderWindow, vtkRenderWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set/Get the maximum number of multisamples
  static void SetGlobalMaximumNumberOfMultiSamples(int val);
  static int  GetGlobalMaximumNumberOfMultiSamples();

  // Description:
  // Update system if needed due to stereo rendering.
  virtual void StereoUpdate();

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB...
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2,int front);
  virtual int GetPixelData(int x,int y,int x2,int y2, int front,
                           vtkUnsignedCharArray *data);
  virtual int SetPixelData(int x,int y,int x2,int y2,unsigned char *data,
                           int front);
  virtual int SetPixelData(int x,int y,int x2,int y2,
                           vtkUnsignedCharArray *data, int front);

  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBARGBA...
  virtual float *GetRGBAPixelData(int x,int y,int x2,int y2,int front);
  virtual int GetRGBAPixelData(int x,int y,int x2,int y2, int front,
                               vtkFloatArray* data);
  virtual int SetRGBAPixelData(int x,int y,int x2,int y2, float *data,
                               int front, int blend=0);
  virtual int SetRGBAPixelData(int x,int y,int x2,int y2, vtkFloatArray *data,
                               int front, int blend=0);
  virtual void ReleaseRGBAPixelData(float *data);
  virtual unsigned char *GetRGBACharPixelData(int x,int y,int x2,int y2,
                                              int front);
  virtual int GetRGBACharPixelData(int x,int y,int x2,int y2, int front,
                                   vtkUnsignedCharArray *data);
  virtual int SetRGBACharPixelData(int x, int y, int x2, int y2,
                                   unsigned char *data, int front,
                                   int blend=0);
  virtual int SetRGBACharPixelData(int x,int y,int x2,int y2,
                                   vtkUnsignedCharArray *data, int front,
                                   int blend=0);

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

  // Initialize the state of OpenGL that VTK wants for this window
  virtual void OpenGLInitState();

  // Initialize VTK for rendering in a new OpenGL context
  virtual void OpenGLInitContext();

  // Description:
  // Return the OpenGL name of the back left buffer.
  // It is GL_BACK_LEFT if GL is bound to the window-system-provided
  // framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
  // application-created framebuffer object (GPU-based offscreen rendering)
  // It is used by vtkOpenGLCamera.
  unsigned int GetBackLeftBuffer();

  // Description:
  // Return the OpenGL name of the back right buffer.
  // It is GL_BACK_RIGHT if GL is bound to the window-system-provided
  // framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
  // application-created framebuffer object (GPU-based offscreen rendering)
  // It is used by vtkOpenGLCamera.
  unsigned int GetBackRightBuffer();

  // Description:
  // Return the OpenGL name of the front left buffer.
  // It is GL_FRONT_LEFT if GL is bound to the window-system-provided
  // framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
  // application-created framebuffer object (GPU-based offscreen rendering)
  // It is used by vtkOpenGLCamera.
  unsigned int GetFrontLeftBuffer();

  // Description:
  // Return the OpenGL name of the front right buffer.
  // It is GL_FRONT_RIGHT if GL is bound to the window-system-provided
  // framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
  // application-created framebuffer object (GPU-based offscreen rendering)
  // It is used by vtkOpenGLCamera.
  unsigned int GetFrontRightBuffer();

  // Description:
  // Return the OpenGL name of the back left buffer.
  // It is GL_BACK if GL is bound to the window-system-provided
  // framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
  // application-created framebuffer object (GPU-based offscreen rendering)
  // It is used by vtkOpenGLCamera.
  unsigned int GetBackBuffer();

  // Description:
  // Return the OpenGL name of the front left buffer.
  // It is GL_FRONT if GL is bound to the window-system-provided
  // framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
  // application-created framebuffer object (GPU-based offscreen rendering)
  // It is used by vtkOpenGLCamera.
  unsigned int GetFrontBuffer();

  // Description:
  // @deprecated Replaced by
  // vtkOpenGLCheckErrorMacro
  VTK_LEGACY(virtual void CheckGraphicError());
  VTK_LEGACY(virtual int HasGraphicError());
  VTK_LEGACY(virtual const char *GetLastGraphicErrorString());

  // Description:
  // Get the time when the OpenGL context was created.
  virtual unsigned long GetContextCreationTime();

  // Description:
  // Returns the extension manager. A new one will be created if one hasn't
  // already been set up.
  vtkOpenGLExtensionManager* GetExtensionManager();

  // Description:
  // Returns an Hardware Support object. A new one will be created if one
  // hasn't already been set up.
  vtkOpenGLHardwareSupport* GetHardwareSupport();

  //BTX
  // Description:
  // Returns its texture unit manager object. A new one will be created if one
  // hasn't already been set up.
  vtkTextureUnitManager *GetTextureUnitManager();
  //ETX

  // Description:
  // Block the thread until the actual rendering is finished().
  // Useful for measurement only.
  virtual void WaitForCompletion();

protected:
  vtkOpenGLRenderWindow();
  ~vtkOpenGLRenderWindow();

  long OldMonitorSetting;
  vtkIdList *TextureResourceIds;

  int GetPixelData(int x, int y, int x2, int y2, int front, unsigned char* data);
  int GetRGBAPixelData(int x, int y, int x2, int y2, int front, float* data);
  int GetRGBACharPixelData(int x, int y, int x2, int y2, int front,
                           unsigned char* data);

  // Description:
  // Create an offScreen window based on OpenGL framebuffer extension.
  // Return if the creation was successful or not.
  // \pre positive_width: width>0
  // \pre positive_height: height>0
  // \pre not_initialized: !OffScreenUseFrameBuffer
  // \post valid_result: (result==0 || result==1)
  //                     && (result implies OffScreenUseFrameBuffer)
  int CreateHardwareOffScreenWindow(int width, int height);

  // Description:
  // Destroy an offscreen window based on OpenGL framebuffer extension.
  // \pre initialized: OffScreenUseFrameBuffer
  // \post destroyed: !OffScreenUseFrameBuffer
  void DestroyHardwareOffScreenWindow();

  // Description:
  // Flag telling if a framebuffer-based offscreen is currently in use.
  int OffScreenUseFrameBuffer;

  // Description:
  // Variables used by the framebuffer-based offscreen method.
  int NumberOfFrameBuffers;
  unsigned int TextureObjects[4]; // really GLuint
  unsigned int FrameBufferObject; // really GLuint
  unsigned int DepthRenderBufferObject; // really GLuint

  // Description:
  // Create a not-off-screen window.
  virtual void CreateAWindow() = 0;

  // Description:
  // Destroy a not-off-screen window.
  virtual void DestroyWindow() = 0;

  // Description:
  // Set the texture unit manager.
  void SetTextureUnitManager(vtkTextureUnitManager *textureUnitManager);

  unsigned int BackLeftBuffer;
  unsigned int BackRightBuffer;
  unsigned int FrontLeftBuffer;
  unsigned int FrontRightBuffer;
  unsigned int FrontBuffer;
  unsigned int BackBuffer;

#ifndef VTK_LEGACY_REMOVE
  // Description:
  // @deprecated Replaced by
  // vtkOpenGLCheckErrorMacro
  unsigned int LastGraphicError;
#endif

  // Description:
  // Flag telling if the context has been created here or was inherited.
  int OwnContext;

  vtkTimeStamp ContextCreationTime;

  vtkTextureUnitManager *TextureUnitManager;

private:
  vtkOpenGLRenderWindow(const vtkOpenGLRenderWindow&);  // Not implemented.
  void operator=(const vtkOpenGLRenderWindow&);  // Not implemented.

  void SetExtensionManager(vtkOpenGLExtensionManager*);
  void SetHardwareSupport(vtkOpenGLHardwareSupport * renderWindow);

  vtkOpenGLExtensionManager* ExtensionManager;
  vtkOpenGLHardwareSupport* HardwareSupport;
};

#endif
