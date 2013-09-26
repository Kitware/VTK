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

#include "vtkObjectFactory.h"
#include "vtkPixelBufferObject.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLError.h"

#include "vtkgl.h"
#include <cassert>

//#define VTK_TO_DEBUG
//#define VTK_TO_TIMING

#ifdef VTK_TO_TIMING
#include "vtkTimerLog.h"
#endif

#define BUFFER_OFFSET(i) (static_cast<char *>(NULL) + (i))

// Mapping from DepthTextureCompareFunction values to OpenGL values.

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

// Mapping from DepthTextureMode values to OpenGL values.

static GLint OpenGLDepthTextureMode[3]=
{
  GL_LUMINANCE,
  GL_INTENSITY,
  GL_ALPHA
};

static const char *DepthTextureModeAsString[3]=
{
  "Luminance",
  "Intensity",
  "Alpha"
};

// Mapping from Wrap values to OpenGL values.
static GLint OpenGLWrap[5]=
{
  GL_CLAMP,
  vtkgl::CLAMP_TO_EDGE,
  GL_REPEAT,
  vtkgl::CLAMP_TO_BORDER,
  vtkgl::MIRRORED_REPEAT
};

static const char *WrapAsString[5]=
{
  "Clamp",
  "ClampToEdge",
  "Repeat",
  "ClampToBorder",
  "MirroredRepeat"
};

// Mapping MinificationFilter values to OpenGL values.
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
static GLint OpenGLMagFilter[6]=
{
  GL_NEAREST,
  GL_LINEAR
};

static const char *MinMagFilterAsString[6]=
{
  "Nearest",
  "Linear",
  "NearestMipmapNearest",
  "NearestMipmapLinear",
  "LinearMipmapNearest",
  "LinearMipmapLinear"
};

static GLenum OpenGLDepthInternalFormat[5]=
{
  GL_DEPTH_COMPONENT,
  vtkgl::DEPTH_COMPONENT16,
  vtkgl::DEPTH_COMPONENT24,
  vtkgl::DEPTH_COMPONENT32,
  vtkgl::DEPTH_COMPONENT32F
};

static GLenum OpenGLDepthInternalFormatType[5]=
{
  GL_UNSIGNED_INT,
  GL_UNSIGNED_INT,
  GL_UNSIGNED_INT,
  GL_UNSIGNED_INT,
  GL_FLOAT
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
  this->Format = 0;
  this->Type = 0;
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
  this->BorderColor[0] = 0.0f;
  this->BorderColor[1] = 0.0f;
  this->BorderColor[2] = 0.0f;
  this->BorderColor[3] = 0.0f;
  this->Priority = 1.0f;
  this->MinLOD = -1000.0f;
  this->MaxLOD = 1000.0f;
  this->BaseLevel = 0;
  this->MaxLevel = 0;
  this->DepthTextureCompare = false;
  this->DepthTextureCompareFunction = Lequal;
  this->DepthTextureMode = Luminance;
  this->GenerateMipmap = false;
}

//----------------------------------------------------------------------------
vtkTextureObject::~vtkTextureObject()
{
  this->DestroyTexture();
}

