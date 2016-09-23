/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLRenderWindow.h"
#include <cassert>
#include "vtkFloatArray.h"
#include "vtkgl.h"
#include "vtkIdList.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLHardwareSupport.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLTexture.h"
#include "vtkUnsignedCharArray.h"
#include "vtkTextureUnitManager.h"
#include "vtkPainterDeviceAdapter.h"
#include "vtkStdString.h"
#include <sstream>
using std::ostringstream;

vtkCxxSetObjectMacro(vtkOpenGLRenderWindow, ExtensionManager, vtkOpenGLExtensionManager);
vtkCxxSetObjectMacro(vtkOpenGLRenderWindow, HardwareSupport, vtkOpenGLHardwareSupport);
vtkCxxSetObjectMacro(vtkOpenGLRenderWindow, TextureUnitManager, vtkTextureUnitManager);

// Initialize static member that controls global maximum number of multisamples
// (off by default on Apple because it causes problems on some Mac models).
#if defined(__APPLE__)
static int vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = 0;
#else
static int vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = 8;
#endif

const char* defaultWindowName = "Visualization Toolkit - OpenGL";

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(int val)
{
  if (val == vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples) return;
  vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = val;
}

// ----------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetGlobalMaximumNumberOfMultiSamples()
{
  return vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;
}

// ----------------------------------------------------------------------------
vtkOpenGLRenderWindow::vtkOpenGLRenderWindow()
{
  this->ExtensionManager = NULL;
  this->HardwareSupport = NULL;
  this->TextureUnitManager = 0;

  this->PainterDeviceAdapter = vtkPainterDeviceAdapter::New();

  this->MultiSamples = vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;
  this->TextureResourceIds = vtkIdList::New();
  delete [] this->WindowName;
  this->WindowName = new char[strlen(defaultWindowName) + 1];
  strcpy(this->WindowName, defaultWindowName);

  this->OffScreenUseFrameBuffer = 0;
  this->FrameBufferObject = 0;
  this->HardwareBufferSize[0] = 0;
  this->HardwareBufferSize[1] = 0;
  this->HardwareOffScreenBuffersBind = false;

  this->BackLeftBuffer = static_cast<unsigned int>(GL_BACK_LEFT);
  this->BackRightBuffer = static_cast<unsigned int>(GL_BACK_RIGHT);
  this->FrontLeftBuffer = static_cast<unsigned int>(GL_FRONT_LEFT);
  this->FrontRightBuffer = static_cast<unsigned int>(GL_FRONT_RIGHT);
  this->BackBuffer = static_cast<unsigned int>(GL_BACK);
  this->FrontBuffer = static_cast<unsigned int>(GL_FRONT);

  this->OwnContext = 1;
}

//----------------------------------------------------------------------------
const char *vtkOpenGLRenderWindow::GetRenderingBackend()
{
  return "OpenGL1";
}


// free up memory & close the window
// ----------------------------------------------------------------------------
vtkOpenGLRenderWindow::~vtkOpenGLRenderWindow()
{
  this->TextureResourceIds->Delete();
  if(this->TextureUnitManager!=0)
  {
    this->TextureUnitManager->SetContext(0);
  }

  if (this->ExtensionManager)
  {
    this->ExtensionManager->SetRenderWindow(0);
  }
  if (this->HardwareSupport)
  {
    this->HardwareSupport->SetExtensionManager(0);
    //this->HardwareSupport->Delete();
  }
  this->SetTextureUnitManager(0);
  this->SetExtensionManager(0);
  this->SetHardwareSupport(0);
}

