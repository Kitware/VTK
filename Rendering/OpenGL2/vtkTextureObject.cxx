/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextureObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextureObject.h"

#include "vtk_glew.h"

#include "vtkObjectFactory.h"


#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
#include "vtkPixelBufferObject.h"
#endif

#include "vtkNew.h"
#include "vtkOpenGLError.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLTexture.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"

#include "vtkglVBOHelper.h"

#include <cassert>

//#define VTK_TO_DEBUG
//#define VTK_TO_TIMING

#ifdef VTK_TO_TIMING
#include "vtkTimerLog.h"
#endif

#include "vtkTextureObjectFS.h"
#include "vtkTextureObjectVS.h"  // a pass through shader

#define BUFFER_OFFSET(i) (static_cast<char *>(NULL) + (i))

// Mapping from DepthTextureCompareFunction values to OpenGL values.
//----------------------------------------------------------------------------
static GLint OpenGLDepthTextureCompareFunction[8]=
{
  GL_LEQUAL,
  GL_GEQUAL,
  GL_LESS,
  GL_GREATER,
  GL_EQUAL,
  GL_NOTEQUAL,
  GL_ALWAYS,
  GL_NEVER
};

//----------------------------------------------------------------------------
static const char *DepthTextureCompareFunctionAsString[8]=
{
  "Lequal",
  "Gequal",
  "Less",
  "Greater",
  "Equal",
  "NotEqual",
  "AlwaysTrue",
  "Never"
};

// Mapping from Wrap values to OpenGL values
#if GL_ES_VERSION_2_0 != 1
  //--------------------------------------------------------------------------
  static GLint OpenGLWrap[4]=
  {
    GL_CLAMP_TO_EDGE,
    GL_REPEAT,
    GL_MIRRORED_REPEAT,
    GL_CLAMP_TO_BORDER
  };

  //--------------------------------------------------------------------------
  static const char *WrapAsString[4]=
  {
    "ClampToEdge",
    "Repeat",
    "MirroredRepeat",
    "ClampToBorder"
  };

  //----------------------------------------------------------------------------
  static GLenum OpenGLAlphaInternalFormat[5]=
  {
    GL_ALPHA,
    GL_ALPHA4,
    GL_ALPHA8,
    GL_ALPHA12,
    GL_ALPHA16
  };

#else
  //--------------------------------------------------------------------------
  static GLint OpenGLWrap[3]=
  {
    GL_CLAMP_TO_EDGE,
    GL_REPEAT,
    GL_MIRRORED_REPEAT
  };

  //--------------------------------------------------------------------------
  static const char *WrapAsString[3]=
  {
    "ClampToEdge",
    "Repeat",
    "MirroredRepeat"
  };

  //----------------------------------------------------------------------------
  static GLenum OpenGLAlphaInternalFormat[5]=
  {
    GL_ALPHA,
    GL_ALPHA,
    GL_ALPHA,
    GL_ALPHA,
    GL_ALPHA
  };

#endif

// Mapping MinificationFilter values to OpenGL values.
//----------------------------------------------------------------------------
static GLint OpenGLMinFilter[6]=
{
  GL_NEAREST,
  GL_LINEAR,
  GL_NEAREST_MIPMAP_NEAREST,
  GL_NEAREST_MIPMAP_LINEAR,
  GL_LINEAR_MIPMAP_NEAREST,
  GL_LINEAR_MIPMAP_LINEAR
};

// Mapping MagnificationFilter values to OpenGL values.
//----------------------------------------------------------------------------
static GLint OpenGLMagFilter[6]=
{
  GL_NEAREST,
  GL_LINEAR
};

//----------------------------------------------------------------------------
static const char *MinMagFilterAsString[6]=
{
  "Nearest",
  "Linear",
  "NearestMipmapNearest",
  "NearestMipmapLinear",
  "LinearMipmapNearest",
  "LinearMipmapLinear"
};

//----------------------------------------------------------------------------
static GLenum OpenGLDepthInternalFormat[5]=
{
  GL_DEPTH_COMPONENT,
  GL_DEPTH_COMPONENT16,
#ifdef GL_DEPTH_COMPONENT24
  GL_DEPTH_COMPONENT24,
#else
  GL_DEPTH_COMPONENT16,
#endif
#ifdef GL_DEPTH_COMPONENT32
  GL_DEPTH_COMPONENT32,
#else
  GL_DEPTH_COMPONENT16,
#endif
#ifdef GL_DEPTH_COMPONENT32F
  GL_DEPTH_COMPONENT32F
#else
  GL_DEPTH_COMPONENT16
#endif
};

//----------------------------------------------------------------------------
static GLenum OpenGLDepthInternalFormatType[5]=
{
  GL_UNSIGNED_INT,
  GL_UNSIGNED_INT,
  GL_UNSIGNED_INT,
  GL_UNSIGNED_INT,
#ifdef GL_DEPTH_COMPONENT32F
  GL_FLOAT
#else
  GL_UNSIGNED_INT
#endif
};

/*
static const char *DepthInternalFormatFilterAsString[6]=
{
  "Native",
  "Fixed16",
  "Fixed24",
  "Fixed32",
  "Float32"
};
*/

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkTextureObject);

//----------------------------------------------------------------------------
vtkTextureObject::vtkTextureObject()
{
  this->Context = NULL;
  this->Handle = 0;
  this->NumberOfDimensions = 0;
  this->Target = 0;
  this->Components = 0;
  this->Width = 0;
  this->Height = 0;
  this->Depth = 0;
  this->RequireTextureInteger = false;
  this->SupportsTextureInteger = false;
  this->RequireTextureFloat = false;
  this->SupportsTextureFloat = false;
  this->RequireDepthBufferFloat = false;
  this->SupportsDepthBufferFloat = false;
  this->AutoParameters = 1;
  this->WrapS = Repeat;
  this->WrapT = Repeat;
  this->WrapR = Repeat;
  this->MinificationFilter = Nearest;
  this->MagnificationFilter = Nearest;
  this->LinearMagnification = false;
  this->MinLOD = -1000.0f;
  this->MaxLOD = 1000.0f;
  this->BaseLevel = 0;
  this->MaxLevel = 0;
  this->DepthTextureCompare = false;
  this->DepthTextureCompareFunction = Lequal;
  this->GenerateMipmap = false;
  this->ShaderProgram = NULL;
  this->BorderColor[0] = 0.0f;
  this->BorderColor[1] = 0.0f;
  this->BorderColor[2] = 0.0f;
  this->BorderColor[3] = 0.0f;

  this->ResetFormatAndType();
}

//----------------------------------------------------------------------------
vtkTextureObject::~vtkTextureObject()
{
  this->DestroyTexture();
}

//----------------------------------------------------------------------------
bool vtkTextureObject::IsSupported(vtkOpenGLRenderWindow* vtkNotUsed(win),
      bool requireTexFloat,
      bool requireDepthFloat,
      bool requireTexInt)
{
#if GL_ES_VERSION_2_0 != 1
  bool texFloat = true;
  if (requireTexFloat)
    {
    texFloat = (glewIsSupported("GL_ARB_texture_float") != 0);
    }

  bool depthFloat = true;
  if (requireDepthFloat)
    {
    depthFloat = (glewIsSupported("GL_ARB_depth_buffer_float") != 0);
    }

  bool texInt = true;
  if (requireTexInt)
    {
    texInt = (glewIsSupported("GL_EXT_texture_integer") != 0);
    }

#else
  bool texFloat = !requireTexFloat;
  bool depthFloat = !requireDepthFloat;
  bool texInt = !requireTexInt;
#if GL_ES_VERSION_3_0 == 1
  texFloat = true;
  depthFloat = true;  // I think this is the case
  texInt = true;
#endif
#endif

  return texFloat && depthFloat && texInt;
}