//----------------------------------------------------------------------------
bool vtkTextureObject::IsSupported(vtkRenderWindow* win,
      bool requireTexFloat,
      bool requireDepthFloat,
      bool requireTexInt)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  if (renWin)
    {
    vtkOpenGLExtensionManager* mgr = renWin->GetExtensionManager();

    bool gl12 = mgr->ExtensionSupported("GL_VERSION_1_2")==1;
    bool gl13 = mgr->ExtensionSupported("GL_VERSION_1_3")==1;
    bool gl20 = mgr->ExtensionSupported("GL_VERSION_2_0")==1;

    bool npot=gl20 ||
      mgr->ExtensionSupported("GL_ARB_texture_non_power_of_two");

    bool tex3D=gl12 || mgr->ExtensionSupported("GL_EXT_texture3D");
    bool multi=gl13 || mgr->ExtensionSupported("GL_ARB_multitexture");

    bool texFloat = true;
    if (requireTexFloat)
      {
      texFloat = mgr->ExtensionSupported("GL_ARB_texture_float")==1;
      }

    bool depthFloat = true;
    if (requireDepthFloat)
      {
      depthFloat = mgr->ExtensionSupported("GL_ARB_depth_buffer_float")==1;
      }

    bool texInt = true;
    if (requireTexInt)
      {
      texInt = mgr->ExtensionSupported("GL_EXT_texture_integer")==1;
      }

    return npot && tex3D && multi && texFloat && depthFloat && texInt;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkTextureObject::LoadRequiredExtensions(vtkRenderWindow *renWin)
{
  vtkOpenGLRenderWindow *context = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  if (!context)
    {
    return false;
    }

  vtkOpenGLExtensionManager* mgr = context->GetExtensionManager();

  bool gl12 = mgr->ExtensionSupported("GL_VERSION_1_2")==1;
  bool gl13 = mgr->ExtensionSupported("GL_VERSION_1_3")==1;
  bool gl20 = mgr->ExtensionSupported("GL_VERSION_2_0")==1;

  bool npot = (gl20 ||
    mgr->ExtensionSupported("GL_ARB_texture_non_power_of_two"));

  bool tex3D = (gl12 || mgr->ExtensionSupported("GL_EXT_texture3D"));
  bool multi = (gl13 || mgr->ExtensionSupported("GL_ARB_multitexture"));

  this->SupportsTextureInteger
    = mgr->LoadSupportedExtension("GL_EXT_texture_integer")==1;

  bool texInt
    = (!this->RequireTextureInteger || this->SupportsTextureInteger);

  this->SupportsTextureFloat
    = mgr->ExtensionSupported("GL_ARB_texture_float")==1;

  bool texFloat
    = (!this->RequireTextureFloat || this->SupportsTextureFloat);

  this->SupportsDepthBufferFloat
    = mgr->ExtensionSupported("GL_ARB_depth_buffer_float")==1;

  bool depthFloat
    = (!this->RequireDepthBufferFloat || this->SupportsDepthBufferFloat);

  bool supported
    = npot && tex3D && multi && texInt && texFloat && depthFloat;

  if (supported)
    {
    // tex3D
    if(gl12)
      {
      mgr->LoadSupportedExtension("GL_VERSION_1_2");
      }
    else
      {
      mgr->LoadCorePromotedExtension("GL_EXT_texture3D");
      }
    // multi
    if(gl13)
      {
      mgr->LoadSupportedExtension("GL_VERSION_1_3");
      }
    else
      {
      mgr->LoadCorePromotedExtension("GL_ARB_multitexture");
      }
    // nothing to load for:
    // GL_ARB_texture_non_power_of_two, GL_ARB_texture_float,
    // GL_ARB_depth_buffer_float only defineconstants
    // only using constants from GL_EXT_texture_integer
    }

  return supported;
}

//----------------------------------------------------------------------------
void vtkTextureObject::SetContext(vtkRenderWindow* renWin)
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
  // check for support
  vtkOpenGLRenderWindow *context
     = dynamic_cast<vtkOpenGLRenderWindow*>(renWin);

  if ( !context
    || !this->LoadRequiredExtensions(renWin) )
    {
    vtkErrorMacro("Required OpenGL extensions not supported by the context.");
    return;
    }
  // initialize
  this->Context = renWin;
  this->Context->MakeCurrent();
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkTextureObject::GetContext()
{
  return static_cast<vtkRenderWindow*>(this->Context);
}

//----------------------------------------------------------------------------
void vtkTextureObject::DestroyTexture()
{
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
  this->Target =0;
  this->Format = 0;
  this->Type = 0;
  this->Components = 0;
  this->Width = this->Height = this->Depth = 0;
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
      glTexParameteri(this->Target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(this->Target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

      glTexParameteri(this->Target, GL_TEXTURE_WRAP_S, GL_CLAMP);
      glTexParameteri(this->Target, GL_TEXTURE_WRAP_T, GL_CLAMP);

      glTexParameteri(GL_TEXTURE_2D, vtkgl::TEXTURE_BASE_LEVEL, 0);
      glTexParameteri(GL_TEXTURE_2D, vtkgl::TEXTURE_MAX_LEVEL, 0);

      glBindTexture(this->Target, 0);
      }
    }
}

//---------------------------------------------------------------------------
void vtkTextureObject::Activate(unsigned int texUnit)
{
  vtkgl::ActiveTexture(static_cast<GLenum>(texUnit));
  this->Bind();
}

//---------------------------------------------------------------------------
void vtkTextureObject::Deactivate(unsigned int texUnit)
{
  vtkgl::ActiveTexture(static_cast<GLenum>(texUnit));
  this->UnBind();
}

//----------------------------------------------------------------------------
void vtkTextureObject::Bind()
{
  assert(this->Context);
  assert(this->Handle);

  glBindTexture(this->Target, this->Handle);
  vtkOpenGLCheckErrorMacro("failed at glBindTexture");

  if (this->AutoParameters && (this->GetMTime()>this->SendParametersTime))
    {
    this->SendParameters();
    }
}

//----------------------------------------------------------------------------
void vtkTextureObject::UnBind()
{
  glBindTexture(this->Target, 0);
  vtkOpenGLCheckErrorMacro("failed at glBindTexture(0)");
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
      case GL_TEXTURE_1D:
        target=GL_TEXTURE_BINDING_1D;
        break;
      case GL_TEXTURE_2D:
        target=GL_TEXTURE_BINDING_2D;
        break;
      case vtkgl::TEXTURE_3D:
        target=vtkgl::TEXTURE_BINDING_3D;
        break;
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
  glTexParameteri(this->Target,GL_TEXTURE_WRAP_T,OpenGLWrap[this->WrapT]);

  glTexParameteri(
        this->Target,
        vtkgl::TEXTURE_WRAP_R,
        OpenGLWrap[this->WrapR]);

  glTexParameteri(
        this->Target,
        GL_TEXTURE_MIN_FILTER,
        OpenGLMinFilter[this->MinificationFilter]);

  glTexParameteri(
        this->Target,
        GL_TEXTURE_MAG_FILTER,
        OpenGLMagFilter[this->MagnificationFilter]);

  glTexParameterfv(this->Target,GL_TEXTURE_BORDER_COLOR,this->BorderColor);

  glTexParameterf(this->Target,GL_TEXTURE_PRIORITY,this->Priority);
  glTexParameterf(this->Target,vtkgl::TEXTURE_MIN_LOD,this->MinLOD);
  glTexParameterf(this->Target,vtkgl::TEXTURE_MAX_LOD,this->MaxLOD);
  glTexParameteri(this->Target,vtkgl::TEXTURE_BASE_LEVEL,this->BaseLevel);
  glTexParameteri(this->Target,vtkgl::TEXTURE_MAX_LEVEL,this->MaxLevel);

  glTexParameteri(
        this->Target,
        vtkgl::DEPTH_TEXTURE_MODE,
        OpenGLDepthTextureMode[this->DepthTextureMode]);

  if(DepthTextureCompare)
    {
    glTexParameteri(
          this->Target,
          vtkgl::TEXTURE_COMPARE_MODE,
          vtkgl::COMPARE_R_TO_TEXTURE);
    }
  else
    {
    glTexParameteri(
          this->Target,
          vtkgl::TEXTURE_COMPARE_MODE,
          GL_NONE);
    }

  glTexParameteri(
        this->Target,
        vtkgl::TEXTURE_COMPARE_FUNC,
        OpenGLDepthTextureCompareFunction[this->DepthTextureCompareFunction]);

  vtkOpenGLCheckErrorMacro("failed after SendParameters");
  this->SendParametersTime.Modified();
}

//----------------------------------------------------------------------------
unsigned int vtkTextureObject::GetInternalFormat(int vtktype, int numComps,
                                                 bool shaderSupportsTextureInt)
{
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
      return 0;
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
    return GL_DEPTH_COMPONENT;

    case VTK_SIGNED_CHAR:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            return vtkgl::LUMINANCE8I_EXT;
          case 2:
            return vtkgl::LUMINANCE_ALPHA8I_EXT;
          case 3:
            return vtkgl::RGB8I_EXT;
          case 4:
            return vtkgl::RGBA8I_EXT;
          }
        }
      else
        {
        switch (numComps)
          {
          case 1:
            return GL_LUMINANCE8;
          case 2:
            return GL_LUMINANCE8_ALPHA8;
          case 3:
            return GL_RGB8;
          case 4:
            return GL_RGBA8;
          }
        }

    case VTK_UNSIGNED_CHAR:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            return vtkgl::LUMINANCE8UI_EXT;
          case 2:
            return vtkgl::LUMINANCE_ALPHA8UI_EXT;
          case 3:
            return vtkgl::RGB8UI_EXT;
          case 4:
            return vtkgl::RGBA8UI_EXT;
          }
        }
      else
        {
        switch (numComps)
          {
          case 1:
            return GL_LUMINANCE8;
          case 2:
            return GL_LUMINANCE8_ALPHA8;
          case 3:
            return GL_RGB8;
          case 4:
            return GL_RGBA8;
          }
        }

    case VTK_SHORT:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            return vtkgl::LUMINANCE16I_EXT;
          case 2:
            return vtkgl::LUMINANCE_ALPHA16I_EXT;
          case 3:
            return vtkgl::RGB16I_EXT;
          case 4:
            return vtkgl::RGBA16I_EXT;
          }
        }
      else
        {
        switch (numComps)
          {
          case 1:
            if(this->SupportsTextureFloat)
              {
              return vtkgl::LUMINANCE32F_ARB;
//            return GL_LUMINANCE16; // not supported as a render target
              }
            else
              {
              vtkGenericWarningMacro("Unsupported type!");
              return 0;
              }
          case 2:
            if(this->SupportsTextureFloat)
              {
              return vtkgl::LUMINANCE_ALPHA32F_ARB;
              //            return GL_LUMINANCE16_ALPHA16; // not supported as a render target
              }
            else
              {
              vtkGenericWarningMacro("Unsupported type!");
              return 0;
              }
          case 3:
            return GL_RGB16;
          case 4:
            return GL_RGBA16;
          }
        }

    case VTK_UNSIGNED_SHORT:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            return vtkgl::LUMINANCE16UI_EXT;
          case 2:
            return vtkgl::LUMINANCE_ALPHA16UI_EXT;
          case 3:
            return vtkgl::RGB16UI_EXT;
          case 4:
            return vtkgl::RGBA16UI_EXT;
          }
        }
      else
        {
        switch (numComps)
          {
          case 1:
            if(this->SupportsTextureFloat)
              {
              return vtkgl::LUMINANCE32F_ARB;
//      return GL_LUMINANCE16; // not supported as a render target
              }
            else
              {
              vtkGenericWarningMacro("Unsupported type!");
              return 0;
              }
          case 2:
            if(this->SupportsTextureFloat)
              {
              return vtkgl::LUMINANCE_ALPHA32F_ARB;
//      return GL_LUMINANCE16_ALPHA16; // not supported as a render target
              }
            else
              {
               vtkGenericWarningMacro("Unsupported type!");
               return 0;
              }
          case 3:
            return GL_RGB16;
          case 4:
            return GL_RGBA16;
          }
        }

    case VTK_INT:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            return vtkgl::LUMINANCE32I_EXT;

          case 2:
            return vtkgl::LUMINANCE_ALPHA32I_EXT;

          case 3:
            return vtkgl::RGB32I_EXT;

          case 4:
            return vtkgl::RGBA32I_EXT;
          }
        }
      else
        {
        if(this->SupportsTextureFloat)
          {
          switch (numComps)
            {
            case 1:
              return vtkgl::LUMINANCE32F_ARB;

            case 2:
              return vtkgl::LUMINANCE_ALPHA32F_ARB;

            case 3:
              return vtkgl::RGB32F_ARB;

            case 4:
              return vtkgl::RGBA32F_ARB;
            }
          }
        else
          {
          vtkGenericWarningMacro("Unsupported type!");
          return 0;
          }
        }

    case VTK_UNSIGNED_INT:
      if(this->SupportsTextureInteger && shaderSupportsTextureInt)
        {
        switch (numComps)
          {
          case 1:
            return vtkgl::LUMINANCE32UI_EXT;

          case 2:
            return vtkgl::LUMINANCE_ALPHA32UI_EXT;

          case 3:
            return vtkgl::RGB32UI_EXT;

          case 4:
            return vtkgl::RGBA32UI_EXT;
          }
        }
      else
        {
        if(this->SupportsTextureFloat)
          {
          switch (numComps)
            {
            case 1:
              return vtkgl::LUMINANCE32F_ARB;

            case 2:
              return vtkgl::LUMINANCE_ALPHA32F_ARB;

            case 3:
              return vtkgl::RGB32F_ARB;

            case 4:
              return vtkgl::RGBA32F_ARB;
            }
          }
        else
          {
          vtkGenericWarningMacro("Unsupported type!");
          return 0;
          }
        }

    case VTK_FLOAT:
      if(this->SupportsTextureFloat)
        {
        switch (numComps)
          {
          case 1:
            return vtkgl::LUMINANCE32F_ARB;

          case 2:
            return vtkgl::LUMINANCE_ALPHA32F_ARB;

          case 3:
            return vtkgl::RGB32F_ARB;

          case 4:
            return vtkgl::RGBA32F_ARB;
          }
        }
      else
        {
        vtkGenericWarningMacro("Unsupported type!");
        return 0;
        }
    case VTK_DOUBLE:
      vtkGenericWarningMacro("Unsupported type double!");
    }
  return 0;
}

