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
#include "vtk_glew.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkglVBOHelper.h"

#include <cassert>
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLActor.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLRenderer.h"
#include "vtkOpenGLError.h"
#include "vtkUnsignedCharArray.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkStdString.h"
#include "vtkTextureObject.h"
#include "vtkTextureUnitManager.h"
#include "vtkRendererCollection.h"

#include <sstream>
using std::ostringstream;

vtkCxxSetObjectMacro(vtkOpenGLRenderWindow, TextureUnitManager, vtkTextureUnitManager);

// Initialize static member that controls global maximum number of multisamples
// (off by default on Apple because it causes problems on some Mac models).
#if defined(__APPLE__)
static int vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = 0;
#else
static int vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = 8;
#endif

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::SetGlobalMaximumNumberOfMultiSamples(int val)
{
  if (val == vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples)
    {
    return;
    }
  vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples = val;
}

// ----------------------------------------------------------------------------
int vtkOpenGLRenderWindow::GetGlobalMaximumNumberOfMultiSamples()
{
  return vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;
}

// used for OpenGL32 Support
static bool vtkOpenGLRenderWindowContextSupportsOpenGL32 = false;

bool vtkOpenGLRenderWindow::GetContextSupportsOpenGL32()
{
  return vtkOpenGLRenderWindowContextSupportsOpenGL32;
}

// ----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::SetContextSupportsOpenGL32(bool val)
{
  if (val == vtkOpenGLRenderWindowContextSupportsOpenGL32)
    {
    return;
    }
  vtkOpenGLRenderWindowContextSupportsOpenGL32 = val;
}


// ----------------------------------------------------------------------------
vtkOpenGLRenderWindow::vtkOpenGLRenderWindow()
{
  this->Initialized = false;

  this->ShaderCache = vtkOpenGLShaderCache::New();

  this->TextureUnitManager=0;

  this->MultiSamples = vtkOpenGLRenderWindowGlobalMaximumNumberOfMultiSamples;
  delete [] this->WindowName;
  this->WindowName = new char[strlen("Visualization Toolkit - OpenGL")+1];
  strcpy( this->WindowName, "Visualization Toolkit - OpenGL" );

  this->OffScreenUseFrameBuffer=0;

  this->BackLeftBuffer=static_cast<unsigned int>(GL_BACK_LEFT);
  this->BackRightBuffer=static_cast<unsigned int>(GL_BACK_RIGHT);
  this->FrontLeftBuffer=static_cast<unsigned int>(GL_FRONT_LEFT);
  this->FrontRightBuffer=static_cast<unsigned int>(GL_FRONT_RIGHT);
  this->BackBuffer=static_cast<unsigned int>(GL_BACK);
  this->FrontBuffer=static_cast<unsigned int>(GL_FRONT);

  #ifndef VTK_LEGACY_REMOVE
  this->LastGraphicError=static_cast<unsigned int>(GL_NO_ERROR);
  #endif

  this->DrawPixelsTextureObject = NULL;

  this->OwnContext=1;
}

// free up memory & close the window
// ----------------------------------------------------------------------------
vtkOpenGLRenderWindow::~vtkOpenGLRenderWindow()
{
  if(this->DrawPixelsTextureObject != 0)
    {
    this->DrawPixelsTextureObject->UnRegister(this);
    this->DrawPixelsTextureObject = NULL;
    }
  this->TextureResourceIds.clear();
  if(this->TextureUnitManager!=0)
    {
    this->TextureUnitManager->SetContext(0);
    }

  this->SetTextureUnitManager(0);
  this->ShaderCache->UnRegister(this);
}

void vtkOpenGLRenderWindow::ReleaseGraphicsResources()
{
  vtkCollectionSimpleIterator rsit;
  this->Renderers->InitTraversal(rsit);
  vtkRenderer *aren;
  while ( (aren = this->Renderers->GetNextRenderer(rsit)) )
    {
    if (aren->GetRenderWindow() == this)
      {
      aren->ReleaseGraphicsResources(this);
      }
    }

  if(this->DrawPixelsTextureObject != 0)
     {
     this->DrawPixelsTextureObject->ReleaseGraphicsResources(this);
     }

  this->ShaderCache->ReleaseGraphicsResources(this);

  if (this->TextureResourceIds.size())
    {
    vtkErrorMacro("There are still active textures when there should not be.");
    typedef std::map<const vtkTextureObject *, int>::const_iterator TRIter;
    TRIter found = this->TextureResourceIds.begin();
    for ( ; found != this->TextureResourceIds.end(); found++)
      {
      vtkErrorMacro("Leaked for texture object: " << const_cast<vtkTextureObject *>(found->first));
      }
    }
}