//----------------------------------------------------------------------------
bool vtkTextureObject::LoadRequiredExtensions(vtkOpenGLRenderWindow *renWin)
{
#if GL_ES_VERSION_2_0 != 1
  this->SupportsTextureInteger =
    (glewIsSupported("GL_EXT_texture_integer") != 0);

  this->SupportsTextureFloat =
    (glewIsSupported("GL_ARB_texture_float") != 0);

  this->SupportsDepthBufferFloat =
    (glewIsSupported("GL_ARB_depth_buffer_float") != 0);
#else
  // some of these may have extensions etc for ES 2.0
  // setting to false right now as I do not know
  this->SupportsTextureInteger = false;
  this->SupportsTextureFloat = false;
  this->SupportsDepthBufferFloat = false;
#if GL_ES_VERSION_3_0 == 1
  this->SupportsTextureInteger = true;
  this->SupportsTextureFloat = true;
  this->SupportsDepthBufferFloat = true;
#endif
#endif

  return this->IsSupported(renWin,
    this->RequireTextureFloat,
    this->RequireDepthBufferFloat,
    this->RequireTextureInteger);
}

//----------------------------------------------------------------------------
void vtkTextureObject::SetContext(vtkOpenGLRenderWindow* renWin)
{
  // avoid pointless reassignment
  if (this->Context == renWin)
    {
    return;
    }
  // free previous resources
  this->DestroyTexture();
  this->Context = NULL;
  this->Modified();
  // all done if assigned null
  if (!renWin)
    {
    return;
    }

  if (!this->LoadRequiredExtensions(renWin) )
    {
    vtkErrorMacro("Required OpenGL extensions not supported by the context.");
    return;
    }
  // initialize
  this->Context = renWin;
  this->Context->MakeCurrent();
}

//----------------------------------------------------------------------------
vtkOpenGLRenderWindow* vtkTextureObject::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
void vtkTextureObject::DestroyTexture()
{
  // deactivate it first
  this->Deactivate();

  // because we don't hold a reference to the render
  // context we don't have any control on when it is
  // destroyed. In fact it may be destroyed before
  // we are(eg smart pointers), in which case we should
  // do nothing.
  if (this->Context && this->Handle)
    {
    GLuint tex = this->Handle;
    glDeleteTextures(1, &tex);
    vtkOpenGLCheckErrorMacro("failed at glDeleteTexture");
    }
  this->Handle = 0;
  this->NumberOfDimensions = 0;
  this->Target = 0;
  this->Components = 0;
  this->Width = this->Height = this->Depth = 0;
  this->ResetFormatAndType();
}

//----------------------------------------------------------------------------
void vtkTextureObject::CreateTexture()
{
  assert(this->Context);

  // reuse the existing handle if we have one
  if (!this->Handle)
    {
    GLuint tex=0;
    glGenTextures(1, &tex);
    vtkOpenGLCheckErrorMacro("failed at glGenTextures");
    this->Handle=tex;

    if (this->Target)
      {
      glBindTexture(this->Target, this->Handle);
      vtkOpenGLCheckErrorMacro("failed at glBindTexture");

      // See: http://www.opengl.org/wiki/Common_Mistakes#Creating_a_complete_texture
      // turn off mip map filter or set the base and max level correctly. here
      // both are done.
      glTexParameteri(this->Target, GL_TEXTURE_MIN_FILTER,
                      this->GetMinificationFilterMode(this->MinificationFilter));
      glTexParameteri(this->Target, GL_TEXTURE_MAG_FILTER,
                      this->GetMagnificationFilterMode(this->MagnificationFilter));

      glTexParameteri(this->Target, GL_TEXTURE_WRAP_S,
                      this->GetWrapSMode(this->WrapS));
      glTexParameteri(this->Target, GL_TEXTURE_WRAP_T,
                      this->GetWrapTMode(this->WrapT));

#if defined(GL_TEXTURE_3D)
      if (this->Target == GL_TEXTURE_3D)
        {
        glTexParameteri(this->Target, GL_TEXTURE_WRAP_R,
                        this->GetWrapRMode(this->WrapR));
        }
#endif

#ifdef GL_TEXTURE_BASE_LEVEL
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
#endif

#ifdef GL_TEXTURE_MAX_LEVEL
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
#endif

      glBindTexture(this->Target, 0);
      }
    }
}

//---------------------------------------------------------------------------
int vtkTextureObject::GetTextureUnit()
{
  if (this->Context)
    {
    return this->Context->GetTextureUnitForTexture(this);
    }
  return -1;
}

//---------------------------------------------------------------------------
void vtkTextureObject::Activate()
{
  // activate a free texture unit for this texture
  this->Context->ActivateTexture(this);
  this->Bind();
}

//---------------------------------------------------------------------------
void vtkTextureObject::Deactivate()
{
  if (this->Context)
    {
    this->Context->ActivateTexture(this);
    this->UnBind();
    this->Context->DeactivateTexture(this);
    }
}