// ----------------------------------------------------------------------------
vtkMTimeType vtkOpenGLRenderWindow::GetContextCreationTime()
{
  return this->ContextCreationTime.GetMTime();
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the back left buffer.
// It is GL_BACK_LEFT if GL is bound to the window-system-provided
// framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetBackLeftBuffer()
{
  return this->BackLeftBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the back right buffer.
// It is GL_BACK_RIGHT if GL is bound to the window-system-provided
// framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetBackRightBuffer()
{
  return this->BackRightBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the front left buffer.
// It is GL_FRONT_LEFT if GL is bound to the window-system-provided
// framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetFrontLeftBuffer()
{
  return this->FrontLeftBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the front right buffer.
// It is GL_FRONT_RIGHT if GL is bound to the window-system-provided
// framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetFrontRightBuffer()
{
  return this->FrontRightBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the back left buffer.
// It is GL_BACK if GL is bound to the window-system-provided
// framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetBackBuffer()
{
  return this->BackBuffer;
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the front left buffer.
// It is GL_FRONT if GL is bound to the window-system-provided
// framebuffer. It is vtkgl::COLOR_ATTACHMENT0_EXT if GL is bound to an
// application-created framebuffer object (GPU-based offscreen rendering)
// It is used by vtkOpenGLCamera.
unsigned int vtkOpenGLRenderWindow::GetFrontBuffer()
{
  return this->FrontBuffer;
}

// Update system if needed due to stereo rendering.
void vtkOpenGLRenderWindow::StereoUpdate(void)
{
  // if stereo is on and it wasn't before
  if (this->StereoRender && (!this->StereoStatus))
  {
    switch (this->StereoType)
    {
      case VTK_STEREO_CRYSTAL_EYES:
        // not clear this is supposed to be empty,
        // but it has been that way forever.
        break;
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_CHECKERBOARD:
        this->StereoStatus = 1;
        break;
      case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
        this->StereoStatus = 1;
        break;
    }
  }
  else if ((!this->StereoRender) && this->StereoStatus)
  {
    switch (this->StereoType)
    {
      case VTK_STEREO_CRYSTAL_EYES:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_RED_BLUE:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_ANAGLYPH:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_DRESDEN:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_INTERLACED:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_CHECKERBOARD:
        this->StereoStatus = 0;
        break;
      case VTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
        this->StereoStatus = 0;
        break;
    }
  }
}

void vtkOpenGLRenderWindow::SetSize(int a[2])
{
  this->SetSize(a[0], a[1]);
}

void vtkOpenGLRenderWindow::SetSize(int x, int y)
{
  if (this->Size[0] == x
    && this->Size[1] == y)
  {
    // Nothing should happend in the superclass but never knows...
    this->Superclass::SetSize(x, y);
    return;
  }

  this->Superclass::SetSize(x, y);
  if (this->HardwareOffScreenBuffersBind)
  {
    // We activate the offscreen buffers again so they will be
    // recreated at the new window size
    this->SetUseOffScreenBuffers(true);
  }
}

void vtkOpenGLRenderWindow::OpenGLInit()
{
  OpenGLInitContext();
  OpenGLInitState();
}

void vtkOpenGLRenderWindow::OpenGLInitState()
{
  glMatrixMode( GL_MODELVIEW );
  glDepthFunc( GL_LEQUAL );
  glEnable( GL_DEPTH_TEST );
  glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

  // initialize blending for transparency
  if(vtkgl::BlendFuncSeparate!=0)
  {
    vtkgl::BlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                             GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
  }
  else
  {
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  }
  glEnable(GL_BLEND);

  if (this->PointSmoothing)
  {
    glEnable(GL_POINT_SMOOTH);
  }
  else
  {
    glDisable(GL_POINT_SMOOTH);
  }

  if (this->LineSmoothing)
  {
    glEnable(GL_LINE_SMOOTH);
  }
  else
  {
    glDisable(GL_LINE_SMOOTH);
  }

  if (this->PolygonSmoothing)
  {
    glEnable(GL_POLYGON_SMOOTH);
  }
  else
  {
    glDisable(GL_POLYGON_SMOOTH);
  }

  glEnable(GL_NORMALIZE);
  glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
  glAlphaFunc(GL_GREATER,0);

  // Default OpenGL is 4 bytes but it is only safe with RGBA format.
  // If format is RGB, row alignment is 4 bytes only if the width is divisible
  // by 4. Let's do it the safe way: 1-byte alignment.
  // If an algorithm really need 4 bytes alignment, it should set it itself,
  // this is the recommended way in "Avoiding 16 Common OpenGL Pitfalls",
  // section 7:
  // http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
  glPixelStorei(GL_PACK_ALIGNMENT,1);
  // Set the number of alpha bit planes used by the window
  int rgba[4];
  this->GetColorBufferSizes(rgba);
  this->SetAlphaBitPlanes(rgba[3]);
}

void vtkOpenGLRenderWindow::OpenGLInitContext()
{
  // When a new OpenGL context is created, force an update
  // of the extension manager by calling modified on it.
  vtkOpenGLExtensionManager *extensions = this->GetExtensionManager();
  extensions->Modified();

  this->ContextCreationTime.Modified();

  // We have to set the function pointer to null, otherwise the following
  // scenario would fail on Windows (and maybe other kind of configurations):
  // 1. Render onscreen on GPU that supports OpenGL 1.4
  // 2. Switch to offscreen with GDI Windows implementation (1.1)
  vtkgl::BlendFuncSeparate=0;

  // Try to initialize vtkgl::BlendFuncSeparate() if available.
  if (extensions->ExtensionSupported("GL_VERSION_1_4"))
  {
    extensions->LoadExtension("GL_VERSION_1_4");
  }
  else
  {
    if (extensions->ExtensionSupported("GL_EXT_blend_func_separate"))
    {
      extensions->LoadCorePromotedExtension("GL_EXT_blend_func_separate");
    }
  }
}

void vtkOpenGLRenderWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

int vtkOpenGLRenderWindow::GetDepthBufferSize()
{
  GLint size;

  if ( this->Mapped )
  {
    this->MakeCurrent();
    size = 0;
    glGetIntegerv( GL_DEPTH_BITS, &size );
    return static_cast<int>(size);
  }
  else
  {
    vtkDebugMacro(<< "Window is not mapped yet!" );
    return 24;
  }
}

int vtkOpenGLRenderWindow::GetColorBufferSizes(int *rgba)
{
  GLint size;

  if (rgba==NULL)
  {
    return 0;
  }
  rgba[0] = 0;
  rgba[1] = 0;
  rgba[2] = 0;
  rgba[3] = 0;

  if ( this->Mapped)
  {
    this->MakeCurrent();
    glGetIntegerv( GL_RED_BITS, &size );
    rgba[0] = static_cast<int>(size);
    glGetIntegerv( GL_GREEN_BITS, &size  );
    rgba[1] = static_cast<int>(size);
    glGetIntegerv( GL_BLUE_BITS, &size );
    rgba[2] = static_cast<int>(size);
    glGetIntegerv( GL_ALPHA_BITS, &size );
    rgba[3] = static_cast<int>(size);
    return rgba[0]+rgba[1]+rgba[2]+rgba[3];
  }
  else
  {
    vtkDebugMacro(<< "Window is not mapped yet!" );
    rgba[0] = 8;
    rgba[1] = 8;
    rgba[2] = 8;
    rgba[3] = 8;
    return 32;
  }
}

unsigned char* vtkOpenGLRenderWindow::GetPixelData(int x1, int y1,
                                                   int x2, int y2,
                                                   int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  unsigned char *data =
    new unsigned char[(x_hi - x_low + 1)*(y_hi - y_low + 1)*3];
  this->GetPixelData(x1, y1, x2, y2, front, data);
  return data;
}

int vtkOpenGLRenderWindow::GetPixelData(int x1, int y1,
                                        int x2, int y2,
                                        int front,
                                        vtkUnsignedCharArray* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  int width  = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 3*width*height;

  if ( data->GetMaxId()+1 != size)
  {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(3);
    data->SetNumberOfValues(size);
  }
  return this->GetPixelData(x1, y1, x2, y2, front, data->GetPointer(0));

}

int vtkOpenGLRenderWindow::GetPixelData(int x1, int y1,
                                        int x2, int y2,
                                        int front, unsigned char* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  // set the current window
  this->MakeCurrent();

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  if (front)
  {
    glReadBuffer(static_cast<GLenum>(this->GetFrontLeftBuffer()));
  }
  else
  {
    glReadBuffer(static_cast<GLenum>(this->GetBackLeftBuffer()));
  }

  glDisable( GL_SCISSOR_TEST );

#if defined(sparc) && !defined(GL_VERSION_1_2)
  // We need to read the image data one row at a time and convert it
  // from RGBA to RGB to get around a bug in Sun OpenGL 1.1
  long    xloop, yloop;
  unsigned char *buffer;
  unsigned char *p_data = NULL;

  buffer = new unsigned char [4*(x_hi - x_low + 1)];
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
  {
    // read in a row of pixels
    glReadPixels(x_low,yloop,(x_hi-x_low+1),1,
                 GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    for (xloop = 0; xloop <= x_hi-x_low; xloop++)
    {
      *p_data = buffer[xloop*4]; p_data++;
      *p_data = buffer[xloop*4+1]; p_data++;
      *p_data = buffer[xloop*4+2]; p_data++;
    }
  }

  delete [] buffer;
#else
  // If the Sun bug is ever fixed, then we could use the following
  // technique which provides a vast speed improvement on the SGI

  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );

  // Calling pack alignment ensures that we can grab the any size window
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );
  glReadPixels(x_low, y_low, x_hi-x_low+1, y_hi-y_low+1, GL_RGB,
               GL_UNSIGNED_BYTE, data);
#endif

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }

}

int vtkOpenGLRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
                                        vtkUnsignedCharArray *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
  {

    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  int width  = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 3*width*height;

  if ( data->GetMaxId()+1 != size)
  {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
  }
  return this->SetPixelData(x1, y1, x2, y2, data->GetPointer(0), front);

}

int vtkOpenGLRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
                                        unsigned char *data, int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  // set the current window
  this->MakeCurrent();

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  GLint buffer;
  glGetIntegerv(GL_DRAW_BUFFER, &buffer);

  if (front)
  {
    glDrawBuffer(this->GetFrontBuffer());
  }
  else
  {
    glDrawBuffer(this->GetBackBuffer());
  }

  if (y1 < y2)
  {

    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  glDisable( GL_SCISSOR_TEST );
  glViewport(0, 0, this->Size[0], this->Size[1]);

#if defined(sparc) && !defined(GL_VERSION_1_2)
  // We need to read the image data one row at a time and convert it
  // from RGBA to RGB to get around a bug in Sun OpenGL 1.1
  long    xloop, yloop;
  unsigned char *buffer;
  unsigned char *p_data = NULL;

  buffer = new unsigned char [4*(x_hi - x_low + 1)];

  // now write the binary info one row at a time
  glDisable(GL_BLEND);
  p_data = data;
  for (yloop = y_low; yloop <= y_hi; yloop++)
  {
    for (xloop = 0; xloop <= x_hi - x_low; xloop++)
    {
      buffer[xloop*4] = *p_data; p_data++;
      buffer[xloop*4+1] = *p_data; p_data++;
      buffer[xloop*4+2] = *p_data; p_data++;
      buffer[xloop*4+3] = 0xff;
    }
    /* write out a row of pixels */
    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadIdentity();
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadIdentity();
    glRasterPos3f( (2.0 * static_cast<GLfloat>(x_low) / this->Size[0] - 1),
                   (2.0 * static_cast<GLfloat>(yloop) / this->Size[1] - 1),
                   -1.0 );
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW );
    glPopMatrix();

    glDrawPixels((x_hi-x_low+1),1, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  }

  // This seems to be necessary for the image to show up
  glFlush();

  glEnable(GL_BLEND);
#else
  // If the Sun bug is ever fixed, then we could use the following
  // technique which provides a vast speed improvement on the SGI

  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );

  // now write the binary info
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos3f( (2.0 * static_cast<GLfloat>(x_low) / this->Size[0] - 1),
                 (2.0 * static_cast<GLfloat>(y_low) / this->Size[1] - 1),
                 -1.0 );
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
  glDisable(GL_BLEND);
  glDrawPixels((x_hi-x_low+1), (y_hi - y_low + 1),
               GL_RGB, GL_UNSIGNED_BYTE, data);
  glEnable(GL_BLEND);

  // This seems to be necessary for the image to show up
  glFlush();
#endif

  glDrawBuffer(buffer);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

float* vtkOpenGLRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2,
                                               int front)
{

  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  float *data = new float[ (width*height*4) ];
  this->GetRGBAPixelData(x1, y1, x2, y2, front, data);

  return data;

}

int vtkOpenGLRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            int front, vtkFloatArray* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  int size = 4*width*height;

  if ( data->GetMaxId()+1 != size)
  {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(4);
    data->SetNumberOfValues(size);
  }
  return this->GetRGBAPixelData(x1, y1, x2, y2, front, data->GetPointer(0));

}

int vtkOpenGLRenderWindow::GetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            int front, float* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  // set the current window
  this->MakeCurrent();

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  if (front)
  {
    glReadBuffer(static_cast<GLenum>(this->GetFrontLeftBuffer()));
  }
  else
  {
    glReadBuffer(static_cast<GLenum>(this->GetBackLeftBuffer()));
  }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;


  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );

  glPixelStorei( GL_PACK_ALIGNMENT, 1 );
  glReadPixels( x_low, y_low, width, height, GL_RGBA, GL_FLOAT, data);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

void vtkOpenGLRenderWindow::ReleaseRGBAPixelData(float *data)
{
  delete[] data;
}

int vtkOpenGLRenderWindow::SetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            vtkFloatArray *data, int front,
                                            int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;

  int size = 4*width*height;
  if ( data->GetMaxId()+1 != size )
  {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
  }

  return this->SetRGBAPixelData(x1, y1, x2, y2, data->GetPointer(0), front,
                                blend);
}