// ----------------------------------------------------------------------------
unsigned long vtkOpenGLRenderWindow::GetContextCreationTime()
{
  return this->ContextCreationTime.GetMTime();
}

// ----------------------------------------------------------------------------
// Description:
// Return the OpenGL name of the back left buffer.
// It is GL_BACK_LEFT if GL is bound to the window-system-provided
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT if GL is bound to an
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
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
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
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT if GL is bound to an
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
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT+1 if GL is bound to an
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
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT if GL is bound to an
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
// framebuffer. It is GL_COLOR_ATTACHMENT0_EXT if GL is bound to an
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

#ifndef VTK_LEGACY_REMOVE
//----------------------------------------------------------------------------
void vtkOpenGLRenderWindow::CheckGraphicError()
{
  VTK_LEGACY_BODY(vtkRenderWindow::CheckGraphicError, "VTK 6.1");
  this->LastGraphicError=static_cast<unsigned int>(glGetError());
}

//----------------------------------------------------------------------------
int vtkOpenGLRenderWindow::HasGraphicError()
{
  VTK_LEGACY_BODY(vtkRenderWindow::HasGraphics, "VTK 6.1");
  return static_cast<GLenum>(this->LastGraphicError)!=GL_NO_ERROR;
}

//----------------------------------------------------------------------------
const char *vtkOpenGLRenderWindow::GetLastGraphicErrorString()
{
  VTK_LEGACY_BODY(vtkRenderWindow::GetLastGraphicErrorString, "VTK 6.1");
  const char *result="Unknown error";
  return result;
}
#endif


void vtkOpenGLRenderWindow::OpenGLInit()
{
  OpenGLInitContext();
  OpenGLInitState();
}

void vtkOpenGLRenderWindow::OpenGLInitState()
{
  glDepthFunc( GL_LEQUAL );
  glEnable( GL_DEPTH_TEST );

  // initialize blending for transparency
  if(glBlendFuncSeparate != 0)
    {
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                             GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
    }
  else
    {
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
    }
  glEnable(GL_BLEND);

  if (this->PointSmoothing)
    {
#ifdef GL_POINT_SMOOTH
    glEnable(GL_POINT_SMOOTH);
#endif
    }
  else
    {
#ifdef GL_POINT_SMOOTH
    glDisable(GL_POINT_SMOOTH);
#endif
    }

  if (this->LineSmoothing)
    {
#ifdef GL_LINE_SMOOTH
    glEnable(GL_LINE_SMOOTH);
#endif
    }
  else
    {
#ifdef GL_LINE_SMOOTH
    glDisable(GL_LINE_SMOOTH);
#endif
    }

  if (this->PolygonSmoothing)
    {
#ifdef GL_POLYGON_SMOOTH
    glEnable(GL_POLYGON_SMOOTH);
#endif
    }
  else
    {
#ifdef GL_POLYGON_SMOOTH
    glDisable(GL_POLYGON_SMOOTH);
#endif
    }

  // Default OpenGL is 4 bytes but it is only safe with RGBA format.
  // If format is RGB, row alignment is 4 bytes only if the width is divisible
  // by 4. Let's do it the safe way: 1-byte alignment.
  // If an algorithm really need 4 bytes alignment, it should set it itself,
  // this is the recommended way in "Avoiding 16 Common OpenGL Pitfalls",
  // section 7:
  // http://www.opengl.org/resources/features/KilgardTechniques/oglpitfall/
#ifdef GL_UNPACK_ALIGNMENT
  glPixelStorei(GL_UNPACK_ALIGNMENT,1);
#endif
  glPixelStorei(GL_PACK_ALIGNMENT,1);
  // Set the number of alpha bit planes used by the window
  int rgba[4];
  this->GetColorBufferSizes(rgba);
  this->SetAlphaBitPlanes(rgba[3]);
}