//---------------------------------------------------------------------------
void vtkTextureObject::ReleaseGraphicsResources(vtkWindow *win)
{
  vtkOpenGLRenderWindow *rwin =
   vtkOpenGLRenderWindow::SafeDownCast(win);

  // It is almost guarenteed that in case of valid handle, we will have
  // value other than zero. A check like this may be required at other
  // places as well.
  if (this->Handle && rwin)
    {
    rwin->ActivateTexture(this);
    this->UnBind();
    rwin->DeactivateTexture(this);
    GLuint tex = this->Handle;
    glDeleteTextures(1, &tex);
    this->Handle = 0;
    this->NumberOfDimensions = 0;
    this->Target =0;
    this->Format = 0;
    this->Type = 0;
    this->Components = 0;
    this->Width = this->Height = this->Depth = 0;
    }
  if (this->ShaderProgram)
    {
    this->ShaderProgram->ReleaseGraphicsResources(win);
    delete this->ShaderProgram;
    this->ShaderProgram = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkTextureObject::Bind()
{
  assert(this->Context);
  assert(this->Handle);

  glBindTexture(this->Target, this->Handle);
  vtkOpenGLCheckErrorMacro("failed at glBindTexture");

  if (this->AutoParameters && (this->GetMTime() > this->SendParametersTime))
    {
    this->SendParameters();
    }
}

//----------------------------------------------------------------------------
void vtkTextureObject::UnBind()
{
  if (this->Target)
    {
    glBindTexture(this->Target, 0);
    vtkOpenGLCheckErrorMacro("failed at glBindTexture(0)");
    }
}

//----------------------------------------------------------------------------
bool vtkTextureObject::IsBound()
{
  bool result=false;
  if(this->Context && this->Handle)
    {
    GLenum target=0; // to avoid warnings.
    switch(this->Target)
      {
#if defined(GL_TEXTURE_1D) && defined(GL_TEXTURE_BINDING_1D)
      case GL_TEXTURE_1D:
        target=GL_TEXTURE_BINDING_1D;
        break;
#endif
      case GL_TEXTURE_2D:
        target=GL_TEXTURE_BINDING_2D;
        break;
#if defined(GL_TEXTURE_3D) && defined(GL_TEXTURE_BINDING_3D)
      case GL_TEXTURE_3D:
        target=GL_TEXTURE_BINDING_3D;
        break;
#endif
      default:
        assert("check: impossible case" && 0);
        break;
      }
    GLint objectId;
    glGetIntegerv(target,&objectId);
    result=static_cast<GLuint>(objectId)==this->Handle;
    }
  return result;
}

//----------------------------------------------------------------------------
void vtkTextureObject::SendParameters()
{
  assert("pre: is_bound" && this->IsBound());

  glTexParameteri(this->Target,GL_TEXTURE_WRAP_S, OpenGLWrap[this->WrapS]);
  glTexParameteri(this->Target,GL_TEXTURE_WRAP_T, OpenGLWrap[this->WrapT]);

#ifdef GL_TEXTURE_WRAP_R
  glTexParameteri(
        this->Target,
        GL_TEXTURE_WRAP_R,
        OpenGLWrap[this->WrapR]);
#endif

  glTexParameteri(
        this->Target,
        GL_TEXTURE_MIN_FILTER,
        OpenGLMinFilter[this->MinificationFilter]);

  glTexParameteri(
        this->Target,
        GL_TEXTURE_MAG_FILTER,
        OpenGLMagFilter[this->MagnificationFilter]);

#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1
#if GL_ES_VERSION_3_0 != 1
  glTexParameterfv(this->Target, GL_TEXTURE_BORDER_COLOR, this->BorderColor);

  if(DepthTextureCompare)
    {
    glTexParameteri(
          this->Target,
          GL_TEXTURE_COMPARE_MODE,
          GL_COMPARE_R_TO_TEXTURE);
    }
  else
    {
    glTexParameteri(
          this->Target,
          GL_TEXTURE_COMPARE_MODE,
          GL_NONE);
    }
#endif

  glTexParameterf(this->Target, GL_TEXTURE_MIN_LOD, this->MinLOD);
  glTexParameterf(this->Target, GL_TEXTURE_MAX_LOD, this->MaxLOD);
  glTexParameteri(this->Target, GL_TEXTURE_BASE_LEVEL, this->BaseLevel);
  glTexParameteri(this->Target, GL_TEXTURE_MAX_LEVEL, this->MaxLevel);

  glTexParameteri(
        this->Target,
        GL_TEXTURE_COMPARE_FUNC,
        OpenGLDepthTextureCompareFunction[this->DepthTextureCompareFunction]);
#endif

  vtkOpenGLCheckErrorMacro("failed after SendParameters");
  this->SendParametersTime.Modified();
}


#if GL_ES_VERSION_2_0 != 1

//----------------------------------------------------------------------------
unsigned int vtkTextureObject::GetInternalFormat(int vtktype, int numComps,
                                                 bool shaderSupportsTextureInt)
{
  if (this->InternalFormat)
    {
    return this->InternalFormat;
    }

  // 1 or 2 components not supported as render target in FBO on GeForce<8
  // force internal format component to be 3 or 4, even if client format is 1
  // or 2 components.
  // see spec 2.1 page 137 (pdf page 151) in section 3.6.4 Rasterization of
  // Pixel Rectangles: "Conversion to RGB": this step is applied only if
  // the format is LUMINANCE or LUMINANCE_ALPHA:
  // L: R=L, G=L, B=L
  // LA: R=L, G=L, B=L, A=A

  // pre-condition
  if(vtktype==VTK_VOID && numComps != 1)
    {
    vtkErrorMacro("Depth component texture must have 1 component only (" <<
                  numComps << " requested");
    this->InternalFormat = 0;
    return this->InternalFormat;
    }
  const bool oldGeForce=!this->SupportsTextureInteger;

  if(oldGeForce && numComps<3)
    {
    numComps+=2;
    }
  // DON'T DEAL WITH VTK_CHAR as this is platform dependent.
  switch (vtktype)
    {
    case VTK_VOID:
      // numComps can be 3 on GeForce<8.
      this->InternalFormat = GL_DEPTH_COMPONENT;
      break;
    case VTK_SIGNED_CHAR:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            this->InternalFormat = GL_LUMINANCE8I_EXT;
            break;
          case 2:
            this->InternalFormat = GL_LUMINANCE_ALPHA8I_EXT;
            break;
          case 3:
            this->InternalFormat = GL_RGB8I_EXT;
            break;
          case 4:
            this->InternalFormat = GL_RGBA8I_EXT;
            break;
          }
        }
      else
        {
        switch (numComps)
          {
          case 1:
            this->InternalFormat = GL_LUMINANCE8;
            break;
          case 2:
            this->InternalFormat = GL_LUMINANCE8_ALPHA8;
            break;
          case 3:
            this->InternalFormat = GL_RGB8;
            break;
          case 4:
            this->InternalFormat = GL_RGBA8;
            break;
          }
        }
      break;
    case VTK_UNSIGNED_CHAR:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            this->InternalFormat = GL_LUMINANCE8UI_EXT;
            break;
          case 2:
            this->InternalFormat = GL_LUMINANCE_ALPHA8UI_EXT;
            break;
          case 3:
            this->InternalFormat = GL_RGB8UI_EXT;
            break;
          case 4:
            this->InternalFormat = GL_RGBA8UI_EXT;
            break;
          }
        }
      else
        {
        switch (numComps)
          {
          case 1:
            this->InternalFormat = GL_LUMINANCE8;
            break;
          case 2:
            this->InternalFormat = GL_LUMINANCE8_ALPHA8;
            break;
          case 3:
            this->InternalFormat = GL_RGB8;
            break;
          case 4:
            this->InternalFormat = GL_RGBA8;
            break;
          }
        }
      break;
    case VTK_SHORT:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            this->InternalFormat = GL_LUMINANCE16I_EXT;
            break;
          case 2:
            this->InternalFormat = GL_LUMINANCE_ALPHA16I_EXT;
            break;
          case 3:
            this->InternalFormat = GL_RGB16I_EXT;
            break;
          case 4:
            this->InternalFormat = GL_RGBA16I_EXT;
            break;
          }
        }
      else
        {
        switch (numComps)
          {
          case 1:
            if(this->SupportsTextureFloat)
              {
              this->InternalFormat = GL_LUMINANCE32F_ARB;
              break;
  //            this->InternalFormat = GL_LUMINANCE16; // not supported as a render target
              }
            else
              {
              vtkGenericWarningMacro("Unsupported type!");
              this->InternalFormat = 0;
              break;
              }
          case 2:
            if(this->SupportsTextureFloat)
              {
              this->InternalFormat = GL_LUMINANCE_ALPHA32F_ARB;
              break;
              //            this->InternalFormat = GL_LUMINANCE16_ALPHA16; // not supported as a render target
              }
            else
              {
              vtkGenericWarningMacro("Unsupported type!");
              this->InternalFormat = 0;
              break;
              }
          case 3:
            this->InternalFormat = GL_RGB16;
            break;
          case 4:
            this->InternalFormat = GL_RGBA16;
            break;
          }
        }
      break;
    case VTK_UNSIGNED_SHORT:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            this->InternalFormat = GL_LUMINANCE16UI_EXT;
            break;
          case 2:
            this->InternalFormat = GL_LUMINANCE_ALPHA16UI_EXT;
            break;
          case 3:
            this->InternalFormat = GL_RGB16UI_EXT;
            break;
          case 4:
            this->InternalFormat = GL_RGBA16UI_EXT;
            break;
          }
        }
      else
        {
        switch (numComps)
          {
          case 1:
            if(this->SupportsTextureFloat)
              {
              this->InternalFormat = GL_LUMINANCE32F_ARB;
              break;
  //      this->InternalFormat = GL_LUMINANCE16; // not supported as a render target
              }
            else
              {
              vtkGenericWarningMacro("Unsupported type!");
              this->InternalFormat = 0;
              break;
              }
          case 2:
            if(this->SupportsTextureFloat)
              {
              this->InternalFormat = GL_LUMINANCE_ALPHA32F_ARB;
              break;
  //      this->InternalFormat = GL_LUMINANCE16_ALPHA16; // not supported as a render target
              }
            else
              {
               vtkGenericWarningMacro("Unsupported type!");
               this->InternalFormat = 0;
               break;
              }
          case 3:
            this->InternalFormat = GL_RGB16;
            break;
          case 4:
            this->InternalFormat = GL_RGBA16;
            break;
          }
        }
      break;
    case VTK_INT:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            this->InternalFormat = GL_LUMINANCE32I_EXT;
            break;
          case 2:
            this->InternalFormat = GL_LUMINANCE_ALPHA32I_EXT;
            break;
          case 3:
            this->InternalFormat = GL_RGB32I_EXT;
            break;
          case 4:
            this->InternalFormat = GL_RGBA32I_EXT;
            break;
          }
        }
      else
        {
        if(this->SupportsTextureFloat)
          {
          switch (numComps)
            {
            case 1:
              this->InternalFormat = GL_LUMINANCE32F_ARB;
              break;
            case 2:
              this->InternalFormat = GL_LUMINANCE_ALPHA32F_ARB;
              break;
            case 3:
              this->InternalFormat = GL_RGB32F_ARB;
              break;
            case 4:
              this->InternalFormat = GL_RGBA32F_ARB;
              break;
            }
          }
        else
          {
          vtkGenericWarningMacro("Unsupported type!");
          this->InternalFormat = 0;
          }
        }
      break;
    case VTK_UNSIGNED_INT:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            this->InternalFormat = GL_LUMINANCE32UI_EXT;
            break;
          case 2:
            this->InternalFormat = GL_LUMINANCE_ALPHA32UI_EXT;
            break;
          case 3:
            this->InternalFormat = GL_RGB32UI_EXT;
            break;
          case 4:
            this->InternalFormat = GL_RGBA32UI_EXT;
            break;
          }
        }
      else
        {
        if(this->SupportsTextureFloat)
          {
          switch (numComps)
            {
            case 1:
              this->InternalFormat = GL_LUMINANCE32F_ARB;
              break;
            case 2:
              this->InternalFormat = GL_LUMINANCE_ALPHA32F_ARB;
              break;
            case 3:
              this->InternalFormat = GL_RGB32F_ARB;
              break;
            case 4:
              this->InternalFormat = GL_RGBA32F_ARB;
              break;
            }
          }
        else
          {
          vtkGenericWarningMacro("Unsupported type!");
          this->InternalFormat = 0;
          }
        }
      break;
    case VTK_FLOAT:
      if(this->SupportsTextureFloat)
        {
        switch (numComps)
          {
          case 1:
            this->InternalFormat = GL_LUMINANCE32F_ARB;
            break;
          case 2:
            this->InternalFormat = GL_LUMINANCE_ALPHA32F_ARB;
            break;
          case 3:
            this->InternalFormat = GL_RGB32F_ARB;
            break;
          case 4:
            this->InternalFormat = GL_RGBA32F_ARB;
            break;
          }
        }
      else
        {
        vtkGenericWarningMacro("Unsupported type!");
        this->InternalFormat = 0;
        }
      break;
    case VTK_DOUBLE:
      vtkGenericWarningMacro("Unsupported type double!");
      break;
    }
  return this->InternalFormat;
}