int vtkOpenGLRenderWindow::SetRGBAPixelData(int x1, int y1, int x2, int y2,
                                            float *data, int front, int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  // set the current window
  this->MakeCurrent();

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  GLint buffer;
  glGetIntegerv(GL_DRAW_BUFFER, &buffer);

  if (front)
  {
    glDrawBuffer(this->GetFrontBuffer());
  }
  else
  {
    glDrawBuffer(this->GetBackBuffer());
  }

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;

  /* write out a row of pixels */
  glDisable( GL_ALPHA_TEST );
  glDisable( GL_SCISSOR_TEST );
  glViewport(0, 0, this->Size[0], this->Size[1]);
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos3f( (2.0 * static_cast<GLfloat>(x_low) / this->Size[0] - 1),
                 (2.0 * static_cast<GLfloat>(y_low) / this->Size[1] - 1),
                 -1.0 );
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  if (!blend)
  {
    glDisable(GL_BLEND);
    glDrawPixels( width, height, GL_RGBA, GL_FLOAT, data);
    glEnable(GL_BLEND);
  }
  else
  {
    glDrawPixels( width, height, GL_RGBA, GL_FLOAT, data);
  }

  // This seems to be necessary for the image to show up
  glFlush();

  glDrawBuffer(buffer);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

unsigned char *vtkOpenGLRenderWindow::GetRGBACharPixelData(int x1, int y1,
                                                           int x2, int y2,
                                                           int front)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }


  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  unsigned char *data = new unsigned char[ (width*height)*4 ];
  this->GetRGBACharPixelData(x1, y1, x2, y2, front, data);

  return data;
}