void vtkOpenGLRenderWindow::OpenGLInitContext()
{
  this->ContextCreationTime.Modified();

  // When a new OpenGL context is created, force an update
  if (!this->Initialized)
    {
#ifdef GLEW_OK
    GLenum result = glewInit();
    bool m_valid = (result == GLEW_OK);
    if (!m_valid)
      {
      vtkErrorMacro("GLEW could not be initialized.");
      return;
      }

    if (!GLEW_VERSION_2_1)
      {
      vtkErrorMacro("GL version 2.1 is not supported by your graphics driver.");
      //m_valid = false;
      return;
      }
#endif
    this->Initialized = true;
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

  // Calling pack alignment ensures that we can grab the any size window
  glPixelStorei( GL_PACK_ALIGNMENT, 1 );
  glReadPixels(x_low, y_low, x_hi-x_low+1, y_hi-y_low+1, GL_RGB,
               GL_UNSIGNED_BYTE, data);

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


// ---------------------------------------------------------------------------
// a program must be bound
// a VAO must be bound
void vtkOpenGLRenderWindow::RenderQuad(
  float *verts,
  float *tcoords,
  vtkShaderProgram *program, vtkgl::VertexArrayObject *vao)
{
  if (!program || !vao || !verts)
    {
    vtkGenericWarningMacro(<< "Error must have verts, program and vao");
    }

  vtkgl::BufferObject vbo;
  vbo.Upload(verts, 12, vtkgl::BufferObject::ArrayBuffer);
  vao->Bind();
  if (!vao->AddAttributeArray(program, vbo, "vertexMC", 0, sizeof(float)*3, VTK_FLOAT, 3, false))
    {
    vtkGenericWarningMacro(<< "Error setting 'vertexMC' in shader VAO.");
    }

  vtkgl::BufferObject tvbo;
  if (tcoords)
    {
    tvbo.Upload(tcoords, 8, vtkgl::BufferObject::ArrayBuffer);
    if (!vao->AddAttributeArray(program, tvbo, "tcoordMC", 0, sizeof(float)*2, VTK_FLOAT, 2, false))
      {
      vtkGenericWarningMacro(<< "Error setting 'tcoordMC' in shader VAO.");
      }
    }

  GLuint iboData[] = {0, 1, 2, 0, 2, 3};
  vtkgl::BufferObject ibo;
  vao->Bind();
  ibo.Upload(iboData, 6, vtkgl::BufferObject::ElementArrayBuffer);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT,
    reinterpret_cast<const GLvoid *>(NULL));
  ibo.Release();
  vao->Release();
  vao->RemoveAttributeArray("vertexMC");
  vao->RemoveAttributeArray("tcoordMC");
  vbo.Release();
  if (tcoords)
    {
    tvbo.Release();
    }
}


// draw (and stretch as needed) the data to the current viewport
void vtkOpenGLRenderWindow::DrawPixels(
  int srcWidth, int srcHeight, int numComponents, int dataType, void *data)
{
  glDisable( GL_SCISSOR_TEST );
  if (!this->DrawPixelsTextureObject)
    {
    this->DrawPixelsTextureObject = vtkTextureObject::New();
    }
  this->DrawPixelsTextureObject->SetContext(this);
  this->DrawPixelsTextureObject->Create2DFromRaw(srcWidth, srcHeight,
        numComponents, dataType, data);
  this->DrawPixelsTextureObject->CopyToFrameBuffer(NULL, NULL);

  // This seems to be necessary for the image to show up
  glFlush();
}

// very generic call to draw pixel data to a region of the window
void vtkOpenGLRenderWindow::DrawPixels(
  int dstXmin, int dstYmin, int dstXmax, int dstYmax,
  int srcXmin, int srcYmin, int srcXmax, int srcYmax,
  int srcWidth, int srcHeight, int numComponents, int dataType, void *data)
{
  glDisable( GL_SCISSOR_TEST );
  if (!this->DrawPixelsTextureObject)
    {
    this->DrawPixelsTextureObject = vtkTextureObject::New();
    }
  this->DrawPixelsTextureObject->SetContext(this);
  this->DrawPixelsTextureObject->Create2DFromRaw(srcWidth, srcHeight,
        numComponents, dataType, data);
  this->DrawPixelsTextureObject->CopyToFrameBuffer(
      srcXmin, srcYmin, srcXmax, srcYmax-1,
      dstXmin, dstYmin, dstXmax, dstYmax,
      this->GetSize()[0], this->GetSize()[1],
      NULL, NULL);

  // This seems to be necessary for the image to show up
  glFlush();
}

// less generic verison, old API
void vtkOpenGLRenderWindow::DrawPixels(int x1, int y1, int x2, int y2, int numComponents, int dataType, void *data)
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

  int width = x_hi-x_low+1;
  int height = y_hi-y_low+1;

  // call the more generic version
  this->DrawPixels(x_low, y_low, x_hi, y_hi,
    0, 0, width-1, height-1, width, height, numComponents, dataType, data);
}