#else

//----------------------------------------------------------------------------
unsigned int vtkTextureObject::GetInternalFormat(int vtktype, int numComps,
                                                 bool shaderSupportsTextureInt)
{
  if (this->InternalFormat)
    {
    return this->InternalFormat;
    }
  // pre-condition
  if(vtktype==VTK_VOID && numComps != 1)
    {
    vtkErrorMacro("Depth component texture must have 1 component only (" <<
                  numComps << " requested");
    this->InternalFormat = 0;
    return this->InternalFormat;
    }

  // DON'T DEAL WITH VTK_CHAR as this is platform dependent.
  switch (vtktype)
    {
    case VTK_VOID:
      // numComps can be 3 on GeForce<8.
      this->InternalFormat = GL_DEPTH_COMPONENT;
      break;
    case VTK_UNSIGNED_CHAR:
    case VTK_SIGNED_CHAR:
      switch (numComps)
        {
        case 1:
          this->InternalFormat = GL_LUMINANCE;
          break;
        case 2:
          this->InternalFormat = GL_LUMINANCE_ALPHA;
          break;
        case 3:
          this->InternalFormat = GL_RGB;
          break;
        case 4:
          this->InternalFormat = GL_RGBA;
          break;
        }
      break;
    // TODO add in support for other texture formats
    // supported by OpenGL ES 3.0
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
    case VTK_INT:
    case VTK_UNSIGNED_INT:
    case VTK_DOUBLE:
      vtkGenericWarningMacro("Unsupported texture type!");
    }
  return this->InternalFormat;
}

#endif