int vtkOpenGLRenderWindow::GetRGBACharPixelData(int x1, int y1,
                                                int x2, int y2,
                                                int front,
                                                vtkUnsignedCharArray* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  int width  = abs(x_hi - x_low) + 1;
  int height = abs(y_hi - y_low) + 1;
  int size = 4*width*height;

  if ( data->GetMaxId()+1 != size)
  {
    vtkDebugMacro("Resizing array.");
    data->SetNumberOfComponents(4);
    data->SetNumberOfValues(size);
  }
  return this->GetRGBACharPixelData(x1, y1, x2, y2, front,
                                    data->GetPointer(0));
}

int vtkOpenGLRenderWindow::GetRGBACharPixelData(int x1, int y1,
                                                int x2, int y2,
                                                int front,
                                                unsigned char* data)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;


  // set the current window
  this->MakeCurrent();


  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }


  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }


  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  if (front)
  {
    glReadBuffer(static_cast<GLenum>(this->GetFrontLeftBuffer()));
  }
  else
  {
    glReadBuffer(static_cast<GLenum>(this->GetBackLeftBuffer()));
  }

  width  = abs(x_hi - x_low) + 1;
  height = abs(y_hi - y_low) + 1;

  glDisable( GL_SCISSOR_TEST );

  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );

  glReadPixels( x_low, y_low, width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                data);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }

}


int vtkOpenGLRenderWindow::SetRGBACharPixelData(int x1,int y1,int x2,int y2,
                                                vtkUnsignedCharArray *data,
                                                int front, int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;

  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }

  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }

  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;

  int size = 4*width*height;
  if ( data->GetMaxId()+1 != size )
  {
    vtkErrorMacro("Buffer is of wrong size. It is " << data->GetMaxId()+1
                  << ", it should be: " << size);
    return VTK_ERROR;
  }

  return this->SetRGBACharPixelData(x1, y1, x2, y2, data->GetPointer(0),
                                    front, blend);

}

int vtkOpenGLRenderWindow::SetRGBACharPixelData(int x1, int y1, int x2,
                                                int y2, unsigned char *data,
                                                int front, int blend)
{
  int     y_low, y_hi;
  int     x_low, x_hi;
  int     width, height;


  // set the current window
  this->MakeCurrent();


  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  GLint buffer;
  glGetIntegerv(GL_DRAW_BUFFER, &buffer);

  if (front)
  {
    glDrawBuffer(this->GetFrontBuffer());
  }
  else
  {
    glDrawBuffer(this->GetBackBuffer());
  }


  if (y1 < y2)
  {
    y_low = y1;
    y_hi  = y2;
  }
  else
  {
    y_low = y2;
    y_hi  = y1;
  }


  if (x1 < x2)
  {
    x_low = x1;
    x_hi  = x2;
  }
  else
  {
    x_low = x2;
    x_hi  = x1;
  }


  width  = abs(x_hi-x_low) + 1;
  height = abs(y_hi-y_low) + 1;


  /* write out a row of pixels */
  glViewport(0, 0, this->Size[0], this->Size[1]);
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos3f( (2.0 * static_cast<GLfloat>(x_low) / this->Size[0] - 1),
                 (2.0 * static_cast<GLfloat>(y_low) / this->Size[1] - 1),
                 -1.0 );
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glDisable( GL_ALPHA_TEST );
  glDisable( GL_SCISSOR_TEST );

  // Disable writing on the z-buffer.
  glDepthMask(GL_FALSE);
  glDisable(GL_DEPTH_TEST);

  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );

  if (!blend)
  {
    glDisable(GL_BLEND);
    glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                  data);
    glEnable(GL_BLEND);
  }
  else
  {
    glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                  data);
  }

  // Renenable writing on the z-buffer.
  glDepthMask(GL_TRUE);
  glEnable(GL_DEPTH_TEST);

  // This seems to be necessary for the image to show up
  glFlush();

  glDrawBuffer(buffer);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}


int vtkOpenGLRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2,
                                           float* z_data )
{
  int             y_low;
  int             x_low;
  int             width, height;

  // set the current window
  this->MakeCurrent();

  if (y1 < y2)
  {
    y_low = y1;
  }
  else
  {
    y_low = y2;
  }

  if (x1 < x2)
  {
    x_low = x1;
  }
  else
  {
    x_low = x2;
  }

  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );
  glDisable( GL_SCISSOR_TEST );
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );

  glReadPixels( x_low, y_low,
                width, height,
                GL_DEPTH_COMPONENT, GL_FLOAT,
                z_data );

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}

float *vtkOpenGLRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2  )
{
  float           *z_data;

  int             width, height;
  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  z_data = new float[width*height];
  this->GetZbufferData(x1, y1, x2, y2, z_data);

  return z_data;
}

int vtkOpenGLRenderWindow::GetZbufferData( int x1, int y1, int x2, int y2,
                                           vtkFloatArray *buffer )
{
  int  width, height;
  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;
  int size = width*height;
  if ( buffer->GetMaxId()+1 != size)
  {
    vtkDebugMacro("Resizing array.");
    buffer->SetNumberOfComponents(1);
    buffer->SetNumberOfValues(size);
  }
  return this->GetZbufferData(x1, y1, x2, y2, buffer->GetPointer(0));
}

int vtkOpenGLRenderWindow::SetZbufferData( int x1, int y1, int x2, int y2,
                                           vtkFloatArray *buffer )
{
  int width, height;
  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;
  int size = width*height;
  if ( buffer->GetMaxId()+1 != size )
  {
    vtkErrorMacro("Buffer is of wrong size.");
    return VTK_ERROR;
  }
  return this->SetZbufferData(x1, y1, x2, y2, buffer->GetPointer(0));
}

int vtkOpenGLRenderWindow::SetZbufferData( int x1, int y1, int x2, int y2,
                                           float *buffer )
{
  int             y_low;
  int             x_low;
  int             width, height;

  // set the current window
  this->MakeCurrent();

  if (y1 < y2)
  {
    y_low = y1;
  }
  else
  {
    y_low = y2;
  }

  if (x1 < x2)
  {
    x_low = x1;
  }
  else
  {
    x_low = x2;
  }

  width =  abs(x2 - x1)+1;
  height = abs(y2 - y1)+1;

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
  {
    ;
  }

  glViewport(0, 0, this->Size[0], this->Size[1]);
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_PROJECTION );
  glPushMatrix();
  glLoadIdentity();
  glRasterPos2f( 2.0 * static_cast<GLfloat>(x_low) / this->Size[0] - 1,
                 2.0 * static_cast<GLfloat>(y_low) / this->Size[1] - 1);
  glMatrixMode( GL_PROJECTION );
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW );
  glPopMatrix();

  glDisable( GL_ALPHA_TEST );
  glDisable( GL_SCISSOR_TEST );

  int gldepth;
  glGetIntegerv(GL_DEPTH_FUNC, &gldepth);
  glDepthFunc(GL_ALWAYS);

  // Turn of texturing in case it is on - some drivers have a problem
  // getting / setting pixels with texturing enabled.
  glDisable( GL_TEXTURE_2D );
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );

  glDrawPixels( width, height, GL_DEPTH_COMPONENT, GL_FLOAT, buffer);

  // This seems to be necessary for the image to show up
  glFlush();

  glDepthFunc(gldepth);

  if (glGetError() != GL_NO_ERROR)
  {
    return VTK_ERROR;
  }
  else
  {
    return VTK_OK;
  }
}


void vtkOpenGLRenderWindow::RegisterTextureResource (GLuint id)
{
  this->TextureResourceIds->InsertNextId (static_cast<int>(id));
}