int vtkOpenGLRenderWindow::SetPixelData(int x1, int y1, int x2, int y2,
                                        unsigned char *data, int front)
{
  // set the current window
  this->MakeCurrent();

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
    {
    ;
    }

  GLint buffer;

#ifdef GL_DRAW_BUFFER
  glGetIntegerv(GL_DRAW_BUFFER, &buffer);
#endif

  if (front)
    {
    glDrawBuffer(this->GetFrontBuffer());
    }
  else
    {
    glDrawBuffer(this->GetBackBuffer());
    }

  this->DrawPixels(x1, y1, x2, y2, 3, VTK_UNSIGNED_CHAR, data);

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
  // set the current window
  this->MakeCurrent();

  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
    {
    ;
    }

  GLint buffer;
#ifdef GL_DRAW_BUFFER
  glGetIntegerv(GL_DRAW_BUFFER, &buffer);
#endif

  if (front)
    {
    glDrawBuffer(this->GetFrontBuffer());
    }
  else
    {
    glDrawBuffer(this->GetBackBuffer());
    }

  if (!blend)
    {
    glDisable(GL_BLEND);
    this->DrawPixels(x1, y1, x2, y2, 4, VTK_FLOAT, data); // TODO replace dprecated function
    glEnable(GL_BLEND);
    }
  else
    {
    this->DrawPixels(x1, y1, x2, y2, 4, VTK_FLOAT, data);
    }

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
  // set the current window
  this->MakeCurrent();


  // Error checking
  // Must clear previous errors first.
  while(glGetError() != GL_NO_ERROR)
    {
    ;
    }

  GLint buffer;
#ifdef GL_DRAW_BUFFER
  glGetIntegerv(GL_DRAW_BUFFER, &buffer);
#endif

  if (front)
    {
    glDrawBuffer(this->GetFrontBuffer());
    }
  else
    {
    glDrawBuffer(this->GetBackBuffer());
    }


  // Disable writing on the z-buffer.
  glDepthMask(GL_FALSE);
  glDisable(GL_DEPTH_TEST);

  if (!blend)
    {
    glDisable(GL_BLEND);
    this->DrawPixels(x1,y1,x2,y2,4, VTK_UNSIGNED_CHAR, data);
    glEnable(GL_BLEND);
    }
  else
    {
    this->DrawPixels(x1,y1,x2,y2,4, VTK_UNSIGNED_CHAR, data);
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

int vtkOpenGLRenderWindow::SetZbufferData( int vtkNotUsed(x1), int vtkNotUsed(y1),
                                           int vtkNotUsed(x2), int vtkNotUsed(y2),
                                           float *vtkNotUsed(buffer) )
{
  // Todo: not sure this can be done in the new opengl, either implement or remove
  return VTK_ERROR;
}


void vtkOpenGLRenderWindow::ActivateTexture(vtkTextureObject *texture)
{
  // Only add if it isn't already there
  typedef std::map<const vtkTextureObject *, int>::const_iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found == this->TextureResourceIds.end())
    {
    int activeUnit =  this->GetTextureUnitManager()->Allocate();
    if (activeUnit < 0)
      {
      vtkErrorMacro("Hardware does not support the number of textures defined.");
      return;
      }
    this->TextureResourceIds.insert(std::make_pair(texture, activeUnit));
    glActiveTexture(GL_TEXTURE0 + activeUnit);
    }
  else
    {
    glActiveTexture(GL_TEXTURE0 + found->second);
    }
}

void vtkOpenGLRenderWindow::DeactivateTexture(vtkTextureObject *texture)
{
  // Only deactivate if it isn't already there
  typedef std::map<const vtkTextureObject *, int>::iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found != this->TextureResourceIds.end())
    {
    this->GetTextureUnitManager()->Free(found->second);
    this->TextureResourceIds.erase(found);
    }
}