unsigned int vtkTextureObject::GetFormat(int vtktype, int numComps,
                                         bool shaderSupportsTextureInt)
{
  if (vtktype == VTK_VOID)
    {
    return GL_DEPTH_COMPONENT;
    }

  if(this->SupportsTextureInteger && shaderSupportsTextureInt
     && (vtktype==VTK_SIGNED_CHAR||vtktype==VTK_UNSIGNED_CHAR||
         vtktype==VTK_SHORT||vtktype==VTK_UNSIGNED_SHORT||vtktype==VTK_INT||
       vtktype==VTK_UNSIGNED_INT))
    {
    switch (numComps)
      {
      case 1:
        return vtkgl::LUMINANCE_INTEGER_EXT;
      case 2:
        return vtkgl::LUMINANCE_ALPHA_INTEGER_EXT;
      case 3:
        return vtkgl::RGB_INTEGER_EXT;
      case 4:
        return vtkgl::RGBA_INTEGER_EXT;
      }
    }
  else
    {
    switch (numComps)
      {
      case 1:
        return GL_LUMINANCE;
      case 2:
        return GL_LUMINANCE_ALPHA;
      case 3:
        return GL_RGB;
      case 4:
        return GL_RGBA;
      }
    }
  return 0;
}

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
int vtkTextureObject::GetDataType()
{
  return ::vtkGetVTKType(this->Type);
}