// ----------------------------------------------------------------------------
// Description:
// Create an offScreen window based on OpenGL framebuffer extension.
// Return if the creation was successful or not.
// \pre positive_width: width>0
// \pre positive_height: height>0
// \pre not_initialized: !OffScreenUseFrameBuffer
// \post valid_result: (result==0 || result==1)
//                     && (result implies OffScreenUseFrameBuffer)
int vtkOpenGLRenderWindow::CreateHardwareOffScreenWindow(int width, int height)
{
  assert("pre: positive_width" && width>0);
  assert("pre: positive_height" && height>0);
  assert("pre: not_initialized" && !this->OffScreenUseFrameBuffer);

  // Create a regular OpenGLcontext (ie create a window)
  this->CreateAWindow();

  this->MakeCurrent();
  this->OpenGLInit();

  int result = this->CreateHardwareOffScreenBuffers(width, height);
  if (!result)
  {
    this->DestroyWindow();
  }
  else
  {
    this->BindHardwareOffScreenBuffers();
    this->OffScreenUseFrameBuffer = 1;
  }

  // A=>B = !A || B
  assert("post: valid_result" && (result==0 || result==1)
         && (!result || OffScreenUseFrameBuffer));
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Create an offScreen rendering buffer based on OpenGL framebuffer extension.
// Return if the creation was successful or not.
int vtkOpenGLRenderWindow::CreateHardwareOffScreenBuffers(int width, int height,
                                                          bool bind)
{
  assert("pre: positive_width" && width>0);
  assert("pre: positive_height" && height>0);

  // This implementation currently ignores multisampling configurations:
  if (this->MultiSamples > 1)
  {
    vtkDebugMacro(<<"Multisampling is not currently supported by the "
                  "accelerated offscreen rendering backend. Falling back to "
                  "a platform-specific offscreen solution...");
    return 0;
  }

  this->MakeCurrent();

  if (this->FrameBufferObject &&
    this->HardwareBufferSize[0] == width &&
    this->HardwareBufferSize[1] == height)
  {
    if (bind)
    {
      this->BindHardwareOffScreenBuffers();
    }
    return 1;
  }
  if (this->FrameBufferObject)
  {
    this->DestroyHardwareOffScreenBuffers();
  }

  this->FrameBufferObject = 0;
  this->HardwareBufferSize[0] = 0;
  this->HardwareBufferSize[1] = 0;

  // Check for OpenGL extensions GL_EXT/ARB_framebuffer_object and
  // GL_ARB_texture_non_power_of_two (core-promoted feature in OpenGL 2.0)
  vtkOpenGLExtensionManager *extensions = this->GetExtensionManager();

  float glVersion;
  sscanf(reinterpret_cast<const char*>(glGetString(GL_VERSION)), "%f", &glVersion);

  int supports_GL_EXT_framebuffer_object =
    glVersion >= 3.0 ||
    extensions->ExtensionSupported("GL_EXT_framebuffer_object") ||
    extensions->ExtensionSupported("GL_ARB_framebuffer_object");

  // TODO Mesa 6.5.1 is from 2006 verify that this is still an issue
  // with  newer releases
  // We skip it if you use Mesa. Even if the VTK offscreen test passes (OSCone)
  // with Mesa, all the Paraview batch test are failing (Mesa 6.5.1 or CVS)
  // After too much time spent to investigate this case, we just skip it.
  int isMesa = false;
    /*= extensions->DriverGLRendererHas("Mesa")
    && !extensions->GetIgnoreDriverBugs("Mesa 6.5.1 pvbatch offscreen bug");*/

  int supports_texture_non_power_of_two =
    extensions->ExtensionSupported("GL_VERSION_2_0") ||
    extensions->ExtensionSupported("GL_ARB_texture_non_power_of_two");
  int supports_texture_rectangle =
    extensions->ExtensionSupported("GL_ARB_texture_rectangle");

  // The following extension does not exist on ATI. There will be no HW
  // Offscreen on ATI if a stencil buffer is required.
  int supports_packed_depth_stencil =
    extensions->ExtensionSupported("GL_EXT_packed_depth_stencil");

  if(!(supports_GL_EXT_framebuffer_object &&
       (supports_texture_non_power_of_two || supports_texture_rectangle) &&
       !isMesa && (!this->StencilCapable || supports_packed_depth_stencil)))
  {
    if (!supports_GL_EXT_framebuffer_object)
    {
      vtkDebugMacro( << " extension GL_EXT_framebuffer_object is not supported. "
        "Hardware accelerated offscreen rendering is not available" );
    }
    if (!supports_texture_non_power_of_two)
    {
      vtkDebugMacro( << " extension texture_non_power_of_two is not supported "
        "because neither OpenGL 2.0 nor GL_ARB_texture_non_power_of_two extension "
        "is supported. Hardware accelerated offscreen rendering is not available");
    }
    if (!supports_texture_rectangle)
    {
      vtkDebugMacro(<<" extension GL_ARB_texture_rectangle is not supported");
    }
    if (isMesa)
    {
      vtkDebugMacro(<<" Renderer is Mesa. Hardware accelerated offscreen "
        "rendering is not available");
    }
    if (this->StencilCapable && !supports_packed_depth_stencil)
    {
      vtkDebugMacro(<<" a stencil buffer is required but extension "
        "GL_EXT_packed_depth_stencil is not supported");
    }
    return 0;
  }

  int result = 0;

  if (!extensions->LoadSupportedExtension("GL_EXT_framebuffer_object"))
  {
    extensions->LoadSupportedExtension("GL_ARB_framebuffer_object");
  }

  this->NumberOfFrameBuffers = 1;
  GLboolean isStereo;
  glGetBooleanv(GL_STEREO, &isStereo);
  if (isStereo)
  {
    this->NumberOfFrameBuffers = 2;
  }

  // Up to 2 for stereo
  GLuint textureObjects[2] = { 0, 0 };
  GLuint frameBufferObject;
  GLuint depthRenderBufferObject;
  vtkgl::GenFramebuffersEXT(1, &frameBufferObject); // color
  vtkgl::GenRenderbuffersEXT(1, &depthRenderBufferObject); // depth
  glGenTextures(this->NumberOfFrameBuffers, textureObjects);
  // Bind the color buffer
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, frameBufferObject);

  GLenum target = supports_texture_non_power_of_two ?
    GL_TEXTURE_2D : vtkgl::TEXTURE_RECTANGLE_ARB;

  for (int i = 0 ; i < this->NumberOfFrameBuffers; i++)
  {
    glBindTexture(target, textureObjects[i]);
    glTexParameteri(target, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_WRAP_T, vtkgl::CLAMP_TO_EDGE);
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(target, 0, GL_RGBA8, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                   vtkgl::COLOR_ATTACHMENT0_EXT+i,
                                   target, textureObjects[i], 0);
  }

  GLenum status = vtkgl::CheckFramebufferStatusEXT(vtkgl::FRAMEBUFFER_EXT);

  if (status == vtkgl::FRAMEBUFFER_UNSUPPORTED_EXT && target == GL_TEXTURE_2D &&
     supports_texture_rectangle)
  {
    // The following cards fall in this case:
    // GeForce FX Go5650/AGP/SSE2 with Linux driver 2.0.2 NVIDIA 87.76
    // GeForce FX 5900 Ultra/AGP/SSE2 with Linux driver 2.0.2 NVIDIA 87.74
    // GeForce FX 5200/AGP/SSE2 with Windows XP SP2 32bit driver 2.0.3
    // Quadro FX 1000/AGP/SSE2 with Windows XP SP2 32bit driver 2.0.1
    // Quadro FX 2000/AGP/SSE2 with Windows XP SP2 32bit driver 2.0.1
    target = vtkgl::TEXTURE_RECTANGLE_ARB;
    // try again.
    glDeleteTextures(this->NumberOfFrameBuffers, textureObjects);
    glGenTextures(this->NumberOfFrameBuffers, textureObjects);
    for (int i = 0; i < this->NumberOfFrameBuffers; i++)
    {
      glBindTexture(target, textureObjects[i]);
      glTexParameteri(target, GL_TEXTURE_WRAP_S, vtkgl::CLAMP_TO_EDGE);
      glTexParameteri(target, GL_TEXTURE_WRAP_T, vtkgl::CLAMP_TO_EDGE);
      glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexImage2D(target, 0, GL_RGBA8, width, height,
                   0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                     vtkgl::COLOR_ATTACHMENT0_EXT+i,
                                     target, textureObjects[i], 0);
    }
    // Ask for the status again.
    status = vtkgl::CheckFramebufferStatusEXT(vtkgl::FRAMEBUFFER_EXT);
  }

  if (status != vtkgl::FRAMEBUFFER_COMPLETE_EXT)
  {
    vtkDebugMacro(<< "Hardware does not support GPU Offscreen rendering.");
    vtkgl::DeleteFramebuffersEXT(1, &frameBufferObject);
    vtkgl::DeleteRenderbuffersEXT(1, &depthRenderBufferObject);
    glDeleteTextures(this->NumberOfFrameBuffers, textureObjects);
    bind = true;
  }
  else
  {
    // Set up the depth (and stencil), render buffer
    vtkgl::BindRenderbufferEXT(vtkgl::RENDERBUFFER_EXT, depthRenderBufferObject);
    vtkgl::RenderbufferStorageEXT(vtkgl::RENDERBUFFER_EXT,
      this->StencilCapable ? vtkgl::DEPTH_STENCIL_EXT : vtkgl::DEPTH_COMPONENT24,
      width, height);
    vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT,
                                      vtkgl::DEPTH_ATTACHMENT_EXT,
                                      vtkgl::RENDERBUFFER_EXT,
                                      depthRenderBufferObject);
    if (this->StencilCapable)
    {
      vtkgl::FramebufferRenderbufferEXT(vtkgl::FRAMEBUFFER_EXT,
                                        vtkgl::STENCIL_ATTACHMENT_EXT,
                                        vtkgl::RENDERBUFFER_EXT,
                                        depthRenderBufferObject);
    }

    // Last check to see if the FBO is supported or not.
    status = vtkgl::CheckFramebufferStatusEXT(vtkgl::FRAMEBUFFER_EXT);
    if (status != vtkgl::FRAMEBUFFER_COMPLETE_EXT)
    {
      vtkDebugMacro(<< "Hardware does not support GPU Offscreen rendering with"
                       "this depth/stencil configuration.");
      glBindTexture(target, 0);
      vtkgl::DeleteFramebuffersEXT(1, &frameBufferObject);
      vtkgl::DeleteRenderbuffersEXT(1, &depthRenderBufferObject);
      glDeleteTextures(this->NumberOfFrameBuffers, textureObjects);
      bind = true;
    }
    else
    {
      result = 1;
      // Save GL objects by static casting to standard C types. GL* types
      // are not allowed in VTK header files.
      this->FrameBufferObject =
        static_cast<unsigned int>(frameBufferObject);
      this->DepthRenderBufferObject =
        static_cast<unsigned int>(depthRenderBufferObject);
      for (int i = 0; i < this->NumberOfFrameBuffers; i++)
      {
        this->TextureObjects[i] = static_cast<unsigned int>(textureObjects[i]);
      }
      this->HardwareBufferSize[0] = width;
      this->HardwareBufferSize[1] = height;
      this->HardwareOffScreenBuffersBind = true;

      if (bind)
      {
        unsigned int buffer =
          static_cast<unsigned int>(vtkgl::COLOR_ATTACHMENT0_EXT);
        this->BackLeftBuffer  = buffer;
        this->FrontLeftBuffer = buffer;
        this->BackBuffer      = buffer;
        this->FrontBuffer     = buffer;

        if (this->NumberOfFrameBuffers == 2)
        {
          buffer = static_cast<unsigned int>(vtkgl::COLOR_ATTACHMENT1_EXT);
          this->BackRightBuffer = buffer;
          this->FrontRightBuffer = buffer;
        }
      }
    }
  }

  if (!bind)
  {
    // Rebind the hardware on-screen buffer for now
    this->UnbindHardwareOffScreenBuffers();
  }

  // A=>B = !A || B
  assert("post: valid_result" && (result==0 || result==1)
         && (!result || this->FrameBufferObject));
  return result;
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::BindHardwareOffScreenBuffers()
{
  if (!this->FrameBufferObject || this->HardwareOffScreenBuffersBind)
  {
    return;
  }

  this->MakeCurrent();
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,
    static_cast<GLuint>(this->FrameBufferObject));

  unsigned int buffer =
    static_cast<unsigned int>(vtkgl::COLOR_ATTACHMENT0_EXT);
  this->BackLeftBuffer  = buffer;
  this->FrontLeftBuffer = buffer;
  this->BackBuffer      = buffer;
  this->FrontBuffer     = buffer;

  if (this->NumberOfFrameBuffers == 2)
  {
    buffer = static_cast<unsigned int>(vtkgl::COLOR_ATTACHMENT1_EXT);
    this->BackRightBuffer = buffer;
    this->FrontRightBuffer = buffer;
  }
  this->HardwareOffScreenBuffersBind = true;
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::UnbindHardwareOffScreenBuffers()
{
  if (!this->FrameBufferObject || !this->HardwareOffScreenBuffersBind)
  {
    return;
  }

  this->MakeCurrent();
  vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT, 0);

  // Restore framebuffer names.
  this->BackLeftBuffer = static_cast<unsigned int>(GL_BACK_LEFT);
  this->BackRightBuffer = static_cast<unsigned int>(GL_BACK_RIGHT);
  this->FrontLeftBuffer = static_cast<unsigned int>(GL_FRONT_LEFT);
  this->FrontRightBuffer = static_cast<unsigned int>(GL_FRONT_RIGHT);
  this->BackBuffer = static_cast<unsigned int>(GL_BACK);
  this->FrontBuffer = static_cast<unsigned int>(GL_FRONT);

  this->HardwareOffScreenBuffersBind = false;
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::DestroyHardwareOffScreenBuffers()
{
  if (!this->FrameBufferObject)
  {
    return;
  }
  this->UnbindHardwareOffScreenBuffers();

  GLuint frameBufferObject = static_cast<GLuint>(this->FrameBufferObject);
  vtkgl::DeleteFramebuffersEXT(1, &frameBufferObject);
  this->FrameBufferObject = 0;

  GLuint depthRenderBufferObject =
    static_cast<GLuint>(this->DepthRenderBufferObject);
  vtkgl::DeleteRenderbuffersEXT(1, &depthRenderBufferObject);
  this->DepthRenderBufferObject = 0;

  GLuint textureObjects[4];
  for (int i = 0; i < this->NumberOfFrameBuffers; i++)
  {
    textureObjects[i] = static_cast<GLuint>(this->TextureObjects[i]);
  }

  glDeleteTextures(this->NumberOfFrameBuffers, textureObjects);

  this->DepthRenderBufferObject = 0;
  this->FrameBufferObject = 0;
  this->HardwareBufferSize[0] = 0;
  this->HardwareBufferSize[1] = 0;
}

