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

#include "vtkPixelBufferObject.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkgl.h"

//#define VTK_TO_TIMING

#ifdef VTK_TO_TIMING
#include "vtkTimerLog.h"
#endif

#define BUFFER_OFFSET(i) (static_cast<char *>(NULL) + (i))

vtkStandardNewMacro(vtkTextureObject);
vtkCxxRevisionMacro(vtkTextureObject, "1.3");
//----------------------------------------------------------------------------
vtkTextureObject::vtkTextureObject()
{
  this->Context = 0;
  this->Handle = 0;
  this->NumberOfDimensions = 0;
  this->Target =0;
  this->Format = 0;
  this->Type = 0;
  this->Components = 0;
  this->Width=this->Height=this->Depth=0;
  this->SupportsTextureInteger=false;
}

//----------------------------------------------------------------------------
vtkTextureObject::~vtkTextureObject()
{
  this->SetContext(0);
}

//----------------------------------------------------------------------------
bool vtkTextureObject::IsSupported(vtkRenderWindow* win)
{
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(win);
  if (renWin)
    {
    vtkOpenGLExtensionManager* mgr = renWin->GetExtensionManager();
    
    bool gl12=mgr->ExtensionSupported("GL_VERSION_1_2");
    bool gl20=mgr->ExtensionSupported("GL_VERSION_2_0");
    
    bool npot=gl20 ||
      mgr->ExtensionSupported("GL_ARB_texture_non_power_of_two");
    
    bool tex3D=gl12 || mgr->ExtensionSupported("GL_EXT_texture3D");
    
    bool floatTextures=mgr->ExtensionSupported("GL_ARB_texture_float");
    
    return npot && tex3D && floatTextures;
    }
  return false;
}

//----------------------------------------------------------------------------
bool vtkTextureObject::LoadRequiredExtensions(vtkOpenGLExtensionManager* mgr)
{
  // Optional extension, requires GeForce8
  this->SupportsTextureInteger =
    mgr->LoadSupportedExtension("GL_EXT_texture_integer") != 0;
  
  bool gl12=mgr->ExtensionSupported("GL_VERSION_1_2");
  bool gl20=mgr->ExtensionSupported("GL_VERSION_2_0");
  
  bool npot=gl20 ||
    mgr->ExtensionSupported("GL_ARB_texture_non_power_of_two");
  
  bool tex3D=gl12 || mgr->ExtensionSupported("GL_EXT_texture3D");
  
  bool floatTextures=mgr->ExtensionSupported("GL_ARB_texture_float");
  
  bool supported=npot && tex3D && floatTextures;
  
  if(supported)
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
    // npot does not provide new functions, nothing to do.
    // texture_float does not provide new functions, nothing to do.
    }
  return supported;
}