//----------------------------------------------------------------------------
bool vtkTextureObject::Create1D(int numComps,
                                vtkPixelBufferObject* pbo,
                                bool shaderSupportsTextureInt)
{
#ifdef VTK_TO_TIMING
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();
#endif
  assert(this->Context);
  assert(pbo->GetContext() == this->Context);

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
#ifdef VTK_TO_TIMING
  timer->StopTimer();
  double time=timer->GetElapsedTime();
  timer->Delete();
  cerr<<"upload PBO to 1D texture time="<<time<<" seconds."<<endl;
#endif
  return true;
}

//----------------------------------------------------------------------------
bool vtkTextureObject::Create2D(unsigned int width, unsigned int height,
                                int numComps, vtkPixelBufferObject* pbo,
                                bool shaderSupportsTextureInt)
{
#ifdef VTK_TO_TIMING
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();
#endif
  assert(this->Context);
  assert(pbo->GetContext() == this->Context);

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

#ifdef VTK_TO_TIMING
  timer->StopTimer();
  double time=timer->GetElapsedTime();
  timer->Delete();
  cerr<<"upload PBO to 2D texture time="<<time<<" seconds."<<endl;
#endif
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

#ifdef VTK_TO_DEBUG
  cerr << "pbo size=" << pbo->GetSize() << endl;
  cerr << "width=" << width << endl;
  cerr << "height=" << height << endl;
  cerr << "width*height=" << width*height << endl;
#endif

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

#ifdef VTK_TO_DEBUG
  cerr << "width=" << width << endl;
  cerr << "height=" << height << endl;
  cerr << "width*height=" << width*height << endl;
#endif

  assert("pre: valid_internalFormat" && internalFormat>=0
         && internalFormat<NumberOfDepthFormats);

  GLenum inFormat=OpenGLDepthInternalFormat[internalFormat];
  GLenum type=::vtkGetType(rawType);

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

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(this->Target, 0, static_cast<GLint>(inFormat),
               static_cast<GLsizei>(width), static_cast<GLsizei>(height), 0,
               this->Format, this->Type,raw);
  vtkOpenGLCheckErrorMacro("failed at glTexImage2D");
  this->UnBind();
  return true;
}