// ----------------------------------------------------------------------------
// Description:
// Destroy an offscreen window based on OpenGL framebuffer extension.
// \pre initialized: OffScreenUseFrameBuffer
// \post destroyed: !OffScreenUseFrameBuffer
void vtkOpenGLRenderWindow::DestroyHardwareOffScreenWindow()
{
  assert("pre: initialized" && this->OffScreenUseFrameBuffer);

  this->UnbindHardwareOffScreenBuffers();
  this->DestroyHardwareOffScreenBuffers();

  this->OffScreenUseFrameBuffer = 0;

  this->DestroyWindow();
}

// ----------------------------------------------------------------------------
// Description:
// Returns the extension manager. A new one will be created if one hasn't
// already been set up.
vtkOpenGLExtensionManager* vtkOpenGLRenderWindow::GetExtensionManager()
{
  if (!this->ExtensionManager)
  {
    vtkOpenGLExtensionManager* mgr = vtkOpenGLExtensionManager::New();
    // This does not form a reference loop since vtkOpenGLExtensionManager does
    // not keep a reference to the render window.
    mgr->SetRenderWindow(this);
    this->SetExtensionManager(mgr);
    mgr->Delete();
  }
  return this->ExtensionManager;
}

// ----------------------------------------------------------------------------
// Description:
// Returns an Hardware Support object. A new one will be created if one hasn't
// already been set up.
vtkOpenGLHardwareSupport* vtkOpenGLRenderWindow::GetHardwareSupport()
{
  if (!this->HardwareSupport)
  {
    vtkOpenGLHardwareSupport* hardware = vtkOpenGLHardwareSupport::New();

    // This does not form a reference loop since vtkOpenGLHardwareSupport does
    // not keep a reference to the render window.
    hardware->SetExtensionManager(this->GetExtensionManager());
    this->SetHardwareSupport(hardware);
    hardware->Delete();
  }
  return this->HardwareSupport;
}