//----------------------------------------------------------------------------
void vtkTextureObject::SetContext(vtkRenderWindow* renWin)
{
  if (this->Context == renWin)
    {
    return;
    }

  this->DestroyTexture();

  vtkOpenGLRenderWindow* openGLRenWin = vtkOpenGLRenderWindow::SafeDownCast(renWin);
  this->Context = openGLRenWin;
  if (openGLRenWin)
    {
    if (!this->LoadRequiredExtensions(openGLRenWin->GetExtensionManager()))
      {
      this->Context = 0;
      vtkErrorMacro("Required OpenGL extensions not supported by the context.");
      }
    }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkRenderWindow* vtkTextureObject::GetContext()
{
  return this->Context;
}

//----------------------------------------------------------------------------
void vtkTextureObject::DestroyTexture()
{
  if (this->Context && this->Handle)
    {
    GLuint tex = this->Handle;
    glDeleteTextures(1, &tex);
    vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
    this->Handle=0;
    }
  this->NumberOfDimensions = 0;
  this->Target =0;
  this->Format = 0;
  this->Type = 0;
  this->Components = 0;
  this->Width=this->Height=this->Depth=0;
}

//----------------------------------------------------------------------------
void vtkTextureObject::CreateTexture()
{
  if (this->Context && !this->Handle)
    {
    GLuint tex=0;
    glGenTextures(1, &tex);
    vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
    this->Handle=tex;

    if (this->Target)
      {
      glBindTexture(this->Target, this->Handle);
      vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
      // NVidia drivers has some initialization bug. min_filter and
      // mag_filter has to explicitly initialized even if the OpenGL spec
      // states there is a default value.
      glTexParameteri(this->Target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
      glTexParameteri(this->Target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
      
      glTexParameteri(this->Target, GL_TEXTURE_WRAP_S, GL_CLAMP);
      vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
      glTexParameteri(this->Target, GL_TEXTURE_WRAP_T, GL_CLAMP);
      vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");

      glBindTexture(this->Target, 0);
      vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
      }
    }
}

//----------------------------------------------------------------------------
void vtkTextureObject::Bind()
{
  if (this->Context && this->Handle)
    {
    glBindTexture(this->Target, this->Handle);
    vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
    }
}


//----------------------------------------------------------------------------
void vtkTextureObject::UnBind()
{
  if (this->Context && this->Handle)
    {
    glBindTexture(this->Target, 0);
    vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
    }
}

//----------------------------------------------------------------------------
int vtkTextureObject::GetInternalFormat(int vtktype, int numComps,
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
            return vtkgl::LUMINANCE32F_ARB;
//            return GL_LUMINANCE16; // not supported as a render target
          case 2:
            return vtkgl::LUMINANCE_ALPHA32F_ARB;
//            return GL_LUMINANCE16_ALPHA16; // not supported as a render target
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
            return vtkgl::LUMINANCE32F_ARB;
//      return GL_LUMINANCE16; // not supported as a render target
            
          case 2:
            return vtkgl::LUMINANCE_ALPHA32F_ARB;
//      return GL_LUMINANCE16_ALPHA16; // not supported as a render target
            
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
      
    case VTK_FLOAT:
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
      
    case VTK_DOUBLE:
      vtkGenericWarningMacro("Unsupported type double!");
    }
  return 0;
}

int vtkTextureObject::GetFormat(int vtktype, int numComps,
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

static GLint vtkGetType(int vtk_scalar_type)
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

static int vtkGetVTKType(GLint gltype)
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
  return ::vtkGetType(this->Type);
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
  if (!this->Context)
    {
    vtkErrorMacro("No context specified. Cannot create texture.");
    return false;
    }

  if (pbo->GetContext() != this->Context)
    {
    vtkErrorMacro("Context mismatch. Cannot load data.");
    return false;
    }
  
  GLenum target = GL_TEXTURE_1D;

  // Now, detemine texture parameters using the information from the pbo.
  
  // * internalFormat depends on number of components and the data type.
  GLint internalFormat = this->GetInternalFormat(pbo->GetType(), numComps,
                                                 shaderSupportsTextureInt);

  // * format depends on the number of components.
  GLint format = this->GetFormat(pbo->GetType(), numComps,
                                 shaderSupportsTextureInt);

  // * type if the data type in the pbo
  GLint type = ::vtkGetType(pbo->GetType());

  if (!internalFormat || !format || !type)
    {
    vtkErrorMacro("Failed to detemine texture paramters.");
    return false;
    }

  this->Target = target;
  this->CreateTexture();
  this->Bind();

  pbo->Bind(vtkPixelBufferObject::UNPACKED_BUFFER);
  //vtkgl::ClampColorARB(vtkgl::CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  // Source texture data from the PBO.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage1D(target, 0, internalFormat,
    pbo->GetSize()/numComps, 0, format, type, BUFFER_OFFSET(0));
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
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
  cout<<"upload PBO to 1D texture time="<<time<<" seconds."<<endl;
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
  if (!this->Context)
    {
    vtkErrorMacro("No context specified. Cannot create texture.");
    return false;
    }

  if (pbo->GetContext() != this->Context)
    {
    vtkErrorMacro("Context mismatch. Cannot load data.");
    return false;
    }

  if (pbo->GetSize() != width*height*numComps)
    {
    vtkErrorMacro("PBO size must match texture size.");
    return false;
    }

  GLenum target = GL_TEXTURE_2D;

  // Now, detemine texture parameters using the information from the pbo.

  // * internalFormat depends on number of components and the data type.
  GLint internalFormat =this->GetInternalFormat(pbo->GetType(), numComps,
                                                shaderSupportsTextureInt);

  // * format depends on the number of components.
  GLint format = this->GetFormat(pbo->GetType(), numComps,
                                 shaderSupportsTextureInt);

  // * type if the data type in the pbo
  GLint type = ::vtkGetType(pbo->GetType());

  if (!internalFormat || !format || !type)
    {
    vtkErrorMacro("Failed to detemine texture paramters.");
    return false;
    }

  this->Target = target;
  this->CreateTexture();
  this->Bind();

  pbo->Bind(vtkPixelBufferObject::UNPACKED_BUFFER);  
  //vtkgl::ClampColorARB(vtkgl::CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  // Source texture data from the PBO.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(target, 0, internalFormat,
    width, height, 0, format, type, BUFFER_OFFSET(0));
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
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
  cout<<"upload PBO to 2D texture time="<<time<<" seconds."<<endl;
#endif
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
  if (!this->Context)
    {
    vtkErrorMacro("No context specified. Cannot create texture.");
    return false;
    }

  if (pbo->GetContext() != this->Context)
    {
    vtkErrorMacro("Context mismatch. Cannot load data.");
    return false;
    }

  if (pbo->GetSize() != width*height*depth*numComps)
    {
    vtkErrorMacro("PBO size must match texture size.");
    return false;
    }

  GLenum target = vtkgl::TEXTURE_3D;

  // Now, detemine texture parameters using the information from the pbo.

  // * internalFormat depends on number of components and the data type.
  GLint internalFormat = this->GetInternalFormat(pbo->GetType(), numComps,
                                                 shaderSupportsTextureInt);

  // * format depends on the number of components.
  GLint format = this->GetFormat(pbo->GetType(), numComps,
                                 shaderSupportsTextureInt);

  // * type if the data type in the pbo
  GLint type = ::vtkGetType(pbo->GetType());

  if (!internalFormat || !format || !type)
    {
    vtkErrorMacro("Failed to detemine texture paramters.");
    return false;
    }

  this->Target = target;
  this->CreateTexture();
  this->Bind();

  pbo->Bind(vtkPixelBufferObject::UNPACKED_BUFFER);
  //vtkgl::ClampColorARB(vtkgl::CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  // Source texture data from the PBO.
  vtkgl::TexImage3D(target, 0, internalFormat,
    width, height, depth, 0, format, type, BUFFER_OFFSET(0));
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
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
  cout<<"upload PBO to 3D texture time="<<time<<" seconds."<<endl;
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
  if (!this->Context || !this->Handle)
    {
    vtkErrorMacro("Texture must be created before downloading.");
    return 0;
    }

  vtkPixelBufferObject* pbo = vtkPixelBufferObject::New();
  pbo->SetContext(this->Context);

  int vtktype = ::vtkGetVTKType(this->Type);
  if (vtktype == 0)
    {
    vtkErrorMacro("Failed to determine type.");
    return 0;
    }

  unsigned int size = this->Width* this->Height* this->Depth;

  // doesnt matter which Upload*D method we use since we are not really
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
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  this->UnBind();
  pbo->UnBind();

#ifdef VTK_TO_TIMING
  timer->StopTimer();
  double time=timer->GetElapsedTime();
  timer->Delete();
  cout<<"download texture to PBO, time="<<time<<" seconds."<<endl;
#endif

  return pbo;
}