//----------------------------------------------------------------------------
void vtkTextureObject::SetInternalFormat(unsigned int glInternalFormat)
{
  if (this->InternalFormat != glInternalFormat)
    {
    this->InternalFormat = glInternalFormat;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
unsigned int vtkTextureObject::GetFormat(int vtktype, int numComps,
                                         bool shaderSupportsTextureInt)
{
  if (this->Format)
    {
    return this->Format;
    }

  if (vtktype == VTK_VOID)
    {
    this->Format = GL_DEPTH_COMPONENT;
    }

#if GL_ES_VERSION_2_0 != 1
  if(this->SupportsTextureInteger && shaderSupportsTextureInt
     && (vtktype==VTK_SIGNED_CHAR||vtktype==VTK_UNSIGNED_CHAR||
         vtktype==VTK_SHORT||vtktype==VTK_UNSIGNED_SHORT||vtktype==VTK_INT||
       vtktype==VTK_UNSIGNED_INT))
    {
    switch (numComps)
      {
      case 1:
        this->Format = GL_LUMINANCE_INTEGER_EXT;
        break;
      case 2:
        this->Format = GL_LUMINANCE_ALPHA_INTEGER_EXT;
        break;
      case 3:
        this->Format = GL_RGB_INTEGER_EXT;
        break;
      case 4:
        this->Format = GL_RGBA_INTEGER_EXT;
        break;
      }
    }
  else
#endif
    {
    switch (numComps)
      {
      case 1:
        this->Format = GL_LUMINANCE;
        break;
      case 2:
        this->Format = GL_LUMINANCE_ALPHA;
        break;
      case 3:
        this->Format = GL_RGB;
        break;
      case 4:
        this->Format = GL_RGBA;
        break;
      }
    }
  return this->Format;
}

//----------------------------------------------------------------------------
void vtkTextureObject::SetFormat(unsigned int glFormat)
{
  if (this->Format != glFormat)
    {
    this->Format = glFormat;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkTextureObject::ResetFormatAndType()
{
  this->Format = 0;
  this->InternalFormat = 0;
  this->Type = 0;
}

//----------------------------------------------------------------------------
static GLenum vtkGetType(int vtk_scalar_type)
{
  // DON'T DEAL with VTK_CHAR as this is platform dependent.
  switch (vtk_scalar_type)
    {
  case VTK_SIGNED_CHAR:
    return GL_BYTE;

  case VTK_UNSIGNED_CHAR:
    return GL_UNSIGNED_BYTE;

  case VTK_SHORT:
    return GL_SHORT;

  case VTK_UNSIGNED_SHORT:
    return GL_UNSIGNED_SHORT;

  case VTK_INT:
    return GL_INT;

  case VTK_UNSIGNED_INT:
    return GL_UNSIGNED_INT;

  case VTK_FLOAT:
  case VTK_VOID: // used for depth component textures.
    return GL_FLOAT;
    }
  return 0;
}

//----------------------------------------------------------------------------
static int vtkGetVTKType(GLenum gltype)
{
   // DON'T DEAL with VTK_CHAR as this is platform dependent.
  switch (gltype)
    {
  case GL_BYTE:
    return VTK_SIGNED_CHAR;

  case GL_UNSIGNED_BYTE:
    return VTK_UNSIGNED_CHAR;

  case GL_SHORT:
    return VTK_SHORT;

  case GL_UNSIGNED_SHORT:
    return VTK_UNSIGNED_SHORT;

  case GL_INT:
    return VTK_INT;

  case GL_UNSIGNED_INT:
    return VTK_UNSIGNED_INT;

  case GL_FLOAT:
    return VTK_FLOAT;
    }

  return 0;
}

//----------------------------------------------------------------------------
int vtkTextureObject::GetDataType(int vtk_scalar_type)
{
  if (!this->Type)
    {
    this->Type = ::vtkGetType(vtk_scalar_type);
    }

  return this->Type;
}

//----------------------------------------------------------------------------
void vtkTextureObject::SetDataType(unsigned int glType)
{
  if (this->Type != glType)
    {
    this->Type = glType;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
unsigned int vtkTextureObject::GetMinificationFilterMode(int vtktype)
{
  switch(vtktype)
    {
    case Nearest:
      return GL_NEAREST;
    case Linear:
      return GL_LINEAR;
    case NearestMipmapNearest:
      return GL_NEAREST_MIPMAP_NEAREST;
    case NearestMipmapLinear:
      return GL_NEAREST_MIPMAP_LINEAR;
    case LinearMipmapNearest:
      return GL_LINEAR_MIPMAP_NEAREST;
    case LinearMipmapLinear:
      return GL_LINEAR_MIPMAP_LINEAR;
    default:
      return GL_NEAREST;
    }
}

//----------------------------------------------------------------------------
unsigned int vtkTextureObject::GetMagnificationFilterMode(int vtktype)
{
  switch(vtktype)
    {
    case Nearest:
      return GL_NEAREST;
    case Linear:
      return GL_LINEAR;
    default:
      return GL_NEAREST;
    }
}

//----------------------------------------------------------------------------
unsigned int vtkTextureObject::GetWrapSMode(int vtktype)
{
  switch(vtktype)
    {
    case ClampToEdge:
      return GL_CLAMP_TO_EDGE;
    case Repeat:
      return GL_REPEAT;
#ifdef GL_CLAMP_TO_BORDER
    case ClampToBorder:
      return GL_CLAMP_TO_BORDER;
#endif
    case MirroredRepeat:
      return GL_MIRRORED_REPEAT;
    default:
      return GL_CLAMP_TO_EDGE;
    }
}

//----------------------------------------------------------------------------
unsigned int vtkTextureObject::GetWrapTMode(int vtktype)
{
  return this->GetWrapSMode(vtktype);
}

//----------------------------------------------------------------------------
unsigned int vtkTextureObject::GetWrapRMode(int vtktype)
{
  return this->GetWrapSMode(vtktype);
}

// 1D  textures are not supported in ES 2.0 or 3.0
#if GL_ES_VERSION_2_0 != 1

//----------------------------------------------------------------------------
bool vtkTextureObject::Create1D(int numComps,
                                vtkPixelBufferObject* pbo,
                                bool shaderSupportsTextureInt)
{
  assert(this->Context);
  assert(pbo->GetContext() == this->Context.GetPointer());

  GLenum target = GL_TEXTURE_1D;

  // Now, detemine texture parameters using the information from the pbo.

  // * internalFormat depends on number of components and the data type.
  GLenum internalFormat = this->GetInternalFormat(pbo->GetType(), numComps,
                                                 shaderSupportsTextureInt);

  // * format depends on the number of components.
  GLenum format = this->GetFormat(pbo->GetType(), numComps,
                                 shaderSupportsTextureInt);

  // * type if the data type in the pbo
  GLenum type = ::vtkGetType(pbo->GetType());

  if (!internalFormat || !format || !type)
    {
    vtkErrorMacro("Failed to detemine texture parameters.");
    return false;
    }

  this->Target = target;
  this->CreateTexture();
  this->Bind();

  pbo->Bind(vtkPixelBufferObject::UNPACKED_BUFFER);

  // Source texture data from the PBO.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage1D(target, 0, static_cast<GLint>(internalFormat),
               static_cast<GLsizei>(pbo->GetSize()/
                                    static_cast<unsigned int>(numComps)),
               0, format,
               type, BUFFER_OFFSET(0));
  vtkOpenGLCheckErrorMacro("failed at glTexImage1D");
  pbo->UnBind();
  this->UnBind();

  this->Target = target;
  this->Format = format;
  this->Type = type;
  this->Components = numComps;
  this->Width = pbo->GetSize();
  this->Height = 1;
  this->Depth =1;
  this->NumberOfDimensions=1;
  return true;
}

//----------------------------------------------------------------------------
bool vtkTextureObject::Create1DFromRaw(unsigned int width, int numComps,
                                       int dataType, void *data)
{
  assert(this->Context);

  // Now determine the texture parameters using the arguments.
  this->GetDataType(dataType);
  this->GetInternalFormat(dataType, numComps, false);
  this->GetFormat(dataType, numComps, false);

  if (!this->InternalFormat || !this->Format || !this->Type)
    {
    vtkErrorMacro("Failed to determine texture parameters.");
    return false;
    }

  GLenum target = GL_TEXTURE_1D;
  this->Target = target;
  this->Components = numComps;
  this->Width = width;
  this->Height = 1;
  this->Depth = 1;
  this->NumberOfDimensions = 1;
  this->CreateTexture();
  this->Bind();

  glTexImage1D(this->Target,
               0,
               this->InternalFormat,
               static_cast<GLsizei> (this->Width),
               0,
               this->Format,
               this->Type,
               static_cast<const GLvoid *> (data));

  vtkOpenGLCheckErrorMacro("failed at glTexImage1D");

  this->UnBind();
  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Create a 1D alpha texture using a raw pointer.
// This is a blocking call. If you can, use PBO instead.
bool vtkTextureObject::CreateAlphaFromRaw(unsigned int width,
                                          int internalFormat,
                                          int rawType,
                                          void* raw)
{
  assert("pre: context_exists" && this->GetContext()!=0);
  assert("pre: raw_exists" && raw!=0);

  assert("pre: valid_internalFormat" && internalFormat>=0
         && internalFormat<NumberOfAlphaFormats);

  // Now, detemine texture parameters using the arguments.
  this->GetDataType(rawType);

  if (!this->InternalFormat)
    {
    this->InternalFormat
      = OpenGLAlphaInternalFormat[internalFormat];
    }

  if (!this->InternalFormat || !this->Type)
    {
    vtkErrorMacro("Failed to detemine texture parameters.");
    return false;
    }

  this->Target = GL_TEXTURE_1D;
  this->Format = GL_ALPHA;
  this->Width = width;
  this->Height = 1;
  this->Depth = 1;
  this->NumberOfDimensions = 1;
  this->Components = 1;

  this->CreateTexture();
  this->Bind();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage1D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
               static_cast<GLsizei>(this->Width), 0,
               this->Format, this->Type, raw);
  vtkOpenGLCheckErrorMacro("failed at glTexImage1D");
  this->UnBind();
  return true;
}

#endif // not ES 2.0 or 3.0

#if GL_ES_VERSION_2_0 != 1 || GL_ES_VERSION_3_0 == 1

//----------------------------------------------------------------------------
bool vtkTextureObject::Create2D(unsigned int width, unsigned int height,
                                int numComps, vtkPixelBufferObject* pbo,
                                bool shaderSupportsTextureInt)
{
  assert(this->Context);
  assert(pbo->GetContext() == this->Context.GetPointer());

  if (pbo->GetSize() < width*height*static_cast<unsigned int>(numComps))
    {
    vtkErrorMacro("PBO size must match texture size.");
    return false;
    }

  // Now, detemine texture parameters using the information from the pbo.
  // * internalFormat depends on number of components and the data type.
  // * format depends on the number of components.
  // * type if the data type in the pbo

  int vtktype = pbo->GetType();
  GLenum type = ::vtkGetType(vtktype);

  GLenum internalFormat
    = this->GetInternalFormat(vtktype, numComps, shaderSupportsTextureInt);

  GLenum format
    = this->GetFormat(vtktype, numComps, shaderSupportsTextureInt);

  if (!internalFormat || !format || !type)
    {
    vtkErrorMacro("Failed to detemine texture parameters.");
    return false;
    }

  GLenum target = GL_TEXTURE_2D;
  this->Target = target;
  this->CreateTexture();
  this->Bind();

  // Source texture data from the PBO.
  pbo->Bind(vtkPixelBufferObject::UNPACKED_BUFFER);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage2D(
        target,
        0,
        internalFormat,
        static_cast<GLsizei>(width),
        static_cast<GLsizei>(height),
        0,
        format,
        type,
        BUFFER_OFFSET(0));

  vtkOpenGLCheckErrorMacro("failed at glTexImage2D");

  pbo->UnBind();
  this->UnBind();

  this->Target = target;
  this->Format = format;
  this->Type = type;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;

  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Create a 2D depth texture using a PBO.
bool vtkTextureObject::CreateDepth(unsigned int width,
                                   unsigned int height,
                                   int internalFormat,
                                   vtkPixelBufferObject *pbo)
{
  assert("pre: context_exists" && this->GetContext()!=0);
  assert("pre: pbo_context_exists" && pbo->GetContext()!=0);
  assert("pre: context_match" && this->GetContext()==pbo->GetContext());
  assert("pre: sizes_match" && pbo->GetSize()==width*height);
  assert("pre: valid_internalFormat" && internalFormat>=0
         && internalFormat<NumberOfDepthFormats);

  GLenum inFormat=OpenGLDepthInternalFormat[internalFormat];
  GLenum type=::vtkGetType(pbo->GetType());

  this->Target=GL_TEXTURE_2D;
  this->Format=GL_DEPTH_COMPONENT;
  this->Type=type;
  this->Width=width;
  this->Height=height;
  this->Depth=1;
  this->NumberOfDimensions=2;
  this->Components=1;

  this->CreateTexture();
  this->Bind();

  pbo->Bind(vtkPixelBufferObject::UNPACKED_BUFFER);

  // Source texture data from the PBO.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(this->Target, 0, static_cast<GLint>(inFormat),
               static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
               this->Format, this->Type, BUFFER_OFFSET(0));
  vtkOpenGLCheckErrorMacro("failed at glTexImage2D");
  pbo->UnBind();
  this->UnBind();
  return true;
}

//----------------------------------------------------------------------------
bool vtkTextureObject::Create3D(unsigned int width, unsigned int height,
                                unsigned int depth, int numComps,
                                vtkPixelBufferObject* pbo,
                                bool shaderSupportsTextureInt)
{
#ifdef GL_TEXTURE_3D
  assert(this->Context);
  assert(this->Context.GetPointer() == pbo->GetContext());

  if (pbo->GetSize() != width*height*depth*static_cast<unsigned int>(numComps))
    {
    vtkErrorMacro("PBO size must match texture size.");
    return false;
    }

  GLenum target = GL_TEXTURE_3D;

  // Now, detemine texture parameters using the information from the pbo.

  // * internalFormat depends on number of components and the data type.
  GLenum internalFormat = this->GetInternalFormat(pbo->GetType(), numComps,
                                                 shaderSupportsTextureInt);

  // * format depends on the number of components.
  GLenum format = this->GetFormat(pbo->GetType(), numComps,
                                  shaderSupportsTextureInt);

  // * type if the data type in the pbo
  GLenum type = ::vtkGetType(pbo->GetType());

  if (!internalFormat || !format || !type)
    {
    vtkErrorMacro("Failed to detemine texture parameters.");
    return false;
    }

  this->Target = target;
  this->CreateTexture();
  this->Bind();

  pbo->Bind(vtkPixelBufferObject::UNPACKED_BUFFER);

  // Source texture data from the PBO.
  glTexImage3D(target, 0, static_cast<GLint>(internalFormat),
                    static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                    static_cast<GLsizei>(depth), 0, format, type,
                    BUFFER_OFFSET(0));

  vtkOpenGLCheckErrorMacro("failed at glTexImage3D");

  pbo->UnBind();
  this->UnBind();

  this->Target = target;
  this->Format = format;
  this->Type = type;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = depth;
  this->NumberOfDimensions = 3;
  return true;

  #else
    return false;
  #endif
}

//----------------------------------------------------------------------------
vtkPixelBufferObject* vtkTextureObject::Download()
{
  assert(this->Context);
  assert(this->Handle);

  vtkPixelBufferObject* pbo = vtkPixelBufferObject::New();
  pbo->SetContext(this->Context);

  int vtktype = ::vtkGetVTKType(this->Type);
  if (vtktype == 0)
    {
    vtkErrorMacro("Failed to determine type.");
    return 0;
    }

  unsigned int size = this->Width* this->Height* this->Depth;

  // doesn't matter which Upload*D method we use since we are not really
  // uploading any data, simply allocating GPU space.
  if (!pbo->Upload1D(vtktype, NULL, size, this->Components, 0))
    {
    vtkErrorMacro("Could not allocate memory for PBO.");
    pbo->Delete();
    return 0;
    }

  pbo->Bind(vtkPixelBufferObject::PACKED_BUFFER);
  this->Bind();

#if GL_ES_VERSION_2_0 != 1
  glGetTexImage(this->Target, 0, this->Format, this->Type, BUFFER_OFFSET(0));
#else
  // you can do something with glReadPixels and binding a texture as a FBO
  // I believe for ES 2.0
#endif

  vtkOpenGLCheckErrorMacro("failed at glGetTexImage");
  this->UnBind();
  pbo->UnBind();

  pbo->SetComponents(this->Components);

  return pbo;
}

//----------------------------------------------------------------------------
bool vtkTextureObject::Create3DFromRaw(unsigned int width, unsigned int height,
                                       unsigned int depth, int numComps,
                                       int dataType, void *data)
{
  assert(this->Context);

  // Now, detemine texture parameters using the arguments.
  this->GetDataType(dataType);
  this->GetInternalFormat(dataType, numComps, false);
  this->GetFormat(dataType, numComps, false);

  if (!this->InternalFormat || !this->Format || !this->Type)
    {
    vtkErrorMacro("Failed to detemine texture parameters.");
    return false;
    }

  this->Target = GL_TEXTURE_3D;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = depth;
  this->NumberOfDimensions = 3;

  this->CreateTexture();
  this->Bind();

  // Source texture data from the PBO.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage3D(
        this->Target,
        0,
        this->InternalFormat,
        static_cast<GLsizei>(this->Width),
        static_cast<GLsizei>(this->Height),
        static_cast<GLsizei>(this->Depth),
        0,
        this->Format,
        this->Type,
        static_cast<const GLvoid *>(data));

  vtkOpenGLCheckErrorMacro("failed at glTexImage2D");

  this->UnBind();

  return true;
}


#endif

//----------------------------------------------------------------------------
bool vtkTextureObject::Create2DFromRaw(unsigned int width, unsigned int height,
                                       int numComps, int dataType, void *data)
{
  assert(this->Context);

  // Now determine the texture parameters using the arguments.
  this->GetDataType(dataType);
  this->GetInternalFormat(dataType, numComps, false);
  this->GetFormat(dataType, numComps, false);

  if (!this->InternalFormat || !this->Format || !this->Type)
    {
    vtkErrorMacro("Failed to determine texture parameters.");
    return false;
    }

  GLenum target = GL_TEXTURE_2D;
  this->Target = target;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;
  this->CreateTexture();
  this->Bind();

  // Source texture data from the PBO.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexImage2D(
        this->Target,
        0,
        this->InternalFormat,
        static_cast<GLsizei>(this->Width),
        static_cast<GLsizei>(this->Height),
        0,
        this->Format,
        this->Type,
        static_cast<const GLvoid *>(data));

  vtkOpenGLCheckErrorMacro("failed at glTexImage2D");

  this->UnBind();
  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Create a 2D depth texture using a raw pointer.
// This is a blocking call. If you can, use PBO instead.
bool vtkTextureObject::CreateDepthFromRaw(unsigned int width,
                                          unsigned int height,
                                          int internalFormat,
                                          int rawType,
                                          void *raw)
{
  assert("pre: context_exists" && this->GetContext()!=0);
  assert("pre: raw_exists" && raw!=0);

  assert("pre: valid_internalFormat" && internalFormat>=0
         && internalFormat<NumberOfDepthFormats);

  // Now, detemine texture parameters using the arguments.
  this->GetDataType(rawType);

  if (!this->InternalFormat)
    {
    this->InternalFormat
      = OpenGLDepthInternalFormat[internalFormat];;
    }

  if (!this->InternalFormat || !this->Type)
    {
    vtkErrorMacro("Failed to detemine texture parameters.");
    return false;
    }

  this->Target = GL_TEXTURE_2D;
  this->Format = GL_DEPTH_COMPONENT;
  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;
  this->Components = 1;

  this->CreateTexture();
  this->Bind();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
               static_cast<GLsizei>(this->Width),
               static_cast<GLsizei>(this->Height), 0,
               this->Format, this->Type,raw);
  vtkOpenGLCheckErrorMacro("failed at glTexImage2D");
  this->UnBind();
  return true;
}

// ----------------------------------------------------------------------------
bool vtkTextureObject::AllocateDepth(unsigned int width, unsigned int height,
                                     int internalFormat)
{
  assert("pre: context_exists" && this->GetContext()!=0);
  assert("pre: valid_internalFormat" && internalFormat>=0
         && internalFormat<NumberOfDepthFormats);

  this->Target = GL_TEXTURE_2D;
  this->Format = GL_DEPTH_COMPONENT;

  // Try to match vtk type to internal fmt
  if (!this->Type)
    {
    this->Type = OpenGLDepthInternalFormatType[internalFormat];
    }

  if (!this->InternalFormat)
    {
    this->InternalFormat = OpenGLDepthInternalFormat[internalFormat];
    }

  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;
  this->Components = 1;

  this->CreateTexture();
  this->Bind();

  glTexImage2D(
          this->Target,
          0,
          static_cast<GLint>(this->InternalFormat),
          static_cast<GLsizei>(this->Width),
          static_cast<GLsizei>(this->Height),
          0,
          this->Format,
          this->Type,
          0);

  vtkOpenGLCheckErrorMacro("failed at glTexImage2D");

  this->UnBind();
  return true;
}

// ----------------------------------------------------------------------------
bool vtkTextureObject::Allocate1D(unsigned int width, int numComps,
                                  int vtkType)
{
#ifdef GL_TEXTURE_1D
  assert(this->Context);

  this->Target = GL_TEXTURE_1D;

  this->GetDataType(vtkType);
  this->GetInternalFormat(vtkType, numComps, false);
  this->GetFormat(vtkType, numComps, false);

  this->Components = numComps;
  this->Width = width;
  this->Height = 1;
  this->Depth = 1;
  this->NumberOfDimensions = 1;

  this->CreateTexture();
  this->Bind();
  glTexImage1D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
               static_cast<GLsizei>(this->Width), 0, this->Format,
               this->Type,0);
  vtkOpenGLCheckErrorMacro("failed at glTexImage1D");
  this->UnBind();
  return true;
#else
  return false;
#endif
}

// ----------------------------------------------------------------------------
// Description:
// Create a 2D color texture but does not initialize its values.
// Internal format is deduced from numComps and vtkType.
bool vtkTextureObject::Allocate2D(unsigned int width,unsigned int height,
                                  int numComps, int vtkType)
{
  assert(this->Context);

  this->Target = GL_TEXTURE_2D;

  this->GetDataType(vtkType);
  this->GetInternalFormat(vtkType, numComps, false);
  this->GetFormat(vtkType, numComps,false);

  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth =1;
  this->NumberOfDimensions=2;

  this->CreateTexture();
  this->Bind();
  glTexImage2D(this->Target, 0, static_cast<GLint>(this->InternalFormat),
               static_cast<GLsizei>(this->Width),
               static_cast<GLsizei>(this->Height),
               0, this->Format, this->Type, 0);
  vtkOpenGLCheckErrorMacro("failed at glTexImage2D");
  this->UnBind();
  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Create a 3D color texture but does not initialize its values.
// Internal format is deduced from numComps and vtkType.
bool vtkTextureObject::Allocate3D(unsigned int width,unsigned int height,
                                  unsigned int depth, int numComps,
                                  int vtkType)
{
#ifdef GL_TEXTURE_3D
  this->Target=GL_TEXTURE_3D;

  if(this->Context==0)
    {
    vtkErrorMacro("No context specified. Cannot create texture.");
    return false;
    }

  this->GetInternalFormat(vtkType, numComps, false);
  this->GetFormat(vtkType, numComps, false);
  this->GetDataType(vtkType);

  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = depth;
  this->NumberOfDimensions = 3;

  this->CreateTexture();
  this->Bind();
  glTexImage3D(this->Target, 0,
               static_cast<GLint>(this->InternalFormat),
               static_cast<GLsizei>(this->Width),
               static_cast<GLsizei>(this->Height),
               static_cast<GLsizei>(this->Depth), 0,
               this->Format, this->Type, 0);
  vtkOpenGLCheckErrorMacro("failed at glTexImage3D");
  this->UnBind();
  return true;
#else
  return false;
#endif
}

//----------------------------------------------------------------------------
bool vtkTextureObject::Create2D(unsigned int width, unsigned int height,
                                int numComps, int vtktype,
                                bool shaderSupportsTextureInt)
{
  assert(this->Context);

  GLenum target = GL_TEXTURE_2D;

  // Now, detemine texture parameters using the information provided.

  this->GetDataType(vtktype);
  this->GetInternalFormat(vtktype, numComps, shaderSupportsTextureInt);
  this->GetFormat(vtktype, numComps, shaderSupportsTextureInt);

  if (!this->InternalFormat || !this->Format || !this->Type)
    {
    vtkErrorMacro("Failed to detemine texture parameters.");
    return false;
    }

  this->Target = target;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = 1;
  this->NumberOfDimensions = 2;

  this->CreateTexture();
  this->Bind();

  // Allocate space for texture, don't upload any data.
  glTexImage2D(target, 0,
               static_cast<GLint>(this->InternalFormat),
               static_cast<GLsizei>(this->Width),
               static_cast<GLsizei>(this->Height),
               0, this->Format, this->Type, NULL);
  vtkOpenGLCheckErrorMacro("failed at glTexImage2D");
  this->UnBind();
  return true;
}

//----------------------------------------------------------------------------
bool vtkTextureObject::Create3D(unsigned int width, unsigned int height,
                                unsigned int depth,
                                int numComps, int vtktype,
                                bool shaderSupportsTextureInt)
{
#ifdef GL_TEXTURE_3D
  assert(this->Context);

  GLenum target = GL_TEXTURE_3D;

  // Now, detemine texture parameters using the information provided.
  this->GetInternalFormat(vtktype, numComps, shaderSupportsTextureInt);
  this->GetFormat(vtktype, numComps, shaderSupportsTextureInt);
  this->GetDataType(vtktype);

  if (!this->InternalFormat || !this->Format || !this->Type)
    {
    vtkErrorMacro("Failed to detemine texture parameters.");
    return false;
    }

  this->Target = target;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth = depth;
  this->NumberOfDimensions = 3;
  this->CreateTexture();
  this->Bind();

  // Allocate space for texture, don't upload any data.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage3D(this->Target, 0,
               static_cast<GLint>(this->InternalFormat),
               static_cast<GLsizei>(this->Width),
               static_cast<GLsizei>(this->Height),
               static_cast<GLsizei>(this->Depth), 0,
               this->Format, this->Type, NULL);
  vtkOpenGLCheckErrorMacro("falied at glTexImage3D");
  this->UnBind();

  return true;
#else
  return false;
#endif
}

// ----------------------------------------------------------------------------
void vtkTextureObject::CopyToFrameBuffer(
  vtkShaderProgram *program, vtkgl::VertexArrayObject *vao)
{
  float minXTexCoord=static_cast<float>(
    static_cast<double>(0.5)/this->Width);
  float minYTexCoord=static_cast<float>(
    static_cast<double>(0.5)/this->Height);

  float maxXTexCoord=static_cast<float>(
    static_cast<double>(this->Width-0.5)/this->Width);
  float maxYTexCoord=static_cast<float>(
    static_cast<double>(this->Height-0.5)/this->Height);

  float tcoords[] = {
    minXTexCoord, minYTexCoord,
    maxXTexCoord, minYTexCoord,
    maxXTexCoord, maxYTexCoord,
    minXTexCoord, maxYTexCoord};

  float verts[] = {
    -1.0f, -1.0f, 0.0f,
    1.0f, -1.0f, 0.0f,
    1.0f, 1.0f, 0.0f,
    -1.0f, 1.0f, 0.0f};

    this->CopyToFrameBuffer(tcoords, verts, program, vao);
}


// ----------------------------------------------------------------------------
void vtkTextureObject::CopyToFrameBuffer(
  int srcXmin, int srcYmin,
  int srcXmax, int srcYmax,
  int dstXmin, int dstYmin,
  int dstSizeX, int dstSizeY,
  vtkShaderProgram *program, vtkgl::VertexArrayObject *vao)
{
  float dstXmax = static_cast<float>(dstXmin+srcXmax-srcXmin);
  float dstYmax = static_cast<float>(dstYmin+srcYmax-srcYmin);

  this->CopyToFrameBuffer(srcXmin, srcYmin, srcXmax, srcYmax,
    dstXmin, dstYmin, dstXmax, dstYmax, dstSizeX, dstSizeY,
    program, vao);
}

// ----------------------------------------------------------------------------
void vtkTextureObject::CopyToFrameBuffer(
  int srcXmin, int srcYmin,
  int srcXmax, int srcYmax,
  int dstXmin, int dstYmin,
  int dstXmax, int dstYmax,
  int dstSizeX, int dstSizeY,
  vtkShaderProgram *program, vtkgl::VertexArrayObject *vao)
{
  assert("pre: positive_srcXmin" && srcXmin>=0);
  assert("pre: max_srcXmax" &&
         static_cast<unsigned int>(srcXmax)<this->GetWidth());
  assert("pre: increasing_x" && srcXmin<=srcXmax);
  assert("pre: positive_srcYmin" && srcYmin>=0);
  assert("pre: max_srcYmax" &&
         static_cast<unsigned int>(srcYmax)<this->GetHeight());
  assert("pre: increasing_y" && srcYmin<=srcYmax);
  assert("pre: positive_dstXmin" && dstXmin>=0);
  assert("pre: positive_dstYmin" && dstYmin>=0);

  float minXTexCoord=static_cast<float>(
    static_cast<double>(srcXmin+0.5)/this->Width);
  float minYTexCoord=static_cast<float>(
    static_cast<double>(srcYmin+0.5)/this->Height);

  float maxXTexCoord=static_cast<float>(
    static_cast<double>(srcXmax+0.5)/this->Width);
  float maxYTexCoord=static_cast<float>(
    static_cast<double>(srcYmax+0.5)/this->Height);

#if GL_ES_VERSION_2_0 != 1
  glPushAttrib(GL_VIEWPORT_BIT);
  glViewport(0,0,dstSizeX,dstSizeY);
#endif

  float tcoords[] = {
    minXTexCoord, minYTexCoord,
    maxXTexCoord, minYTexCoord,
    maxXTexCoord, maxYTexCoord,
    minXTexCoord, maxYTexCoord};

  float verts[] = {
    2.0f*dstXmin/dstSizeX-1.0f, 2.0f*dstYmin/dstSizeY-1.0f, 0.0f,
    2.0f*(dstXmax+1.0f)/dstSizeX-1.0f, 2.0f*dstYmin/dstSizeY-1.0f, 0.0f,
    2.0f*(dstXmax+1.0f)/dstSizeX-1.0f, 2.0f*(dstYmax+1.0f)/dstSizeY-1.0f, 0.0f,
    2.0f*dstXmin/dstSizeX-1.0f, 2.0f*(dstYmax+1.0f)/dstSizeY-1.0f, 0.0f};

    this->CopyToFrameBuffer(tcoords, verts, program, vao);

#if GL_ES_VERSION_2_0 != 1
    glPopAttrib();
#endif
  }

void vtkTextureObject::CopyToFrameBuffer(float *tcoords, float *verts,
  vtkShaderProgram *program, vtkgl::VertexArrayObject *vao)
{
  vtkOpenGLClearErrorMacro();

  // if no program or VAO was provided, then use
  // a simple pass through program and bind this
  // texture to it
  if (!program || !vao)
    {
    if (!this->ShaderProgram)
      {
      this->ShaderProgram = new vtkgl::CellBO;

      // build the shader source code
      std::string VSSource = vtkTextureObjectVS;
      std::string FSSource = vtkTextureObjectFS;
      std::string GSSource;

      // compile and bind it if needed
      vtkShaderProgram *newShader =
        this->Context->GetShaderCache()->ReadyShader(VSSource.c_str(),
                                           FSSource.c_str(),
                                           GSSource.c_str());

      // if the shader changed reinitialize the VAO
      if (newShader != this->ShaderProgram->Program)
        {
        this->ShaderProgram->Program = newShader;
        this->ShaderProgram->vao.ShaderProgramChanged(); // reset the VAO as the shader has changed
        }

      this->ShaderProgram->ShaderSourceTime.Modified();
      }
    else
      {
      this->Context->GetShaderCache()->ReadyShader(this->ShaderProgram->Program);
      }

    // bind and activate this texture
    this->Activate();
    int sourceId = this->GetTextureUnit();
    this->ShaderProgram->Program->SetUniformi("source",sourceId);
    vtkOpenGLRenderWindow::RenderQuad(verts, tcoords, this->ShaderProgram->Program,
      &this->ShaderProgram->vao);
    this->Deactivate();
    }
  else
    {
    vtkOpenGLRenderWindow::RenderQuad(verts, tcoords, program, vao);
    }

  vtkOpenGLCheckErrorMacro("failed after CopyToFrameBuffer")
}

//----------------------------------------------------------------------------
// Description:
// Copy a sub-part of a logical buffer of the framebuffer (color or depth)
// to the texture object. src is the framebuffer, dst is the texture.
// (srcXmin,srcYmin) is the location of the lower left corner of the
// rectangle in the framebuffer. (dstXmin,dstYmin) is the location of the
// lower left corner of the rectangle in the texture. width and height
// specifies the size of the rectangle in pixels.
// If the logical buffer is a color buffer, it has to be selected first with
// glReadBuffer().
// \pre is2D: GetNumberOfDimensions()==2
void vtkTextureObject::CopyFromFrameBuffer(int srcXmin,
                                           int srcYmin,
                                           int vtkNotUsed(dstXmin),
                                           int vtkNotUsed(dstYmin),
                                           int width,
                                           int height)
{
  assert("pre: is2D" && this->GetNumberOfDimensions()==2);

#if 1
  this->Activate();
  glCopyTexImage2D(this->Target,0,this->Format,srcXmin,srcYmin,width,height,0);
  vtkOpenGLCheckErrorMacro("failed at glCopyTexImage2D");

#else

  this->Bind();
  glCopyTexSubImage2D(this->Target,0,dstXmin,dstYmin,srcXmin,srcYmin,width,
                      height);
  vtkOpenGLCheckErrorMacro("failed at glCopyTexSubImage2D");
  this->UnBind();
#endif

}

//----------------------------------------------------------------------------
void vtkTextureObject::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Width: " << this->Width << endl;
  os << indent << "Height: " << this->Height << endl;
  os << indent << "Depth: " << this->Depth << endl;
  os << indent << "Components: " << this->Components << endl;
  os << indent << "Handle: " << this->Handle << endl;
  os << indent << "Target: ";

  switch(this->Target)
    {
#ifdef GL_TEXTURE_1D
    case GL_TEXTURE_1D:
      os << "GL_TEXTURE_1D" << endl;
      break;
#endif
    case  GL_TEXTURE_2D:
      os << "GL_TEXTURE_2D" << endl;
      break;
#ifdef GL_TEXTURE_3D
    case  GL_TEXTURE_3D:
      os << "GL_TEXTURE_3D" << endl;
      break;
#endif
    default:
      os << "unknown value: 0x" << hex << this->Target << dec <<endl;
      break;
    }

  os << indent << "NumberOfDimensions: " << this->NumberOfDimensions << endl;

  os << indent << "WrapS: " << WrapAsString[this->WrapS] << endl;
  os << indent << "WrapT: " << WrapAsString[this->WrapT] << endl;
  os << indent << "WrapR: " << WrapAsString[this->WrapR] << endl;

  os << indent << "MinificationFilter: "
     << MinMagFilterAsString[this->MinificationFilter] << endl;

  os << indent << "MagnificationFilter: "
     << MinMagFilterAsString[this->MagnificationFilter] << endl;

  os << indent << "LinearMagnification: " << this->LinearMagnification << endl;

  os << indent << "MinLOD: " << this->MinLOD << endl;
  os << indent << "MaxLOD: " << this->MaxLOD << endl;
  os << indent << "BaseLevel: " << this->BaseLevel << endl;
  os << indent << "MaxLevel: " << this->MaxLevel << endl;
  os << indent << "DepthTextureCompare: " << this->DepthTextureCompare
     << endl;
  os << indent << "DepthTextureCompareFunction: "
     << DepthTextureCompareFunctionAsString[this->DepthTextureCompareFunction]
     << endl;
  os << indent << "GenerateMipmap: " << this->GenerateMipmap << endl;
}