// ----------------------------------------------------------------------------
// Description:
// Returns its texture unit manager object. A new one will be created if one
// hasn't already been set up.
vtkTextureUnitManager *vtkOpenGLRenderWindow::GetTextureUnitManager()
{
  if(this->TextureUnitManager==0)
  {
    vtkTextureUnitManager *manager=vtkTextureUnitManager::New();

    // This does not form a reference loop since vtkOpenGLHardwareSupport does
    // not keep a reference to the render window.
    manager->SetContext(this);
    this->SetTextureUnitManager(manager);
    manager->Delete();
  }
  return this->TextureUnitManager;
}

// ----------------------------------------------------------------------------
// Description:
// Block the thread until the actual rendering is finished().
// Useful for measurement only.
void vtkOpenGLRenderWindow::WaitForCompletion()
{
  glFinish();
}

// ----------------------------------------------------------------------------
int vtkOpenGLRenderWindow::SetUseOffScreenBuffers(bool offScreen)
{
  if (this->OffScreenRendering ||
    (!offScreen && !this->HardwareOffScreenBuffersBind))
  {
    return 1;
  }


  if (!offScreen)
  {
    if (!this->OffScreenUseFrameBuffer)
    {
      this->UnbindHardwareOffScreenBuffers();
    }
    return 1;
  }

  if (this->OffScreenUseFrameBuffer)
  {
    return 1;
  }
  // We are currently in on screen rendering mode.
  // Create offscreen buffers at the screen size.
  int* size = this->GetSize();
  return this->CreateHardwareOffScreenBuffers(size[0], size[1], true);
}

// ----------------------------------------------------------------------------
bool vtkOpenGLRenderWindow::GetUseOffScreenBuffers()
{
  return this->HardwareOffScreenBuffersBind || this->OffScreenRendering;
}