//----------------------------------------------------------------------------
bool vtkTextureObject::Create2D(unsigned int width, unsigned int height,
                                int numComps, int vtktype,
                                bool shaderSupportsTextureInt)
{
  if (!this->Context)
    {
    vtkErrorMacro("No context specified. Cannot create texture.");
    return false;
    }

  GLenum target = GL_TEXTURE_2D;

  // Now, detemine texture parameters using the information provided.
  // * internalFormat depends on number of components and the data type.
  GLint internalFormat = this->GetInternalFormat(vtktype, numComps,
                                                 shaderSupportsTextureInt);

  // * format depends on the number of components.
  GLint format = this->GetFormat(vtktype, numComps,
                                 shaderSupportsTextureInt);

  // * type if the data type in the pbo
  GLint type = ::vtkGetType(vtktype);

  if (!internalFormat || !format || !type)
    {
    vtkErrorMacro("Failed to detemine texture paramters.");
    return false;
    }

  this->Target = target;
  this->CreateTexture();
  this->Bind();

  //vtkgl::ClampColorARB(vtkgl::CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  // Allocate space for texture, don't upload any data.
  glTexImage2D(target, 0, internalFormat,
    width, height, 0, format, type, NULL);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
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
  if (!this->Context)
    {
    vtkErrorMacro("No context specified. Cannot create texture.");
    return false;
    }

  GLenum target = vtkgl::TEXTURE_3D;

  // Now, detemine texture parameters using the information provided.
  // * internalFormat depends on number of components and the data type.
  GLint internalFormat = this->GetInternalFormat(vtktype, numComps,
                                                 shaderSupportsTextureInt);

  // * format depends on the number of components.
  GLint format = this->GetFormat(vtktype, numComps,
                                 shaderSupportsTextureInt);

  // * type if the data type in the pbo
  GLint type = ::vtkGetType(vtktype);

  if (!internalFormat || !format || !type)
    {
    vtkErrorMacro("Failed to detemine texture paramters.");
    return false;
    }

  this->Target = target;
  this->CreateTexture();
  this->Bind();

  //vtkgl::ClampColorARB(vtkgl::CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
  // Allocate space for texture, don't upload any data.
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  vtkgl::TexImage3D(target, 0, internalFormat,
    width, height, depth, 0, format, type, NULL);
  vtkGraphicErrorMacro(this->Context,"__FILE__ __LINE__");
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
}