int vtkOpenGLRenderWindow::GetTextureUnitForTexture(vtkTextureObject *texture)
{
  // Only deactivate if it isn't already there
  typedef std::map<const vtkTextureObject *, int>::const_iterator TRIter;
  TRIter found = this->TextureResourceIds.find(texture);
  if (found != this->TextureResourceIds.end())
    {
    return found->second;
    }

  return -1;
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

  // 1. create a regular OpenGLcontext (ie create a window)
  this->CreateAWindow();
  this->MakeCurrent();
  int result = 0;

  // test for FBO support
#if GL_ES_VERSION_2_0 != 1
  if(!glewIsSupported("GL_EXT_framebuffer_object"))
    {
    vtkDebugMacro( << " extension GL_EXT_framebuffer_object is not supported. "
      "Hardware accelerated offscreen rendering is not available" );
    this->DestroyWindow();
    }
  else
#endif
    {
    // 2. regular framebuffer code
    this->NumberOfFrameBuffers=1;
#ifdef GL_STEREO
    GLboolean flag;
    glGetBooleanv(GL_STEREO,&flag);
    if(flag)
      {
      this->NumberOfFrameBuffers<<=1;
      }
#endif

    // Up to 2: stereo
    GLuint textureObjects[2];

    GLuint frameBufferObject;
    GLuint depthRenderBufferObject;
    glGenFramebuffers(1, &frameBufferObject); // color
    glGenRenderbuffers(1, &depthRenderBufferObject); // depth
    int i=0;
    while(i<this->NumberOfFrameBuffers)
      {
      textureObjects[i]=0;
      ++i;
      }
    glGenTextures(this->NumberOfFrameBuffers,textureObjects);
    // Color buffers
    glBindFramebuffer(GL_FRAMEBUFFER,frameBufferObject);

    GLenum target;
    target=GL_TEXTURE_2D;

    i=0;
    while(i<this->NumberOfFrameBuffers)
      {
      glBindTexture(target,textureObjects[i]);
      glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#ifdef GL_RGBA8
      glTexImage2D(target,0,GL_RGBA8,width,height,
                   0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
#else
      glTexImage2D(target,0,GL_RGBA,width,height,
                   0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
#endif
      glFramebufferTexture2D(GL_FRAMEBUFFER,
                             GL_COLOR_ATTACHMENT0+i,
                             target, textureObjects[i], 0);
      ++i;
      }
    GLenum status;
    status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if(status != GL_FRAMEBUFFER_COMPLETE)
      {
      vtkDebugMacro(<<"Hardware does not support GPU Offscreen rendering.");
      glBindTexture(target,0);
      glBindFramebuffer(GL_FRAMEBUFFER,0);
      glDeleteFramebuffers(1,&frameBufferObject);
      glDeleteRenderbuffers(1,&depthRenderBufferObject);
      glDeleteTextures(this->NumberOfFrameBuffers,textureObjects);
      this->DestroyWindow();
      }
    else
      {
      // Set up the depth (and stencil), render buffer
      glBindRenderbuffer(GL_RENDERBUFFER,
                         depthRenderBufferObject);
#ifdef GL_DEPTH_STENCIL
      if(this->StencilCapable)
        {
        glRenderbufferStorage(GL_RENDERBUFFER,
                              GL_DEPTH_STENCIL, width,height);
        }
      else
#endif
        {
#ifdef GL_DEPTH_COMPONENT24
        glRenderbufferStorage(GL_RENDERBUFFER,
                              GL_DEPTH_COMPONENT24,width,height);
#else
        glRenderbufferStorage(GL_RENDERBUFFER,
                              GL_DEPTH_COMPONENT16,width,height);
#endif
        }
      glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                GL_DEPTH_ATTACHMENT,
                                GL_RENDERBUFFER,
                                depthRenderBufferObject);
      if(this->StencilCapable)
        {
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                  GL_STENCIL_ATTACHMENT,
                                  GL_RENDERBUFFER,
                                  depthRenderBufferObject);
        }

      // Last check to see if the FBO is supported or not.
      status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
      if(status != GL_FRAMEBUFFER_COMPLETE)
        {
        vtkDebugMacro(<<"Hardware does not support GPU Offscreen rendering withthis depth/stencil configuration.");
        glBindTexture(target,0);
        glBindFramebuffer(GL_FRAMEBUFFER,0);
        glDeleteFramebuffers(1,&frameBufferObject);
        glDeleteRenderbuffers(1,&depthRenderBufferObject);
        glDeleteTextures(this->NumberOfFrameBuffers,textureObjects);
        this->DestroyWindow();
        }
      else
        {
        result=1;
        this->BackLeftBuffer=
          static_cast<unsigned int>(GL_COLOR_ATTACHMENT0);
        this->FrontLeftBuffer=
          static_cast<unsigned int>(GL_COLOR_ATTACHMENT0);

        this->BackBuffer=static_cast<unsigned int>(
          GL_COLOR_ATTACHMENT0);
        this->FrontBuffer=
          static_cast<unsigned int>(GL_COLOR_ATTACHMENT0);

        if(this->NumberOfFrameBuffers==2)
          {
          this->BackRightBuffer=
            static_cast<unsigned int>(GL_COLOR_ATTACHMENT0+1);
          this->FrontRightBuffer=
            static_cast<unsigned int>(GL_COLOR_ATTACHMENT0+1);
          }

        // Save GL objects by static casting to standard C types. GL* types
        // are not allowed in VTK header files.
        this->FrameBufferObject=static_cast<unsigned int>(frameBufferObject);
        this->DepthRenderBufferObject=
          static_cast<unsigned int>(depthRenderBufferObject);
        i=0;
        while(i<this->NumberOfFrameBuffers)
          {
          this->TextureObjects[i]=static_cast<unsigned int>(textureObjects[i]);
          ++i;
          }
        this->OffScreenUseFrameBuffer=1;
        }
      }
    }
  // A=>B = !A || B
  assert("post: valid_result" && (result==0 || result==1)
         && (!result || OffScreenUseFrameBuffer));
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Destroy an offscreen window based on OpenGL framebuffer extension.
// \pre initialized: OffScreenUseFrameBuffer
// \post destroyed: !OffScreenUseFrameBuffer
void vtkOpenGLRenderWindow::DestroyHardwareOffScreenWindow()
{
  assert("pre: initialized" && this->OffScreenUseFrameBuffer);

  this->MakeCurrent();
  glBindFramebuffer(GL_FRAMEBUFFER, 0 );

  // Restore framebuffer names.
  this->BackLeftBuffer=static_cast<unsigned int>(GL_BACK_LEFT);
  this->BackRightBuffer=static_cast<unsigned int>(GL_BACK_RIGHT);
  this->FrontLeftBuffer=static_cast<unsigned int>(GL_FRONT_LEFT);
  this->FrontRightBuffer=static_cast<unsigned int>(GL_FRONT_RIGHT);
  this->BackBuffer=static_cast<unsigned int>(GL_BACK);
  this->FrontBuffer=static_cast<unsigned int>(GL_FRONT);

  GLuint frameBufferObject=static_cast<GLuint>(this->FrameBufferObject);
  glDeleteFramebuffers(1,&frameBufferObject);

  GLuint depthRenderBufferObject=static_cast<GLuint>(this->DepthRenderBufferObject);
  glDeleteRenderbuffers(1,&depthRenderBufferObject);

  GLuint textureObjects[4];
  int i=0;
  while(i<this->NumberOfFrameBuffers)
    {
    textureObjects[i]=static_cast<GLuint>(this->TextureObjects[i]);
    ++i;
    }

  glDeleteTextures(this->NumberOfFrameBuffers,textureObjects);
  this->DestroyWindow();

  this->OffScreenUseFrameBuffer=0;

  assert("post: destroyed" && !this->OffScreenUseFrameBuffer);
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