// ----------------------------------------------------------------------------
bool vtkTextureObject::AllocateDepth(unsigned int width,unsigned int height,
                                     int internalFormat)
{
  assert("pre: context_exists" && this->GetContext()!=0);
  assert("pre: valid_internalFormat" && internalFormat>=0
         && internalFormat<NumberOfDepthFormats);

  this->Target=GL_TEXTURE_2D;
  this->Format=GL_DEPTH_COMPONENT;
  // try to match vtk type to internal fmt
  this->Type=OpenGLDepthInternalFormatType[internalFormat];
  this->Width=width;
  this->Height=height;
  this->Depth=1;
  this->NumberOfDimensions=2;
  this->Components=1;

  this->CreateTexture();
  this->Bind();

  GLenum inFormat=OpenGLDepthInternalFormat[internalFormat];
  glTexImage2D(
          this->Target,
          0,
          static_cast<GLint>(inFormat),
          static_cast<GLsizei>(width),
          static_cast<GLsizei>(height),
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
  assert(this->Context);

  this->Target=GL_TEXTURE_1D;
  GLenum internalFormat = this->GetInternalFormat(vtkType, numComps,
                                                  false);

  // don't care, allocation only, no data transfer
  GLenum format = this->GetFormat(vtkType, numComps,false);

  GLenum type = ::vtkGetType(vtkType);

  this->Format = format;
  this->Type = type;
  this->Components = numComps;
  this->Width = width;
  this->Height = 1;
  this->Depth =1;
  this->NumberOfDimensions=1;

  this->CreateTexture();
  this->Bind();
  glTexImage1D(this->Target, 0, static_cast<GLint>(internalFormat),
               static_cast<GLsizei>(width),0, format, type,0);
  vtkOpenGLCheckErrorMacro("failed at glTexImage1D");
  this->UnBind();
  return true;
}

// ----------------------------------------------------------------------------
// Description:
// Create a 2D color texture but does not initialize its values.
// Internal format is deduced from numComps and vtkType.
bool vtkTextureObject::Allocate2D(unsigned int width,unsigned int height,
                                  int numComps,int vtkType)
{
  assert(this->Context);

  this->Target=GL_TEXTURE_2D;

  GLenum internalFormat = this->GetInternalFormat(vtkType, numComps,
                                                  false);

  // don't care, allocation only, no data transfer
  GLenum format = this->GetFormat(vtkType, numComps,false);

  GLenum type = ::vtkGetType(vtkType);

  this->Format = format;
  this->Type = type;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth =1;
  this->NumberOfDimensions=2;

  this->CreateTexture();
  this->Bind();
  glTexImage2D(this->Target, 0, static_cast<GLint>(internalFormat),
               static_cast<GLsizei>(width), static_cast<GLsizei>(height),
               0, format, type,0);
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
  this->Target=vtkgl::TEXTURE_3D;

  if(this->Context==0)
    {
    vtkErrorMacro("No context specified. Cannot create texture.");
    return false;
    }
  GLenum internalFormat = this->GetInternalFormat(vtkType, numComps,
                                                  false);

  // don't care, allocation only, no data transfer
  GLenum format = this->GetFormat(vtkType, numComps,false);

  GLenum type = ::vtkGetType(vtkType);

  this->Format = format;
  this->Type = type;
  this->Components = numComps;
  this->Width = width;
  this->Height = height;
  this->Depth =depth;
  this->NumberOfDimensions=3;

  this->CreateTexture();
  this->Bind();
  vtkgl::TexImage3D(this->Target, 0, static_cast<GLint>(internalFormat),
                    static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                    static_cast<GLsizei>(depth), 0, format, type,0);
  vtkOpenGLCheckErrorMacro("failed at glTexImage3D");
  this->UnBind();
  return true;
}

//----------------------------------------------------------------------------
bool vtkTextureObject::Create3D(unsigned int width, unsigned int height,
                                unsigned int depth, int numComps,
                                vtkPixelBufferObject* pbo,
                                bool shaderSupportsTextureInt)
{
#ifdef VTK_TO_TIMING
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();
#endif
  assert(this->Context);
  assert(this->Context == pbo->GetContext());

  if (pbo->GetSize() != width*height*depth*static_cast<unsigned int>(numComps))
    {
    vtkErrorMacro("PBO size must match texture size.");
    return false;
    }

  GLenum target = vtkgl::TEXTURE_3D;

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
  vtkgl::TexImage3D(target, 0, static_cast<GLint>(internalFormat),
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

#ifdef VTK_TO_TIMING
  timer->StopTimer();
  double time=timer->GetElapsedTime();
  timer->Delete();
  cerr<<"upload PBO to 3D texture time="<<time<<" seconds."<<endl;
#endif
  return true;
}

//----------------------------------------------------------------------------
vtkPixelBufferObject* vtkTextureObject::Download()
{
#ifdef VTK_TO_TIMING
  vtkTimerLog *timer=vtkTimerLog::New();
  timer->StartTimer();
#endif
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
  glGetTexImage(this->Target, 0, this->Format, this->Type, BUFFER_OFFSET(0));
  vtkOpenGLCheckErrorMacro("failed at glGetTexImage");
  this->UnBind();
  pbo->UnBind();

  pbo->SetComponents(this->Components);

#ifdef VTK_TO_TIMING
  timer->StopTimer();
  double time=timer->GetElapsedTime();
  timer->Delete();
  cerr<<"download texture to PBO, time="<<time<<" seconds."<<endl;
#endif

  return pbo;
}

//----------------------------------------------------------------------------
bool vtkTextureObject::Create2D(unsigned int width, unsigned int height,
                                int numComps, int vtktype,
                                bool shaderSupportsTextureInt)
{
  assert(this->Context);

  GLenum target = GL_TEXTURE_2D;

  // Now, detemine texture parameters using the information provided.
  // * internalFormat depends on number of components and the data type.
  GLenum internalFormat = this->GetInternalFormat(vtktype, numComps,
                                                 shaderSupportsTextureInt);

  // * format depends on the number of components.
  GLenum format = this->GetFormat(vtktype, numComps,
                                 shaderSupportsTextureInt);

  // * type if the data type in the pbo
  GLenum type = ::vtkGetType(vtktype);

  if (!internalFormat || !format || !type)
    {
    vtkErrorMacro("Failed to detemine texture parameters.");
    return false;
    }

  this->Target = target;
  this->CreateTexture();
  this->Bind();

  // Allocate space for texture, don't upload any data.
  glTexImage2D(target, 0, static_cast<GLint>(internalFormat),
               static_cast<GLsizei>(width), static_cast<GLsizei>(height),
               0, format, type, NULL);
  vtkOpenGLCheckErrorMacro("failed at glTexImage2D");
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

//----------------------------------------------------------------------------
bool vtkTextureObject::Create3D(unsigned int width, unsigned int height,
                                unsigned int depth,
                                int numComps, int vtktype,
                                bool shaderSupportsTextureInt)
{
  assert(this->Context);

  GLenum target = vtkgl::TEXTURE_3D;

  // Now, detemine texture parameters using the information provided.
  // * internalFormat depends on number of components and the data type.
  GLenum internalFormat = this->GetInternalFormat(vtktype, numComps,
                                                 shaderSupportsTextureInt);

  // * format depends on the number of components.
  GLenum format = this->GetFormat(vtktype, numComps,
                                 shaderSupportsTextureInt);

  // * type if the data type in the pbo
  GLenum type = ::vtkGetType(vtktype);

  if (!internalFormat || !format || !type)
    {
    vtkErrorMacro("Failed to detemine texture parameters.");
    return false;
    }

  this->Target = target;
  this->CreateTexture();
  this->Bind();

  // Allocate space for texture, don't upload any data.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  vtkgl::TexImage3D(target, 0, static_cast<GLint>(internalFormat),
                    static_cast<GLsizei>(width), static_cast<GLsizei>(height),
                    static_cast<GLsizei>(depth), 0, format, type, NULL);
  vtkOpenGLCheckErrorMacro("falied at glTexImage3D");
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
}

// ----------------------------------------------------------------------------
void vtkTextureObject::CopyToFrameBuffer(int srcXmin,
                                         int srcYmin,
                                         int srcXmax,
                                         int srcYmax,
                                         int dstXmin,
                                         int dstYmin,
                                         int width,
                                         int height)
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
  assert("pre: positive_width" && width>0);
  assert("pre: positive_height" && height>0);
  assert("pre: x_fit" && dstXmin+(srcXmax-srcXmin)<width);
  assert("pre: y_fit" && dstYmin+(srcYmax-srcYmin)<height);

  vtkOpenGLClearErrorMacro();

  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(0.0,width,0.0,height,-1,1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  glPushAttrib(GL_VIEWPORT_BIT|GL_POLYGON_BIT|GL_TEXTURE_BIT);
  vtkgl::ActiveTexture(vtkgl::TEXTURE0);
  glMatrixMode(GL_TEXTURE);
  glPushMatrix();
  glLoadIdentity();

  glViewport(0,0,width,height);
  glDepthRange(0.0,1.0);
  glDisable(GL_POLYGON_OFFSET_FILL);

  GLfloat minXTexCoord=static_cast<GLfloat>(
    static_cast<double>(srcXmin)/this->Width);
  GLfloat minYTexCoord=static_cast<GLfloat>(
    static_cast<double>(srcYmin)/this->Height);

  GLfloat maxXTexCoord=static_cast<GLfloat>(
    static_cast<double>(srcXmax+1)/this->Width);
  GLfloat maxYTexCoord=static_cast<GLfloat>(
    static_cast<double>(srcYmax+1)/this->Height);

  GLfloat dstXmax=static_cast<GLfloat>(dstXmin+srcXmax-srcXmin);
  GLfloat dstYmax=static_cast<GLfloat>(dstYmin+srcYmax-srcYmin);

  // rasterization rules are different from points, lines and polygons.
  // the following vertex coordinates are only valid for 1:1 mapping in the
  // case of polygons.

  // Draw a quad.
  glBegin(GL_TRIANGLE_FAN);
  glTexCoord2f(minXTexCoord,minYTexCoord);
  glVertex2f(static_cast<GLfloat>(dstXmin), static_cast<GLfloat>(dstYmin));
  glTexCoord2f(maxXTexCoord, minYTexCoord);
  glVertex2f(dstXmax+1, static_cast<GLfloat>(dstYmin));
  glTexCoord2f(maxXTexCoord, maxYTexCoord);
  glVertex2f(dstXmax+1, dstYmax+1);
  glTexCoord2f(minXTexCoord, maxYTexCoord);
  glVertex2f(static_cast<GLfloat>(dstXmin), dstYmax+1);
  glEnd();

  glMatrixMode(GL_TEXTURE);
  glPopMatrix();

  glPopAttrib();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();

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
                                           int dstXmin,
                                           int dstYmin,
                                           int width,
                                           int height)
{
  assert("pre: is2D" && this->GetNumberOfDimensions()==2);
  this->Bind();
  glCopyTexSubImage2D(this->Target,0,dstXmin,dstYmin,srcXmin,srcYmin,width,
                      height);
  vtkOpenGLCheckErrorMacro("failed at glCopyTexSubImage2D");
  this->UnBind();
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
    case GL_TEXTURE_1D:
      os << "GL_TEXTURE_1D" << endl;
      break;
    case  GL_TEXTURE_2D:
      os << "GL_TEXTURE_2D" << endl;
      break;
    case  vtkgl::TEXTURE_3D:
      os << "vtkgl::TEXTURE_3D" << endl;
      break;
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

  os << indent << "BorderColor: (" << this->BorderColor[0] << ","
     << this->BorderColor[1] << "," << this->BorderColor[2] << ","
     << this->BorderColor[3] << endl;

  os << indent << "Priority: " << this->Priority <<  endl;
  os << indent << "MinLOD: " << this->MinLOD << endl;
  os << indent << "MaxLOD: " << this->MaxLOD << endl;
  os << indent << "BaseLevel: " << this->BaseLevel << endl;
  os << indent << "MaxLevel: " << this->MaxLevel << endl;
  os << indent << "DepthTextureCompare: " << this->DepthTextureCompare
     << endl;
  os << indent << "DepthTextureCompareFunction: "
     << DepthTextureCompareFunctionAsString[this->DepthTextureCompareFunction]
     << endl;
  os << indent << "DepthTextureMode: "
     << DepthTextureModeAsString[this->DepthTextureMode] << endl;
  os << indent << "GenerateMipmap: " << this->GenerateMipmap << endl;
}
