/*=========================================================================

Program:   Visualization Toolkit
Module:    TestFBOImplementation.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This code test to make sure vtkOpenGLExtensionManager can properly get
// extension functions that can be used.  To do this, we convolve an image
// with a kernel for a Laplacian filter.  This requires the use of functions
// defined in OpenGL 1.2, which should be available pretty much everywhere
// but still has functions that can be loaded as extensions.

#include "vtkConeSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkActor.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCamera.h"
#include "vtkCallbackCommand.h"
#include "vtkUnsignedCharArray.h"
#include "vtkRegressionTestImage.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkgl.h"
#include <cassert>

bool ARB_texture_rectangle_supported=false;
bool depth_texture_supported=false; // OpenGL 1.4 or GL_ARB_depth_texture
bool srgb_texture_supported=false; // OpenGL 2.1 or GL_EXT_texture_sRGB
bool float_texture_supported=false; // GL_ARB_texture_float
bool integer_texture_supported=false; // GL_EXT_texture_integer (GeForce 8)

// ----------------------------------------------------------------------------
// Description:
// Return a string matching the OpenGL errorCode.
// \post result_exists: result!=0
const char *OpenGLErrorMessage2(GLenum errorCode)
{
  const char *result;
  switch(errorCode)
    {
    case GL_NO_ERROR:
      result="No error";
      break;
    case GL_INVALID_ENUM:
      result="Invalid enum";
      break;
    case GL_INVALID_VALUE:
      result="Invalid value";
      break;
    case GL_INVALID_OPERATION:
      result="Invalid operation";
      break;
    case GL_STACK_OVERFLOW:
      result="stack overflow";
      break;
    case GL_STACK_UNDERFLOW:
      result="stack underflow";
      break;
    case GL_OUT_OF_MEMORY:
      result="out of memory";
      break;
    case vtkgl::INVALID_FRAMEBUFFER_OPERATION_EXT:
      // GL_EXT_framebuffer_object, 310
      result="invalid framebuffer operation ext";
      break;
    default:
      result="unknown error";
    }
  assert("post: result_exists" && result!=0);
  return result;
}

void CheckOpenGLError(const char *message)
{
  GLenum errorCode=glGetError();
  if(errorCode!=GL_NO_ERROR)
    {
    cout << "ERROR:"
         << OpenGLErrorMessage2(errorCode) << message << endl;
    }
}

void CheckMinValidValue(GLint value,
                        GLint specMinValue)
{
  if(value<specMinValue)
    {
    cout<<"This OpenGL implementation is not compliant with the OpenGL";
    cout<<"specifications."<<endl;
    }
}

void CheckMinValidFValue(GLfloat value,
                         GLfloat specMinValue)
{
  if(value<specMinValue)
    {
    cout<<"This OpenGL implementation is not compliant with the OpenGL";
    cout<<"specifications."<<endl;
    }
}

void CheckMaxValidValue(GLint value,
                        GLint specMaxValue)
{
  if(value>specMaxValue)
    {
    cout<<"This OpenGL implementation is not compliant with the OpenGL";
    cout<<"specifications."<<endl;
    }
}

void DisplayFrameBufferAttachments();
void DisplayFrameBufferAttachment(unsigned int uattachment);

// ----------------------------------------------------------------------------
// Description:
// Display the status of the current framebuffer on the standard output.
void CheckFrameBufferStatus()
{
  GLenum status;
  status = vtkgl::CheckFramebufferStatusEXT(vtkgl::FRAMEBUFFER_EXT);
  switch(status)
    {
    case 0:
      cout << "call to vtkgl::CheckFramebufferStatusEXT generates an error."
           << endl;
      break;
    case vtkgl::FRAMEBUFFER_COMPLETE_EXT:
      cout<<"framebuffer is complete"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_UNSUPPORTED_EXT:
      cout << "framebuffer is unsupported" << endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
      cout << "framebuffer has an attachment error"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
      cout << "framebuffer has a missing attachment"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
      cout << "framebuffer has bad dimensions"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_FORMATS_EXT:
      cout << "framebuffer has bad formats"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
      cout << "framebuffer has bad draw buffer"<<endl;
      break;
    case vtkgl::FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
      cout << "framebuffer has bad read buffer"<<endl;
      break;
    default:
      cout << "Unknown framebuffer status=0x" << hex<< status << dec << endl;
    }
  // DO NOT REMOVE THE FOLLOWING COMMENTED LINE. FOR DEBUGGING PURPOSE.
  DisplayFrameBufferAttachments();
}

// ----------------------------------------------------------------------------
// Description:
// Display all the attachments of the current framebuffer object.
void DisplayFrameBufferAttachments()
{
  GLint framebufferBinding;
  glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&framebufferBinding);
  CheckOpenGLError("after getting FRAMEBUFFER_BINDING_EXT");
  if(framebufferBinding==0)
    {
    cout<<"Current framebuffer is bind to the system one"<<endl;
    }
  else
    {
    cout<<"Current framebuffer is bind to framebuffer object "
        <<framebufferBinding<<endl;

    GLint maxColorAttachments;
    glGetIntegerv(vtkgl::MAX_COLOR_ATTACHMENTS_EXT,&maxColorAttachments);
    CheckOpenGLError("after getting MAX_COLOR_ATTACHMENTS_EXT");
    int i=0;
    while(i<maxColorAttachments)
      {
      cout<<"color attachement "<<i<<":"<<endl;
      DisplayFrameBufferAttachment(vtkgl::COLOR_ATTACHMENT0_EXT+i);
      ++i;
      }
    cout<<"depth attachement :"<<endl;
    DisplayFrameBufferAttachment(vtkgl::DEPTH_ATTACHMENT_EXT);
    cout<<"stencil attachement :"<<endl;
    DisplayFrameBufferAttachment(vtkgl::STENCIL_ATTACHMENT_EXT);
    }
}

// ----------------------------------------------------------------------------
// Description:
// Display a given attachment for the current framebuffer object.
void DisplayFrameBufferAttachment(unsigned int uattachment)
{
  GLenum attachment=static_cast<GLenum>(uattachment);

  GLint params;
  vtkgl::GetFramebufferAttachmentParameterivEXT(
    vtkgl::FRAMEBUFFER_EXT,attachment,
    vtkgl::FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT,&params);

  CheckOpenGLError("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_TYPE_EXT");

  switch(params)
    {
    case GL_NONE:
      cout<<" this attachment is empty"<<endl;
      break;
    case GL_TEXTURE:
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,&params);
      CheckOpenGLError("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT");
      cout<<" this attachment is a texture with name: "<<params<<endl;
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT,&params);
      CheckOpenGLError("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_LEVEL_EXT");
      cout<<" its mipmap level is: "<<params<<endl;
#if 0
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT,&params);
      CheckOpenGLError("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_CUBE_MAP_FACE_EXT");
      if(params==0)
        {
        cout<<" this is not a cube map texture."<<endl;
        }
      else
        {
        cout<<" this is a cube map texture and the image is contained in face "
            <<params<<endl;
        }
#endif
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT,&params);

      CheckOpenGLError("after getting FRAMEBUFFER_ATTACHMENT_TEXTURE_3D_ZOFFSET_EXT");
      if(params==0)
        {
        cout<<" this is not 3D texture."<<endl;
        }
      else
        {
        cout<<" this is a 3D texture and the zoffset of the attached image is "
            <<params<<endl;
        }
      break;
    case vtkgl::RENDERBUFFER_EXT:
      cout<<" this attachment is a renderbuffer"<<endl;
      vtkgl::GetFramebufferAttachmentParameterivEXT(
        vtkgl::FRAMEBUFFER_EXT,attachment,
        vtkgl::FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT,&params);
      CheckOpenGLError("after getting FRAMEBUFFER_ATTACHMENT_OBJECT_NAME_EXT");
      cout<<" this attachment is a renderbuffer with name: "<<params<<endl;

      vtkgl::BindRenderbufferEXT(vtkgl::RENDERBUFFER_EXT,params);
      CheckOpenGLError(
        "after getting binding the current RENDERBUFFER_EXT to params");

      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_WIDTH_EXT,
                                           &params);
      CheckOpenGLError("after getting RENDERBUFFER_WIDTH_EXT");
      cout<<" renderbuffer width="<<params<<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_HEIGHT_EXT,
                                           &params);
      CheckOpenGLError("after getting RENDERBUFFER_HEIGHT_EXT");
      cout<<" renderbuffer height="<<params<<endl;
      vtkgl::GetRenderbufferParameterivEXT(
        vtkgl::RENDERBUFFER_EXT,vtkgl::RENDERBUFFER_INTERNAL_FORMAT_EXT,
        &params);
      CheckOpenGLError("after getting RENDERBUFFER_INTERNAL_FORMAT_EXT");

      cout<<" renderbuffer internal format=0x"<< hex<<params<<dec<<endl;

      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_RED_SIZE_EXT,
                                           &params);
      CheckOpenGLError("after getting RENDERBUFFER_RED_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the red component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_GREEN_SIZE_EXT,
                                           &params);
      CheckOpenGLError("after getting RENDERBUFFER_GREEN_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the green component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_BLUE_SIZE_EXT,
                                           &params);
      CheckOpenGLError("after getting RENDERBUFFER_BLUE_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the blue component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_ALPHA_SIZE_EXT,
                                           &params);
      CheckOpenGLError("after getting RENDERBUFFER_ALPHA_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the alpha component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(vtkgl::RENDERBUFFER_EXT,
                                           vtkgl::RENDERBUFFER_DEPTH_SIZE_EXT,
                                           &params);
      CheckOpenGLError("after getting RENDERBUFFER_DEPTH_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the depth component="<<params
          <<endl;
      vtkgl::GetRenderbufferParameterivEXT(
        vtkgl::RENDERBUFFER_EXT,vtkgl::RENDERBUFFER_STENCIL_SIZE_EXT,&params);
      CheckOpenGLError("after getting RENDERBUFFER_STENCIL_SIZE_EXT");
      cout<<" renderbuffer actual resolution for the stencil component="
          <<params<<endl;
      break;
    default:
      cout<<" unexcepted value."<<endl;
      break;
    }
}


const char *BooleanToString(GLboolean value)
{
  if(value)
    {
    return "True";
    }
  else
    {
    return "False";
    }
}

const char *TextureCompressionFormat(GLint value)
{
  const char *result;
  switch(value)
    {
    //
    case vtkgl::COMPRESSED_RGB_S3TC_DXT1_EXT:
      result="GL_COMPRESSED_RGB_S3TC_DXT1_EXT";
      break;
    case vtkgl::COMPRESSED_RGBA_S3TC_DXT1_EXT:
      result="GL_COMPRESSED_RGBA_S3TC_DXT1_EXT";
      break;
    case vtkgl::COMPRESSED_RGBA_S3TC_DXT3_EXT:
      result="GL_COMPRESSED_RGBA_S3TC_DXT3_EXT";
      break;
    case vtkgl::COMPRESSED_RGBA_S3TC_DXT5_EXT:
      result="GL_COMPRESSED_RGBA_S3TC_DXT5_EXT";
      break;

      // extension 3DFX_texture_compression_FXT1
    case vtkgl::COMPRESSED_RGB_FXT1_3DFX:
      result="GL_COMPRESSED_RGB_FXT1_3DFX";
      break;
    case vtkgl::COMPRESSED_RGBA_FXT1_3DFX:
      result="GL_COMPRESSED_RGBA_FXT1_3DFX";
      break;

      // extension GL_EXT_texture_sRGB (or OpenGL>=2.1)
    case vtkgl::COMPRESSED_SRGB_S3TC_DXT1_EXT:
      result="GL_COMPRESSED_SRGB_S3TC_DXT1_EXT";
      break;
    case vtkgl::COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT:
      result="GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT1_EXT";
      break;
    case vtkgl::COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT:
      result="GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT3_EXT";
      break;
    case vtkgl::COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT:
      result="GL_COMPRESSED_SRGB_ALPHA_S3TC_DXT5_EXT";
      break;
    default:
      result="unknown texture compression format";
      break;
    }
  return result;
}

const char *MinMagModToString(int minMagMode)
{
  const char *result;
  switch(minMagMode)
    {
    case GL_NEAREST:
      result="GL_NEAREST";
      break;
    case GL_LINEAR:
      result="GL_LINEAR";
      break;
    case GL_NEAREST_MIPMAP_NEAREST:
      result="GL_NEAREST_MIPMAP_NEAREST";
      break;
    case GL_NEAREST_MIPMAP_LINEAR:
      result="GL_NEAREST_MIPMAP_LINEAR";
      break;
    case GL_LINEAR_MIPMAP_NEAREST:
      result="GL_LINEAR_MIPMAP_NEAREST";
      break;
    case GL_LINEAR_MIPMAP_LINEAR:
      result="GL_LINEAR_MIPMAP_LINEAR";
      break;
    default:
      result=0;
//      assert("check: impossible case." && 0);
      break;
    }
  return result;
}

const char *InternalTextureFormatToString(int internalFormat)
{
  const char *result;
  switch(internalFormat)
    {
    case 1:
      result="backwards compatible GL_LUMINANCE";
      break;
    case 2:
      result="backwards compatible GL_LUMINANCE_ALPHA";
      break;
    case 3:
      result="backwards compatible GL_RGB";
      break;
    case 4:
      result="backwards compatible GL_RGBA";
      break;
    case GL_ALPHA:
      result="GL_ALPHA";
      break;
    case GL_DEPTH_COMPONENT:
      result="GL_DEPTH_COMPONENT";
      break;
    case GL_LUMINANCE:
      result="GL_LUMINANCE";
      break;
    case GL_LUMINANCE_ALPHA:
      result="GL_LUMINANCE_ALPHA";
      break;
    case GL_INTENSITY:
      result="GL_INTENSITY";
      break;
    case GL_RGB:
      result="GL_RGB";
      break;
    case GL_RGBA:
      result="GL_RGBA";
      break;
      // sized internal format
    case GL_ALPHA4:
      result="GL_ALPHA4";
      break;
    case GL_ALPHA8:
      result="GL_ALPHA8";
      break;
    case GL_ALPHA12:
      result="GL_ALPHA12";
      break;
    case GL_ALPHA16:
      result="GL_ALPHA16";
      break;
    case vtkgl::DEPTH_COMPONENT16:
      result="GL_DEPTH_COMPONENT16";
      break;
    case vtkgl::DEPTH_COMPONENT24:
      result="GL_DEPTH_COMPONENT24";
      break;
    case vtkgl::DEPTH_COMPONENT32:
      result="GL_DEPTH_COMPONENT32";
      break;
    case GL_LUMINANCE4:
      result="GL_LUMINANCE4";
      break;
    case GL_LUMINANCE8:
      result="GL_LUMINANCE8";
      break;
    case GL_LUMINANCE12:
      result="GL_LUMINANCE12";
      break;
    case GL_LUMINANCE16:
      result="GL_LUMINANCE16";
      break;
    case GL_LUMINANCE4_ALPHA4:
      result="GL_LUMINANCE4_ALPHA4";
      break;
    case GL_LUMINANCE6_ALPHA2:
      result="GL_LUMINANCE6_ALPHA2";
      break;
    case GL_LUMINANCE8_ALPHA8:
      result="GL_LUMINANCE8_ALPHA8";
      break;
    case GL_LUMINANCE12_ALPHA4:
      result="GL_LUMINANCE12_ALPHA4";
      break;
    case GL_LUMINANCE12_ALPHA12:
      result="GL_LUMINANCE12_ALPHA12";
      break;
    case GL_LUMINANCE16_ALPHA16:
      result="GL_LUMINANCE16_ALPHA16";
      break;
    case GL_INTENSITY4:
      result="GL_INTENSITY4";
      break;
    case GL_INTENSITY8:
      result="GL_INTENSITY8";
      break;
    case GL_INTENSITY12:
      result="GL_INTENSITY12";
      break;
    case GL_INTENSITY16:
      result="GL_INTENSITY16";
      break;
    case GL_R3_G3_B2:
      result="GL_R3_G3_B2";
      break;
    case GL_RGB4:
      result="GL_RGB4";
      break;
    case GL_RGB5:
      result="GL_RGB5";
      break;
    case GL_RGB8:
      result="GL_RGB8";
      break;
    case GL_RGB10:
      result="GL_RGB10";
      break;
    case GL_RGB12:
      result="GL_RGB12";
      break;
    case GL_RGB16:
      result="GL_RGB16";
      break;
    case GL_RGBA2:
      result="GL_RGBA2";
      break;
    case GL_RGBA4:
      result="GL_RGBA4";
      break;
    case GL_RGB5_A1:
      result="GL_RGB5_A1";
      break;
    case GL_RGBA8:
      result="GL_RGBA8";
      break;
    case GL_RGB10_A2:
      result="GL_RGB10_A2";
      break;
    case GL_RGBA12:
      result="GL_RGBA12";
      break;
    case GL_RGBA16:
      result="GL_RGBA16";
      break;
      // OpenGL 2.1 (GL_EXT_texture_sRGB)
    case vtkgl::SRGB8:
      result="GL_SRGB8";
      break;
    case vtkgl::SRGB8_ALPHA8:
      result="GL_SRGB8_ALPHA8";
      break;
    case vtkgl::SLUMINANCE8:
      result="GL_SLUMINANCE8";
      break;
    case vtkgl::SLUMINANCE8_ALPHA8:
      result="GL_SLUMINANCE8_ALPHA8";
      break;
      // Provided by GL_ARB_texture_float
    case vtkgl::RGBA32F_ARB:
      result="GL_RGBA32F_ARB";
      break;
    case vtkgl::RGB32F_ARB:
      result="GL_RGB32F_ARB";
      break;
    case vtkgl::ALPHA32F_ARB:
      result="GL_ALPHA32F_ARB";
      break;
    case vtkgl::INTENSITY32F_ARB:
      result="GL_INTENSITY32F_ARB";
      break;
    case vtkgl::LUMINANCE32F_ARB:
      result="GL_LUMINANCE32F_ARB";
      break;
    case vtkgl::LUMINANCE_ALPHA32F_ARB:
      result="GL_LUMINANCE_ALPHA32F_ARB";
      break;
    case vtkgl::RGBA16F_ARB:
      result="GL_RGBA16F_ARB";
      break;
    case vtkgl::RGB16F_ARB:
      result="GL_RGB16F_ARB";
      break;
    case vtkgl::ALPHA16F_ARB:
      result="GL_ALPHA16F_ARB";
      break;
    case vtkgl::INTENSITY16F_ARB:
      result="GL_INTENSITY16F_ARB";
      break;
    case vtkgl::LUMINANCE16F_ARB:
      result="GL_LUMINANCE16F_ARB";
      break;
    case vtkgl::LUMINANCE_ALPHA16F_ARB:
      result="GL_LUMINANCE_ALPHA16F_ARB";
      break;
      // Provided by GL_EXT_texture_integer (from GeForce 8)
    case vtkgl::RGBA32UI_EXT:
      result="GL_RGBA32UI_EXT";
      break;
    case vtkgl::RGB32UI_EXT:
      result="GL_RGB32UI_EXT";
      break;
    case vtkgl::ALPHA32UI_EXT:
      result="GL_ALPHA32UI_EXT";
      break;
    case vtkgl::INTENSITY32UI_EXT:
      result="GL_INTENSITY32UI_EXT";
      break;
    case vtkgl::LUMINANCE32UI_EXT:
      result="GL_LUMINANCE32UI_EXT";
      break;
    case vtkgl::LUMINANCE_ALPHA32UI_EXT:
      result="GL_LUMINANCE_ALPHA32UI_EXT";
      break;
    case vtkgl::RGBA16UI_EXT:
      result="GL_RGBA16UI_EXT";
      break;
    case vtkgl::RGB16UI_EXT:
      result="GL_RGB16UI_EXT";
      break;
    case vtkgl::ALPHA16UI_EXT:
      result="GL_ALPHA16UI_EXT";
      break;
    case vtkgl::INTENSITY16UI_EXT:
      result="GL_INTENSITY16UI_EXT";
      break;
    case vtkgl::LUMINANCE16UI_EXT:
      result="GL_LUMINANCE16UI_EXT";
      break;
    case vtkgl::LUMINANCE_ALPHA16UI_EXT :
      result="GL_LUMINANCE_ALPHA16UI_EXT ";
      break;
    case vtkgl::RGBA8UI_EXT:
      result="GL_RGBA8UI_EXT";
      break;
    case vtkgl::RGB8UI_EXT:
      result="GL_RGB8UI_EXT";
      break;
    case vtkgl::ALPHA8UI_EXT:
      result="GL_ALPHA8UI_EXT";
      break;
    case vtkgl::INTENSITY8UI_EXT:
      result="GL_INTENSITY8UI_EXT";
      break;
    case vtkgl::LUMINANCE8UI_EXT:
      result="GL_LUMINANCE8UI_EXT";
      break;
    case vtkgl::LUMINANCE_ALPHA8UI_EXT:
      result="GL_LUMINANCE_ALPHA8UI_EXT";
      break;
    case vtkgl::RGBA32I_EXT:
      result="GL_RGBA32I_EXT";
      break;
    case vtkgl::RGB32I_EXT:
      result="GL_RGB32I_EXT";
      break;
    case vtkgl::ALPHA32I_EXT:
      result="GL_ALPHA32I_EXT";
      break;
    case vtkgl::INTENSITY32I_EXT:
      result="GL_INTENSITY32I_EXT";
      break;
    case vtkgl::LUMINANCE32I_EXT:
      result="GL_LUMINANCE32I_EXT";
      break;
    case vtkgl::LUMINANCE_ALPHA32I_EXT:
      result="GL_LUMINANCE_ALPHA32I_EXT";
      break;
    case vtkgl::RGBA16I_EXT:
      result="GL_RGBA16I_EXT";
      break;
    case vtkgl::RGB16I_EXT:
      result="GL_RGB16I_EXT";
      break;
    case vtkgl::ALPHA16I_EXT:
      result="GL_ALPHA16I_EXT";
      break;
    case vtkgl::INTENSITY16I_EXT:
      result="GL_INTENSITY16I_EXT";
      break;
    case vtkgl::LUMINANCE16I_EXT:
      result="GL_LUMINANCE16I_EXT";
      break;
    case vtkgl::LUMINANCE_ALPHA16I_EXT:
      result="GL_LUMINANCE_ALPHA16I_EXT";
      break;
    case vtkgl::RGBA8I_EXT:
      result="GL_RGBA8I_EXT";
      break;
    case vtkgl::RGB8I_EXT:
      result="GL_RGB8I_EXT";
      break;
    case vtkgl::ALPHA8I_EXT:
      result="GL_ALPHA8I_EXT";
      break;
    case vtkgl::INTENSITY8I_EXT:
      result="GL_INTENSITY8I_EXT";
      break;
    case vtkgl::LUMINANCE8I_EXT:
      result="GL_LUMINANCE8I_EXT";
      break;
    case vtkgl::LUMINANCE_ALPHA8I_EXT:
      result="GL_LUMINANCE_ALPHA8I_EXT";
      break;
    default:
      result=0;
//      assert("check: impossible case." && 0);
//      cout<<"unknown"<<"(0x"<< hex << ivalue[0] << dec << ")";
      break;
    }
  return result;
}

const char *WrapModeToString(GLenum wrapMode)
{
  const char *result;
  switch(wrapMode)
    {
    case GL_CLAMP:
      result="GL_CLAMP";
      break;
    case GL_REPEAT:
      result="GL_REPEAT";
      break;
    case vtkgl::CLAMP_TO_EDGE:// OpenGL>=1.2 or Gl_SGIS_texture_edge_clamp
      result="vtkgl::CLAMP_TO_EDGE";
      break;
    case vtkgl::CLAMP_TO_BORDER:// OpenGL>=1.3 or GL_ARB_texture_border_clamp
      result="vtkgl::CLAMP_TO_BORDER";
      break;
    case vtkgl::MIRRORED_REPEAT:// OpenGL>=1.4 or GL_ARB_texture_mirrored_repeat
      result="vtkgl::MIRRORED_REPEAT";
      break;
    default:
      result="";
      assert("check: impossible case." && 0);
      break;
    }
  return result;
}

const char *TextureComponentTypeToString(GLint ivalue)
{
  const char *result;
  switch(ivalue)
    {
    case GL_NONE:
      // missing component
      result="missing";
      break;
    case vtkgl::UNSIGNED_NORMALIZED_ARB:
      // default type for OpenGL 1.1, fixed-point component
      result="";
      break;
    case GL_FLOAT:
      result="f"; // floating-point component, with GL_ARB_texture_float
      break;
    case GL_INT:
      result="i"; // signed unnormalized integer component, with GL_EXT_texture_integer (GeForce8)
      break;
    case GL_UNSIGNED_INT:
      result="ui"; // unsigned unnormalized integer component, with GL_EXT_texture_integer (GeForce8)
      break;
    default:
      result="error: unknown type";
      break;
    }
  return result;
}

void QueryTextureObject(GLenum target)
{
  assert("pre: valid_target" && (target==GL_TEXTURE_1D
                                 || target==GL_PROXY_TEXTURE_1D
                                 || target==GL_TEXTURE_2D
                                 || target==GL_PROXY_TEXTURE_2D
                                 || target==vtkgl::TEXTURE_CUBE_MAP_POSITIVE_X
                                 || target==vtkgl::TEXTURE_CUBE_MAP_NEGATIVE_X
                                 || target==vtkgl::TEXTURE_CUBE_MAP_POSITIVE_Y
                                 || target==vtkgl::TEXTURE_CUBE_MAP_NEGATIVE_Y
                                 || target==vtkgl::TEXTURE_CUBE_MAP_POSITIVE_Z
                                 || target==vtkgl::TEXTURE_CUBE_MAP_NEGATIVE_Z
                                 || target==vtkgl::PROXY_TEXTURE_CUBE_MAP
                                 || target==vtkgl::TEXTURE_3D
                                 || target==vtkgl::PROXY_TEXTURE_3D
                                 || target==vtkgl::TEXTURE_RECTANGLE_ARB
                                 || target==vtkgl::PROXY_TEXTURE_RECTANGLE_ARB ));

  GLint ivalue[4];
  GLfloat fvalue[4];

  glGetTexParameterfv(target,GL_TEXTURE_BORDER_COLOR,fvalue);
  CheckOpenGLError("");
  cout<<"border color="<<fvalue[0]<<" "<<fvalue[1]<<" "<<fvalue[2]<<" "
      <<fvalue[3]<<endl;
  glGetTexParameteriv(target,GL_TEXTURE_MIN_FILTER,ivalue);
  CheckOpenGLError("");
  cout<<" min filter=";
  cout<<MinMagModToString(ivalue[0]);
//  cout<<"unknown min filter."<<"(0x"<< hex << ivalue[0] << dec << ")";
  cout<<endl;
  glGetTexParameteriv(target,GL_TEXTURE_MAG_FILTER,ivalue);
  CheckOpenGLError("");
  cout<<" mag filter=";
  cout<<MinMagModToString(ivalue[0]);
  // cout<<"unknown mag filter."<<"(0x"<< hex << ivalue[0] << dec << ")";
  glGetTexParameteriv(target,GL_TEXTURE_WRAP_S,ivalue);
  CheckOpenGLError("");
  cout<<" wrap s="<<WrapModeToString(ivalue[0])<<endl;
  glGetTexParameteriv(target,GL_TEXTURE_WRAP_T,ivalue);
  CheckOpenGLError("");
  cout<<" wrap t="<<WrapModeToString(ivalue[0])<<endl;
  glGetTexParameteriv(target,vtkgl::TEXTURE_WRAP_R,ivalue);
  CheckOpenGLError("");
  cout<<" wrap r="<<WrapModeToString(ivalue[0])<<endl;
  glGetTexParameterfv(target,GL_TEXTURE_PRIORITY,fvalue);
  CheckOpenGLError("");
  cout<<" priority="<<fvalue[0]<<endl;
  glGetTexParameteriv(target,GL_TEXTURE_RESIDENT,ivalue);
  CheckOpenGLError("");
  cout<<" resident="<<BooleanToString(ivalue[0])<<endl;
  glGetTexParameterfv(target,vtkgl::TEXTURE_MIN_LOD,fvalue);
  CheckOpenGLError("");
  cout<<" min LOD="<<fvalue[0]<<endl;
  glGetTexParameterfv(target,vtkgl::TEXTURE_MAX_LOD,fvalue);
  CheckOpenGLError("");
  cout<<" max LOD="<<fvalue[0]<<endl;
  glGetTexParameterfv(target,vtkgl::TEXTURE_BASE_LEVEL,fvalue);
  CheckOpenGLError("");
  cout<<" base level="<<fvalue[0]<<endl;
  glGetTexParameterfv(target,vtkgl::TEXTURE_MAX_LEVEL,fvalue);
  CheckOpenGLError("");
  cout<<" max level="<<fvalue[0]<<endl;
  glGetTexParameterfv(target,vtkgl::TEXTURE_LOD_BIAS,fvalue);
  CheckOpenGLError("");
  cout<<" LOD bias="<<fvalue[0]<<endl;
  glGetTexParameteriv(target,vtkgl::DEPTH_TEXTURE_MODE,ivalue);
  CheckOpenGLError("");
  cout<<" depth texture mode=";
  switch(ivalue[0])
    {
    case GL_LUMINANCE:
      cout<<"GL_LUMINANCE";
      break;
    case GL_INTENSITY:
      cout<<"GL_INTENSITY";
      break;
    case GL_ALPHA:
      cout<<"GL_ALPHA";
      break;
    default:
      cout<<"unknown depth texture mode."<<"(0x"<< hex << ivalue[0] << dec << ")";
      break;
    }
  glGetTexParameteriv(target,vtkgl::TEXTURE_COMPARE_MODE,ivalue);
  CheckOpenGLError("");
  cout<<" compare mode=";
  switch(ivalue[0])
    {
    case GL_NONE:
      cout<<"GL_NONE";
      break;
    case vtkgl::COMPARE_R_TO_TEXTURE:
      cout<<"GL_COMPARE_R_TO_TEXTURE";
      break;
    default:
      cout<<"unknown."<<"(0x"<< hex << ivalue[0] << dec << ")";
      break;
    }
  glGetTexParameteriv(target,vtkgl::TEXTURE_COMPARE_FUNC,ivalue);
  CheckOpenGLError("");
  cout<<" compare function=";
  switch(ivalue[0])
    {
    case GL_LEQUAL:
      cout<<"GL_LEQUAL";
      break;
    case GL_GEQUAL:
      cout<<"GL_GEQUAL";
      break;
    case GL_LESS:
      cout<<"GL_LESS";
      break;
    case GL_GREATER:
      cout<<"GL_GREATER";
      break;
    case GL_EQUAL:
      cout<<"GL_EQUAL";
      break;
    case GL_NOTEQUAL:
      cout<<"GL_NOTEQUAL";
      break;
    case GL_ALWAYS:
      cout<<"GL_ALWAYS";
      break;
    case GL_NEVER:
      cout<<"GL_NEVER";
      break;
    default:
      cout<<"unknown"<<"(0x"<< hex << ivalue[0] << dec << ")";
      break;
    }
  glGetTexParameteriv(target,vtkgl::GENERATE_MIPMAP,ivalue);
  CheckOpenGLError("");
  cout<<" generate mipmap="<<BooleanToString(ivalue[0])<<endl;
}

void QueryTextureImage(GLenum target)
{
  assert("pre: valid_target" && (target==GL_TEXTURE_1D
                                 || target==GL_PROXY_TEXTURE_1D
                                 || target==GL_TEXTURE_2D
                                 || target==GL_PROXY_TEXTURE_2D
                                 || target==vtkgl::TEXTURE_CUBE_MAP_POSITIVE_X
                                 || target==vtkgl::TEXTURE_CUBE_MAP_NEGATIVE_X
                                 || target==vtkgl::TEXTURE_CUBE_MAP_POSITIVE_Y
                                 || target==vtkgl::TEXTURE_CUBE_MAP_NEGATIVE_Y
                                 || target==vtkgl::TEXTURE_CUBE_MAP_POSITIVE_Z
                                 || target==vtkgl::TEXTURE_CUBE_MAP_NEGATIVE_Z
                                 || target==vtkgl::PROXY_TEXTURE_CUBE_MAP
                                 || target==vtkgl::TEXTURE_3D
                                 || target==vtkgl::PROXY_TEXTURE_3D
                                 || target==vtkgl::TEXTURE_RECTANGLE_ARB
                                 || target==vtkgl::PROXY_TEXTURE_RECTANGLE_ARB ));

  GLint ivalue[4];
  GLint iv;

  glGetTexLevelParameteriv(target,0,GL_TEXTURE_WIDTH,ivalue);
  CheckOpenGLError("");
  cout<<" width="<<ivalue[0]<<endl;
  glGetTexLevelParameteriv(target,0,GL_TEXTURE_HEIGHT,ivalue);
  CheckOpenGLError("");
  cout<<" height="<<ivalue[0]<<endl;
  glGetTexLevelParameteriv(target,0,vtkgl::TEXTURE_DEPTH,ivalue);
  CheckOpenGLError("");
  cout<<" depth="<<ivalue[0]<<endl;
  glGetTexLevelParameteriv(target,0,GL_TEXTURE_BORDER,ivalue);
  CheckOpenGLError("");
  cout<<" border="<<ivalue[0]<<endl;
  glGetTexLevelParameteriv(target,0,GL_TEXTURE_INTERNAL_FORMAT,
                           ivalue);
  CheckOpenGLError("");
  cout<<" internal format=";
  const char *f=InternalTextureFormatToString(ivalue[0]);
  if(f==0)
    {
    cout<<"unknown"<<"(0x"<< hex << ivalue[0] << dec << ")";
    }
  else
    {
    cout<<f;
    }
  cout<<endl;
  cout<<" real internal format=";

  glGetTexLevelParameteriv(target,0,GL_TEXTURE_RED_SIZE,ivalue);
  CheckOpenGLError("");
  if(ivalue[0]>0)
    {
    cout<<"R";
    iv=ivalue[0];
    if(float_texture_supported)
      {
      glGetTexLevelParameteriv(target,0,vtkgl::TEXTURE_RED_TYPE_ARB,ivalue);
      cout<<TextureComponentTypeToString(ivalue[0]);
      }
    cout<<iv;
    }
  glGetTexLevelParameteriv(target,0,GL_TEXTURE_GREEN_SIZE,ivalue);
  CheckOpenGLError("");
  if(ivalue[0]>0)
    {
    cout<<"G";
    iv=ivalue[0];
    if(float_texture_supported)
      {
      glGetTexLevelParameteriv(target,0,vtkgl::TEXTURE_GREEN_TYPE_ARB,ivalue);
      cout<<TextureComponentTypeToString(ivalue[0]);
      }
    cout<<iv;
    }
  glGetTexLevelParameteriv(target,0,GL_TEXTURE_BLUE_SIZE,ivalue);
  CheckOpenGLError("");
  if(ivalue[0]>0)
    {
    cout<<"B";
    iv=ivalue[0];
    if(float_texture_supported)
      {
      glGetTexLevelParameteriv(target,0,vtkgl::TEXTURE_BLUE_TYPE_ARB,ivalue);
      cout<<TextureComponentTypeToString(ivalue[0]);
      }
    cout<<iv;
    }
  glGetTexLevelParameteriv(target,0,GL_TEXTURE_LUMINANCE_SIZE,ivalue);
  CheckOpenGLError("");
  if(ivalue[0]>0)
    {
    cout<<"L";
    iv=ivalue[0];
    if(float_texture_supported)
      {
      glGetTexLevelParameteriv(target,0,vtkgl::TEXTURE_LUMINANCE_TYPE_ARB,ivalue);
      cout<<TextureComponentTypeToString(ivalue[0]);
      }
    cout<<iv;
    }
  glGetTexLevelParameteriv(target,0,GL_TEXTURE_ALPHA_SIZE,ivalue);
  CheckOpenGLError("");
  if(ivalue[0]>0)
    {
    cout<<"A";
    iv=ivalue[0];
    if(float_texture_supported)
      {
      glGetTexLevelParameteriv(target,0,vtkgl::TEXTURE_ALPHA_TYPE_ARB,ivalue);
      cout<<TextureComponentTypeToString(ivalue[0]);
      }
    cout<<iv;
    }
  glGetTexLevelParameteriv(target,0,GL_TEXTURE_INTENSITY_SIZE,ivalue);
  CheckOpenGLError("");
  if(ivalue[0]>0)
    {
    cout<<"I";
    iv=ivalue[0];
    if(float_texture_supported)
      {
      glGetTexLevelParameteriv(target,0,vtkgl::TEXTURE_INTENSITY_TYPE_ARB,ivalue);
      cout<<TextureComponentTypeToString(ivalue[0]);
      }
    cout<<iv;
    }
  glGetTexLevelParameteriv(target,0,vtkgl::TEXTURE_DEPTH_SIZE,ivalue);
  CheckOpenGLError("");
  if(ivalue[0]>0)
    {
    cout<<"D";
    iv=ivalue[0];
    if(float_texture_supported)
      {
      glGetTexLevelParameteriv(target,0,vtkgl::TEXTURE_DEPTH_TYPE_ARB,ivalue);
      cout<<TextureComponentTypeToString(ivalue[0]);
      }
    cout<<iv;
    }

  cout<<endl;
  glGetTexLevelParameteriv(target,0,vtkgl::TEXTURE_COMPRESSED,ivalue);
  CheckOpenGLError("");
  cout<<" compressed="<<BooleanToString(ivalue[0])<<endl;
  if(ivalue[0])
    {
    glGetTexLevelParameteriv(target,0,vtkgl::TEXTURE_COMPRESSED_IMAGE_SIZE,
                             ivalue);
    CheckOpenGLError("");
    cout<<" compressed image size="<<ivalue[0]<<" ubytes."<<endl;
    }
}

void QueryTexture1D()
{
  // void GetBooleanv(value,ivalue)  GLboolean

  GLboolean bvalue[4];
  GLint ivalue[4];

  // State per texture unit and binding point
  bvalue[0]=glIsEnabled(GL_TEXTURE_1D);
  cout<<"2D texturing is enabled:"<< BooleanToString(bvalue[0]) << endl;
  glGetIntegerv(GL_TEXTURE_BINDING_1D,ivalue);
  cout<<"texture object "<<ivalue[0]<<" is bind to texture 1d."<<endl;

  // State per texture object
  QueryTextureObject(GL_TEXTURE_1D);

  // State per texture image
  QueryTextureImage(GL_TEXTURE_1D);

  // Texture environment and generation

}

void QueryTexture2D()
{
  // void GetBooleanv(value,ivalue)  GLboolean

  GLboolean bvalue[4];
  GLint ivalue[4];

  // State per texture unit and binding point
  bvalue[0]=glIsEnabled(GL_TEXTURE_2D);
  CheckOpenGLError("");
  cout<<"2D texturing is enabled:"<< BooleanToString(bvalue[0]) << endl;
  glGetIntegerv(GL_TEXTURE_BINDING_2D,ivalue);
  CheckOpenGLError("");
  cout<<"texture object "<<ivalue[0]<<" is bind to texture 2d."<<endl;

  // State per texture object
  QueryTextureObject(GL_TEXTURE_2D);
  CheckOpenGLError("");
  // State per texture image
  QueryTextureImage(GL_TEXTURE_2D);
  CheckOpenGLError("");
  // Texture environment and generation

}

void QueryTexture2DRectangle()
{
  // void GetBooleanv(value,ivalue)  GLboolean

  GLboolean bvalue[4];
  GLint ivalue[4];

  // State per texture unit and binding point
  bvalue[0]=glIsEnabled(vtkgl::TEXTURE_RECTANGLE_ARB);
  cout<<"2D rect texturing is enabled:"<< BooleanToString(bvalue[0]) << endl;
  glGetIntegerv(vtkgl::TEXTURE_BINDING_RECTANGLE_ARB,ivalue);
  cout<<"texture object "<<ivalue[0]<<" is bind to texture 2d rect."<<endl;

  // State per texture object
  QueryTextureObject(vtkgl::TEXTURE_RECTANGLE_ARB);

  // State per texture image
  QueryTextureImage(vtkgl::TEXTURE_RECTANGLE_ARB);

  // Texture environment and generation

}
void QueryTexture3D()
{
  // void GetBooleanv(value,ivalue)  GLboolean

  GLboolean bvalue[4];
  GLint ivalue[4];

  // State per texture unit and binding point
  bvalue[0]=glIsEnabled(vtkgl::TEXTURE_3D);
  cout<<"3D texturing is enabled:"<< BooleanToString(bvalue[0]) << endl;
  glGetIntegerv(vtkgl::TEXTURE_BINDING_3D,ivalue);
  cout<<"texture object "<<ivalue[0]<<" is bind to texture 3d."<<endl;

  // State per texture object
  QueryTextureObject(vtkgl::TEXTURE_3D);

  // State per texture image
  QueryTextureImage(vtkgl::TEXTURE_3D);

  // Texture environment and generation

}

int textureSizes[2][2]={{64,32}, // spec says min of max is 64.
                        {63,32}};

int textureFormat[13]={GL_COLOR_INDEX,
                       GL_STENCIL_INDEX,
                       GL_DEPTH_COMPONENT,
                       GL_RED,
                       GL_GREEN,
                       GL_BLUE,
                       GL_ALPHA,
                       GL_RGB,
                       GL_RGBA,
                       vtkgl::BGR,
                       vtkgl::BGRA,
                       GL_LUMINANCE,
                       GL_LUMINANCE_ALPHA};

int textureBaseInternalFormats[7]={GL_ALPHA,
                                   GL_DEPTH_COMPONENT,
                                   GL_LUMINANCE,
                                   GL_LUMINANCE_ALPHA,
                                   GL_INTENSITY,
                                   GL_RGB,
                                   GL_RGBA};

int textureSizedInternalFormats[87]={GL_ALPHA4,
                                     GL_ALPHA8,
                                     GL_ALPHA12,
                                     GL_ALPHA16,
                                     vtkgl::DEPTH_COMPONENT16, //4
                                     vtkgl::DEPTH_COMPONENT24, //5
                                     vtkgl::DEPTH_COMPONENT32, //6
                                     GL_LUMINANCE4,
                                     GL_LUMINANCE8,
                                     GL_LUMINANCE12,
                                     GL_LUMINANCE16,
                                     GL_LUMINANCE4_ALPHA4,
                                     GL_LUMINANCE6_ALPHA2,
                                     GL_LUMINANCE8_ALPHA8,
                                     GL_LUMINANCE12_ALPHA4,
                                     GL_LUMINANCE12_ALPHA12,
                                     GL_LUMINANCE16_ALPHA16,
                                     GL_INTENSITY4,
                                     GL_INTENSITY8,
                                     GL_INTENSITY12,
                                     GL_INTENSITY16,
                                     GL_R3_G3_B2,
                                     GL_RGB4,
                                     GL_RGB5,
                                     GL_RGB8,
                                     GL_RGB10,
                                     GL_RGB12,
                                     GL_RGB16,
                                     GL_RGBA2,
                                     GL_RGBA4,
                                     GL_RGB5_A1,
                                     GL_RGBA8,
                                     GL_RGB10_A2,
                                     GL_RGBA12,
                                     GL_RGBA16,
                                     vtkgl::SRGB8, //35
                                     vtkgl::SRGB8_ALPHA8,
                                     vtkgl::SLUMINANCE8,
                                     vtkgl::SLUMINANCE8_ALPHA8, // idx=38,count=39
                                     vtkgl::RGBA32F_ARB,
                                     vtkgl::RGB32F_ARB,
                                     vtkgl::ALPHA32F_ARB,
                                     vtkgl::INTENSITY32F_ARB,
                                     vtkgl::LUMINANCE32F_ARB,
                                     vtkgl::LUMINANCE_ALPHA32F_ARB,
                                     vtkgl::RGBA16F_ARB,
                                     vtkgl::RGB16F_ARB,
                                     vtkgl::ALPHA16F_ARB,
                                     vtkgl::INTENSITY16F_ARB,
                                     vtkgl::LUMINANCE16F_ARB,
                                     vtkgl::LUMINANCE_ALPHA16F_ARB,// i=50,c=51
                                     vtkgl::ALPHA8I_EXT,
                                     vtkgl::ALPHA8UI_EXT,
                                     vtkgl::ALPHA16I_EXT,
                                     vtkgl::ALPHA16UI_EXT,
                                     vtkgl::ALPHA32I_EXT,
                                     vtkgl::ALPHA32UI_EXT,
                                     vtkgl::LUMINANCE8I_EXT,
                                     vtkgl::LUMINANCE8UI_EXT,
                                     vtkgl::LUMINANCE16I_EXT,
                                     vtkgl::LUMINANCE16UI_EXT,
                                     vtkgl::LUMINANCE32I_EXT,
                                     vtkgl::LUMINANCE32UI_EXT,
                                     vtkgl::LUMINANCE_ALPHA8I_EXT,
                                     vtkgl::LUMINANCE_ALPHA8UI_EXT,
                                     vtkgl::LUMINANCE_ALPHA16I_EXT,
                                     vtkgl::LUMINANCE_ALPHA16UI_EXT,
                                     vtkgl::LUMINANCE_ALPHA32I_EXT,
                                     vtkgl::LUMINANCE_ALPHA32UI_EXT,
                                     vtkgl::INTENSITY8I_EXT,
                                     vtkgl::INTENSITY8UI_EXT,
                                     vtkgl::INTENSITY16I_EXT,
                                     vtkgl::INTENSITY16UI_EXT,
                                     vtkgl::INTENSITY32I_EXT,
                                     vtkgl::INTENSITY32UI_EXT,
                                     vtkgl::RGB8I_EXT,
                                     vtkgl::RGB8UI_EXT,
                                     vtkgl::RGB16I_EXT,
                                     vtkgl::RGB16UI_EXT,
                                     vtkgl::RGB32I_EXT,
                                     vtkgl::RGB32UI_EXT,
                                     vtkgl::RGBA8I_EXT,
                                     vtkgl::RGBA8UI_EXT,
                                     vtkgl::RGBA16I_EXT,
                                     vtkgl::RGBA16UI_EXT,
                                     vtkgl::RGBA32I_EXT,
                                     vtkgl::RGBA32UI_EXT}; // i=86, c=87

const int NumberOftextureSizedInternalFormats=87;

int textureFormats[23]={GL_COLOR_INDEX,
                        GL_STENCIL_INDEX,
                        GL_DEPTH_COMPONENT,
                        GL_RED,
                        GL_GREEN,
                        GL_BLUE,
                        GL_ALPHA,
                        GL_RGB,
                        GL_RGBA,
                        vtkgl::BGR,
                        vtkgl::BGRA,
                        GL_LUMINANCE,
                        GL_LUMINANCE_ALPHA,
                        vtkgl::RED_INTEGER_EXT,
                        vtkgl::GREEN_INTEGER_EXT,
                        vtkgl::BLUE_INTEGER_EXT,
                        vtkgl::ALPHA_INTEGER_EXT,
                        vtkgl::RGB_INTEGER_EXT,
                        vtkgl::RGBA_INTEGER_EXT,
                        vtkgl::BGR_INTEGER_EXT,
                        vtkgl::BGRA_INTEGER_EXT,
                        vtkgl::LUMINANCE_INTEGER_EXT,
                        vtkgl::LUMINANCE_ALPHA_INTEGER_EXT};

#if 0
int textureFormat[7]={GL_ALPHA,
                      GL_DEPTH_COMPONENT,
                      GL_LUMINANCE,
                      GL_LUMINANCE_ALPHA,
                      GL_INTENSITY,
                      GL_RGB,
                      GL_RGBA //,
};
#endif

int textureType[]={GL_UNSIGNED_BYTE,
                   GL_BITMAP,
                   GL_BYTE,
                   GL_UNSIGNED_SHORT,
                   GL_SHORT,
                   GL_UNSIGNED_INT,
                   GL_INT,
                   GL_FLOAT,
                   vtkgl::UNSIGNED_BYTE_3_3_2,
                   vtkgl::UNSIGNED_BYTE_2_3_3_REV,
                   vtkgl::UNSIGNED_SHORT_5_6_5,
                   vtkgl::UNSIGNED_SHORT_5_6_5_REV,
                   vtkgl::UNSIGNED_SHORT_4_4_4_4,
                   vtkgl::UNSIGNED_SHORT_4_4_4_4_REV,
                   vtkgl::UNSIGNED_SHORT_5_5_5_1,
                   vtkgl::UNSIGNED_SHORT_1_5_5_5_REV,
                   vtkgl::UNSIGNED_INT_8_8_8_8,
                   vtkgl::UNSIGNED_INT_8_8_8_8_REV,
                   vtkgl::UNSIGNED_INT_10_10_10_2,
                   vtkgl::UNSIGNED_INT_2_10_10_10_REV};

GLenum textureTarget[2]={GL_TEXTURE_2D,
                         vtkgl::TEXTURE_RECTANGLE_ARB};

int textureProxyTarget[2]={GL_PROXY_TEXTURE_2D,
                           vtkgl::PROXY_TEXTURE_RECTANGLE_ARB};

int textureMinMag[2]={GL_NEAREST,GL_LINEAR};

// OpenGL 1.2: vtkgl::CLAMP_TO_EDGE
// OpenGL 1.3: vtkgl::CLAMP_TO_BORDER
// OpenGL 1.4: vtkgl::MIRRORED_REPEAT

int textureWrap[5]={GL_CLAMP,
                    GL_REPEAT,
                    vtkgl::CLAMP_TO_EDGE,
                    vtkgl::CLAMP_TO_BORDER,
                    vtkgl::MIRRORED_REPEAT};

// GL_ARB_color_buffer_float
// GL_ARB_half_float_pixel
// GL_ARB_texture_float <=====
// GL_ATI_pixel_format_float
// GL_ATI_texture_float
// GL_NV_float_buffer
// GL_NV_half_float
// GL_EXT_packed_float
// GL_NV_depth_buffer_float


int FromTextureSizedInternalFormatsToBaseInternalFormat(int f)
{
  int result;

  switch(f)
    {
    case GL_ALPHA4:
    case GL_ALPHA8:
    case GL_ALPHA12:
    case GL_ALPHA16:
    case vtkgl::ALPHA32F_ARB:
    case vtkgl::ALPHA16F_ARB:
    case vtkgl::ALPHA8I_EXT:
    case vtkgl::ALPHA8UI_EXT:
    case vtkgl::ALPHA16I_EXT:
    case vtkgl::ALPHA16UI_EXT:
    case vtkgl::ALPHA32I_EXT:
    case vtkgl::ALPHA32UI_EXT:
      result=GL_ALPHA;
      break;
    case vtkgl::DEPTH_COMPONENT16:
    case vtkgl::DEPTH_COMPONENT24:
    case vtkgl::DEPTH_COMPONENT32:
      result=GL_DEPTH_COMPONENT;
      break;
    case GL_LUMINANCE4:
    case GL_LUMINANCE8:
    case GL_LUMINANCE12:
    case GL_LUMINANCE16:
    case vtkgl::SLUMINANCE8:
    case vtkgl::LUMINANCE32F_ARB:
    case vtkgl::LUMINANCE16F_ARB:
    case vtkgl::LUMINANCE8I_EXT:
    case vtkgl::LUMINANCE8UI_EXT:
    case vtkgl::LUMINANCE16I_EXT:
    case vtkgl::LUMINANCE16UI_EXT:
    case vtkgl::LUMINANCE32I_EXT:
    case vtkgl::LUMINANCE32UI_EXT:
      result=GL_LUMINANCE;
      break;
    case GL_LUMINANCE4_ALPHA4:
    case GL_LUMINANCE6_ALPHA2:
    case GL_LUMINANCE8_ALPHA8:
    case GL_LUMINANCE12_ALPHA4:
    case GL_LUMINANCE12_ALPHA12:
    case GL_LUMINANCE16_ALPHA16:
    case vtkgl::SLUMINANCE8_ALPHA8:
    case vtkgl::LUMINANCE_ALPHA32F_ARB:
    case vtkgl::LUMINANCE_ALPHA16F_ARB:
    case vtkgl::LUMINANCE_ALPHA8I_EXT:
    case vtkgl::LUMINANCE_ALPHA8UI_EXT:
    case vtkgl::LUMINANCE_ALPHA16I_EXT:
    case vtkgl::LUMINANCE_ALPHA16UI_EXT:
    case vtkgl::LUMINANCE_ALPHA32I_EXT:
    case vtkgl::LUMINANCE_ALPHA32UI_EXT:
      result=GL_LUMINANCE_ALPHA;
      break;
    case GL_INTENSITY4:
    case GL_INTENSITY8:
    case GL_INTENSITY12:
    case GL_INTENSITY16:
    case vtkgl::INTENSITY32F_ARB:
    case vtkgl::INTENSITY16F_ARB:
    case vtkgl::INTENSITY8I_EXT:
    case vtkgl::INTENSITY8UI_EXT:
    case vtkgl::INTENSITY16I_EXT:
    case vtkgl::INTENSITY16UI_EXT:
    case vtkgl::INTENSITY32I_EXT:
    case vtkgl::INTENSITY32UI_EXT:
      result=GL_INTENSITY;
      break;
    case GL_R3_G3_B2:
    case GL_RGB4:
    case GL_RGB5:
    case GL_RGB8:
    case GL_RGB10:
    case GL_RGB12:
    case GL_RGB16:
    case vtkgl::SRGB8:
    case vtkgl::RGB32F_ARB:
    case vtkgl::RGB16F_ARB:
    case vtkgl::RGB8I_EXT:
    case vtkgl::RGB8UI_EXT:
    case vtkgl::RGB16I_EXT:
    case vtkgl::RGB16UI_EXT:
    case vtkgl::RGB32I_EXT:
    case vtkgl::RGB32UI_EXT:
      result=GL_RGB;
      break;
    case GL_RGBA2:
    case GL_RGBA4:
    case GL_RGB5_A1:
    case GL_RGBA8:
    case GL_RGB10_A2:
    case GL_RGBA12:
    case GL_RGBA16:
    case vtkgl::SRGB8_ALPHA8:
    case vtkgl::RGBA32F_ARB:
    case vtkgl::RGBA16F_ARB:
    case vtkgl::RGBA8I_EXT:
    case vtkgl::RGBA8UI_EXT:
    case vtkgl::RGBA16I_EXT:
    case vtkgl::RGBA16UI_EXT:
    case vtkgl::RGBA32I_EXT:
    case vtkgl::RGBA32UI_EXT:
      result=GL_RGBA;
      break;
    default:
      result=0;
      assert("check: impossible case." && 0);
      break;
    }
  return result;
}

bool TextureSizedInternalFormatIsInteger(int f)
{
  bool result;

  switch(f)
    {
    case vtkgl::ALPHA8I_EXT:
    case vtkgl::ALPHA8UI_EXT:
    case vtkgl::ALPHA16I_EXT:
    case vtkgl::ALPHA16UI_EXT:
    case vtkgl::ALPHA32I_EXT:
    case vtkgl::ALPHA32UI_EXT:
    case vtkgl::LUMINANCE8I_EXT:
    case vtkgl::LUMINANCE8UI_EXT:
    case vtkgl::LUMINANCE16I_EXT:
    case vtkgl::LUMINANCE16UI_EXT:
    case vtkgl::LUMINANCE32I_EXT:
    case vtkgl::LUMINANCE32UI_EXT:
    case vtkgl::LUMINANCE_ALPHA8I_EXT:
    case vtkgl::LUMINANCE_ALPHA8UI_EXT:
    case vtkgl::LUMINANCE_ALPHA16I_EXT:
    case vtkgl::LUMINANCE_ALPHA16UI_EXT:
    case vtkgl::LUMINANCE_ALPHA32I_EXT:
    case vtkgl::LUMINANCE_ALPHA32UI_EXT:
    case vtkgl::INTENSITY8I_EXT:
    case vtkgl::INTENSITY8UI_EXT:
    case vtkgl::INTENSITY16I_EXT:
    case vtkgl::INTENSITY16UI_EXT:
    case vtkgl::INTENSITY32I_EXT:
    case vtkgl::INTENSITY32UI_EXT:
    case vtkgl::RGB8I_EXT:
    case vtkgl::RGB8UI_EXT:
    case vtkgl::RGB16I_EXT:
    case vtkgl::RGB16UI_EXT:
    case vtkgl::RGB32I_EXT:
    case vtkgl::RGB32UI_EXT:
    case vtkgl::RGBA8I_EXT:
    case vtkgl::RGBA8UI_EXT:
    case vtkgl::RGBA16I_EXT:
    case vtkgl::RGBA16UI_EXT:
    case vtkgl::RGBA32I_EXT:
    case vtkgl::RGBA32UI_EXT:
      result=true;
      break;
    default:
      result=false;
    }

  return result;
}

int FromBaseInternalFormatToFormat(int f,
                                   bool isInteger)
{
  int result=f;
  if(f==GL_INTENSITY)
    {
    result=GL_RED;
    }
  if(isInteger)
    {
    switch(result)
      {
      case GL_RED:
        result=vtkgl::RED_INTEGER_EXT;
        break;
      case GL_ALPHA:
        result=vtkgl::ALPHA_INTEGER_EXT;
        break;
      case GL_RGB:
        result=vtkgl::RGB_INTEGER_EXT;
        break;
      case GL_RGBA:
        result=vtkgl::RGBA_INTEGER_EXT;
        break;
      case GL_LUMINANCE:
        result=vtkgl::LUMINANCE_INTEGER_EXT;
        break;
      case GL_LUMINANCE_ALPHA:
        result=vtkgl::LUMINANCE_ALPHA_INTEGER_EXT;
        break;
      default:
        assert("check: impossible case." && 0);
        break;
      }
    }
  return result;
}

const char *TargetToString(int target)
{
  const char *result;
  switch(target)
    {
    case GL_TEXTURE_2D:
      result="GL_TEXTURE_2D";
      break;
    case vtkgl::TEXTURE_RECTANGLE_ARB:
      result="vtkgl::TEXTURE_RECTANGLE_ARB";
      break;
    case vtkgl::TEXTURE_3D:
      result="vtkgl::TEXTURE_3D";
      break;
    default:
      result="";
      assert("check: impossible case." && 0);
      break;
    }
  return result;
}

const char *WrapModeToString(int wrapMode)
{
  const char *result;
  switch(wrapMode)
    {
    case GL_CLAMP:
      result="GL_CLAMP";
      break;
    case GL_REPEAT:
      result="GL_REPEAT";
      break;
    case vtkgl::CLAMP_TO_EDGE:
      result="vtkgl::CLAMP_TO_EDGE";
      break;
    case vtkgl::CLAMP_TO_BORDER:
      result="vtkgl::CLAMP_TO_BORDER";
      break;
    case vtkgl::MIRRORED_REPEAT:
      result="vtkgl::MIRRORED_REPEAT";
      break;
    default:
      result="";
      assert("check: impossible case." && 0);
      break;
    }
  return result;
}

void TestTextureFormatsAndFBO()
{
  // target TEXTURE_2D ARB_TEXTURE_RECTANGLE
  // 1 component luminance, depth, intensity
  // 2 components luminance+alpha
  // 3 components RGB
  // 4 components RGBA
  // wrapping mode: edge edge_clamp
  // linear/nearest.
  // size POT or NPOT

  bool supportedtextureSizedInternalFormats[NumberOftextureSizedInternalFormats];

  int i=0;
  while(i<NumberOftextureSizedInternalFormats)
    {
    supportedtextureSizedInternalFormats[i]=true;
    ++i;
    }
  if(!depth_texture_supported)
    {
    i=4;
    while(i<=6)
      {
      supportedtextureSizedInternalFormats[i]=false;
      ++i;
      }
    }
  if(!srgb_texture_supported)
    {
    i=35;
    while(i<=38)
      {
      supportedtextureSizedInternalFormats[i]=false;
      ++i;
      }
    }
   if(!float_texture_supported)
    {
    i=39;
    while(i<=50)
      {
      supportedtextureSizedInternalFormats[i]=false;
      ++i;
      }
    }
   if(!integer_texture_supported)
    {
    i=51;
    while(i<=86)
      {
      supportedtextureSizedInternalFormats[i]=false;
      ++i;
      }
    }

  GLuint textureObject;

  int numberOfTarget=1;
  if(ARB_texture_rectangle_supported)
    {
    numberOfTarget=2;
    }
  int targetIdx=0;
  while(targetIdx<numberOfTarget)
    {
    GLenum target=textureTarget[targetIdx];
    int wrapIdx=0;
    while(wrapIdx<5)
      {
      int magIdx=0;
      while(magIdx<2)
        {
        int textureSizeIdx=0;
        while(textureSizeIdx<2)
          {

          int internalFormatIndex=0;
          while(internalFormatIndex<NumberOftextureSizedInternalFormats)
            {
            if(supportedtextureSizedInternalFormats[internalFormatIndex])
              {
            cout<<"----------------------------------------------------"<<endl;
            cout<<"Test "<<TargetToString(target)<<" "<<WrapModeToString(textureWrap[wrapIdx])<<" "<<MinMagModToString(textureMinMag[magIdx]);

            if(textureSizeIdx==0)
              {
              cout<<" POT";
              }
            else
              {
              cout<<" NPOT";
              }
            int internalFormat=textureSizedInternalFormats[internalFormatIndex];
            cout<<" "<<InternalTextureFormatToString(internalFormat)<<endl;


            glGenTextures(1,&textureObject);
            CheckOpenGLError("after glGenTextures");
            glBindTexture(target,textureObject);
            CheckOpenGLError("after glBindTexture");
            glTexParameteri(target, GL_TEXTURE_WRAP_S, textureWrap[wrapIdx]);

            GLenum errorCode=glGetError();
            if((textureWrap[wrapIdx]==GL_REPEAT || textureWrap[wrapIdx]==static_cast<GLint>(vtkgl::MIRRORED_REPEAT)) && errorCode==GL_INVALID_ENUM && target==vtkgl::TEXTURE_RECTANGLE_ARB)
              {
              // expected error. see extension spec.
              }
            else
              {
              if(errorCode!=GL_NO_ERROR)
                {
                cout << "ERROR:"
                     << OpenGLErrorMessage2(errorCode) << "after GL_TEXTURE_WRAP_S" <<endl;
                }
              }
            glTexParameteri(target, GL_TEXTURE_WRAP_T, textureWrap[wrapIdx]);

            errorCode=glGetError();
            if((textureWrap[wrapIdx]==GL_REPEAT || textureWrap[wrapIdx]==static_cast<GLint>(vtkgl::MIRRORED_REPEAT)) && errorCode==GL_INVALID_ENUM && target==vtkgl::TEXTURE_RECTANGLE_ARB)
              {
              // expected error. see extension spec.
              }
            else
              {
              if(errorCode!=GL_NO_ERROR)
                {
                cout << "ERROR:"
                     << OpenGLErrorMessage2(errorCode) << "after GL_TEXTURE_WRAP_T" <<endl;
                }
              }

            glTexParameteri(target, GL_TEXTURE_MIN_FILTER, textureMinMag[magIdx]);
            CheckOpenGLError("after GL_TEXTURE_MIN_FILTER");
            glTexParameteri(target, GL_TEXTURE_MAG_FILTER, textureMinMag[magIdx]);
            CheckOpenGLError("after GL_TEXTURE_MAG_FILTER");


            int format= FromBaseInternalFormatToFormat(
              FromTextureSizedInternalFormatsToBaseInternalFormat(internalFormat),
              TextureSizedInternalFormatIsInteger(internalFormat));
            int type=GL_UNSIGNED_BYTE;

            glTexImage2D(textureProxyTarget[targetIdx],0,internalFormat,
                         textureSizes[textureSizeIdx][0],
                         textureSizes[textureSizeIdx][1],
                         0, format, type, NULL );
            CheckOpenGLError("after glTexImage2D on proxy");

            GLint width;
            glGetTexLevelParameteriv(textureProxyTarget[targetIdx],0,
                                     GL_TEXTURE_WIDTH,&width);

            CheckOpenGLError("after getting proxy result");
            if(width!=0)
              {
              glTexImage2D(target,0,internalFormat,
                           textureSizes[textureSizeIdx][0],
                           textureSizes[textureSizeIdx][1],
                           0, format, type, NULL );
              CheckOpenGLError("after  glTexImage2D on real target");
              if(target==GL_TEXTURE_2D)
                {
                QueryTexture2D();
                }
              else
                {
                // vtkgl::TEXTURE_RECTANGLE_ARB
                QueryTexture2DRectangle();
                }
              CheckOpenGLError("after querying the current texture");

              if(vtkgl::GenFramebuffersEXT!=0) // FBO supported
                {
                // Try an FBO with just one color attachment:
                GLint savedFrameBuffer;
                glGetIntegerv(vtkgl::FRAMEBUFFER_BINDING_EXT,&savedFrameBuffer);
                GLuint fbo;
                vtkgl::GenFramebuffersEXT(1,&fbo);
                CheckOpenGLError("");
                vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,fbo);
                CheckOpenGLError("");
                vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                               vtkgl::COLOR_ATTACHMENT0_EXT,
                                               target, textureObject, 0);
                CheckOpenGLError("");
                CheckFrameBufferStatus();

                // Detach the color buffer
                vtkgl::FramebufferTexture2DEXT(vtkgl::FRAMEBUFFER_EXT,
                                               vtkgl::COLOR_ATTACHMENT0_EXT,
                                               target, 0, 0);
                CheckOpenGLError("");
                // Restore default frame buffer.
                vtkgl::BindFramebufferEXT(vtkgl::FRAMEBUFFER_EXT,
                                          savedFrameBuffer);
                CheckOpenGLError("");
                vtkgl::DeleteFramebuffersEXT(1,&fbo);
                CheckOpenGLError("");
                }
              }
            else
              {
              cout<<"Texture format not supported."<<endl;
              }
            glDeleteTextures(1,&textureObject);
            CheckOpenGLError("");
              }
            ++internalFormatIndex;
            }
          ++textureSizeIdx;
          }
        ++magIdx;
        }
      ++wrapIdx;
      }
    ++targetIdx;
    }
}

void TestVisual(int multiSample,
                int alphaBitPlanes,
                int width,
                int height)
{
  cout<<"Context: multisample="<<BooleanToString(multiSample)<<" alphaBitPlanes="<<BooleanToString(alphaBitPlanes)<<" "<<width<<"x"<<height<<endl;
  vtkRenderWindow *renwin = vtkRenderWindow::New();
  if(multiSample)
    {
    renwin->SetMultiSamples(8);
    }
  else
    {
    renwin->SetMultiSamples(0);
    }
  renwin->SetAlphaBitPlanes(alphaBitPlanes);
  renwin->SetSize(width,height);
//  vtkRenderer *renderer = vtkRenderer::New();
//  renwin->AddRenderer(renderer);

//  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
//  renwin->SetInteractor(iren);

  vtkOpenGLExtensionManager *extensions = vtkOpenGLExtensionManager::New();
  extensions->SetRenderWindow(renwin);
  renwin->Render();

  cout<<"OpenGL 1.1 Implementation dependent values : "<<endl;

  GLint value[2];
  GLboolean bvalue[1];
  GLfloat fvalue[2];

  const GLubyte *svalue=glGetString(GL_EXTENSIONS);
  cout<<"GL_EXTENSIONS="<<svalue<<" . Supported extensions."<<endl;

  svalue=glGetString(GL_RENDERER);
  cout<<"GL_RENDERER="<<svalue<<" . Renderer string."<<endl;

  svalue=glGetString(GL_VENDOR);
  cout<<"GL_VENDOR="<<svalue<<" . Vendor string."<<endl;

  svalue=glGetString(GL_VERSION);
  cout<<"GL_VERSION="<<svalue<<" . OpenGL version supported."<<endl;

  glGetIntegerv(GL_MAX_LIGHTS,value);
  cout<<"GL_MAX_LIGHTS="<<value[0]<<" . Maximum number of lights. Min is 8."<<endl;
  CheckMinValidValue(value[0],8);

  glGetIntegerv(GL_MAX_CLIP_PLANES,value);
  cout<<"GL_MAX_CLIP_PLANES="<<value[0]<<" . Maximum number of user clipping planes. Min is 6."<<endl;
  CheckMinValidValue(value[0],6);

  glGetIntegerv(GL_MAX_MODELVIEW_STACK_DEPTH,value);
  cout<<"GL_MAX_MODELVIEW_STACK_DEPTH="<<value[0]<<" . Maximum model-view stack depth. Min is 32."<<endl;
  CheckMinValidValue(value[0],32);

  glGetIntegerv(GL_MAX_PROJECTION_STACK_DEPTH,value);
  cout<<"GL_MAX_PROJECTION_STACK_DEPTH="<<value[0]<<" . Maximum projection matrix stack depth. Min is 2."<<endl;
  CheckMinValidValue(value[0],2);

  glGetIntegerv(GL_MAX_TEXTURE_STACK_DEPTH,value);
  cout<<"GL_MAX_TEXTURE_STACK_DEPTH="<<value[0]<<" . Maximum number depth of texture matrix stack. Min is 2."<<endl;
  CheckMinValidValue(value[0],2);

  glGetIntegerv(GL_SUBPIXEL_BITS,value);
  cout<<"GL_SUBPIXEL_BITS="<<value[0]<<" . Number of bits of subpixel precision in screen x_w and y_w. Min is 4."<<endl;
  CheckMinValidValue(value[0],4);

  glGetIntegerv(GL_MAX_TEXTURE_SIZE,value);
  cout<<"GL_MAX_TEXTURE_SIZE="<<value[0]<<" . Maximum texture image dimension. Min is 64."<<endl;
  CheckMinValidValue(value[0],64);
  cout<<"It means that the maximum 2D texture size is "<<value[0]<<"x"<<value[0]<<endl;
  cout<<"It also means that "<<(value[0]+1)<<"x1 is too large"<<endl;

  glGetIntegerv(GL_MAX_PIXEL_MAP_TABLE,value);
  cout<<"GL_MAX_PIXEL_MAP_TABLE="<<value[0]<<" . Maximum size of a PixelMap translation table. Min is 32."<<endl;
  CheckMinValidValue(value[0],32);

  glGetIntegerv(GL_MAX_NAME_STACK_DEPTH,value);
  cout<<"GL_MAX_NAME_STACK_DEPTH="<<value[0]<<" . Maximum selection name stack depth. Min is 64."<<endl;
  CheckMinValidValue(value[0],64);

  glGetIntegerv(GL_MAX_LIST_NESTING,value);
  cout<<"GL_MAX_LIST_NESTING="<<value[0]<<" . Maximum display list call nesting. Min is 64."<<endl;
  CheckMinValidValue(value[0],64);

  glGetIntegerv(GL_MAX_EVAL_ORDER,value);
  cout<<"GL_MAX_EVAL_ORDER="<<value[0]<<" . Maximum evaluator polynomial order. Min is 8."<<endl;
  CheckMinValidValue(value[0],8);

  glGetIntegerv(GL_MAX_VIEWPORT_DIMS,value);
  cout<<"GL_MAX_VIEWPORT_DIMS="<<value[0]<<"x"<<value[1]<<". Maximum viewport dimensions"<<endl;

  glGetIntegerv(GL_MAX_ATTRIB_STACK_DEPTH,value);
  cout<<"GL_MAX_ATTRIB_STACK_DEPTH="<<value[0]<<". Maximum depth of the server attribute stack. Min is 16."<<endl;
  CheckMinValidValue(value[0],16);

  glGetIntegerv(GL_MAX_CLIENT_ATTRIB_STACK_DEPTH,value);
  cout<<"GL_MAX_CLIENT_ATTRIB_STACK_DEPTH="<<value[0]<<". Maximum depth of the client attribute stack. Min is 16."<<endl;
  CheckMinValidValue(value[0],16);

  glGetIntegerv(GL_AUX_BUFFERS,value);
  cout<<"GL_AUX_BUFFERS="<<value[0]<<". Number of auxiliary buffers. Min is 0."<<endl;
  CheckMinValidValue(value[0],0);

  glGetBooleanv(GL_RGBA_MODE,bvalue);
  cout<<"GL_RGBA_MODE="<<BooleanToString(bvalue[0])<<". True if color buffers store rgba."<<endl;
  glGetBooleanv(GL_INDEX_MODE,bvalue);
  cout<<"GL_INDEX_MODE="<<BooleanToString(bvalue[0])<<". True if color buffers store indexes."<<endl;

  glGetBooleanv(GL_DOUBLEBUFFER,bvalue);
  cout<<"GL_DOUBLEBUFFER="<<BooleanToString(bvalue[0])<<". True if front and back buffers exist."<<endl;

  glGetBooleanv(GL_STEREO,bvalue);
  cout<<"GL_STEREO="<<BooleanToString(bvalue[0])<<". True if left and right buffers exist."<<endl;
  glGetFloatv(GL_POINT_SIZE_RANGE,fvalue);
  cout<<"GL_POINT_SIZE_RANGE="<<fvalue[0]<<","<<fvalue[1]<<". Range (lo to hi) of antialiased point sizes. Min is (1,1)"<<endl;
  // check range.

  glGetFloatv(GL_POINT_SIZE_GRANULARITY,fvalue);
  cout<<"GL_POINT_SIZE_GRANULARITY="<<fvalue[0]<<". Antialiased point size granularity."<<endl;

  glGetFloatv(GL_LINE_WIDTH_RANGE,fvalue);
  cout<<"GL_LINE_WIDTH_RANGE="<<fvalue[0]<<","<<fvalue[1]<<". Range (lo to hi) of antialiased line widths. Min is (1,1)"<<endl;
  // check range.

  glGetFloatv(GL_LINE_WIDTH_GRANULARITY,fvalue);
  cout<<"GL_LINE_WIDTH_GRANULARITY="<<fvalue[0]<<". Antialiased line width granularity."<<endl;

  glGetIntegerv(GL_RED_BITS,value);
  cout<<"GL_RED_BITS="<<value[0]<<". Number of bits per red component in color buffers."<<endl;
  glGetIntegerv(GL_GREEN_BITS,value);
  cout<<"GL_GREEN_BITS="<<value[0]<<". Number of bits per green component in color buffers."<<endl;
  glGetIntegerv(GL_BLUE_BITS,value);
  cout<<"GL_BLUE_BITS="<<value[0]<<". Number of bits per blue component in color buffers."<<endl;
  glGetIntegerv(GL_ALPHA_BITS,value);
  cout<<"GL_ALPHA_BITS="<<value[0]<<". Number of bits per alpha component in color buffers."<<endl;
  glGetIntegerv(GL_INDEX_BITS,value);
  cout<<"GL_INDEX_BITS="<<value[0]<<". Number of bits per index component in color buffers."<<endl;
  glGetIntegerv(GL_DEPTH_BITS,value);
  cout<<"GL_DEPTH_BITS="<<value[0]<<". Number of depth buffer planes."<<endl;

  glGetIntegerv(GL_STENCIL_BITS,value);
  cout<<"GL_STENCIL_BITS="<<value[0]<<". Number of stencil planes."<<endl;

  glGetIntegerv(GL_ACCUM_RED_BITS,value);
  cout<<"GL_ACCUM_RED_BITS="<<value[0]<<". Number of bits per red component in the accumulation buffer."<<endl;
  glGetIntegerv(GL_ACCUM_GREEN_BITS,value);
  cout<<"GL_ACCUM_GREEN_BITS="<<value[0]<<". Number of bits per green component in accumulation buffer."<<endl;
  glGetIntegerv(GL_ACCUM_BLUE_BITS,value);
  cout<<"GL_ACCUM_BLUE_BITS="<<value[0]<<". Number of bits per blue component in accumulation buffer."<<endl;
  glGetIntegerv(GL_ACCUM_ALPHA_BITS,value);
  cout<<"GL_ACCUM_ALPHA_BITS="<<value[0]<<". Number of bits per alpha component in accumulation buffer."<<endl;

  if(extensions->LoadSupportedExtension("GL_VERSION_1_2"))
    {
    cout<<endl<<"OpenGL 1.2 Implementation dependent values : "<<endl;

    glGetIntegerv(vtkgl::MAX_3D_TEXTURE_SIZE,value);
    cout<<"GL_MAX_3D_TEXTURE_SIZE="<<value[0]<<" . Maximum 3D texture image dimension. Min is 16."<<endl;
    CheckMinValidValue(value[0],16);
    cout<<"It means that the maximum 3D texture size is "<<value[0]<<"x"<<value[0]<<"x"<<value[0]<<endl;
    cout<<"It also means that "<<(value[0]+1)<<"x1x1 is too large"<<endl;

    glGetFloatv(vtkgl::ALIASED_POINT_SIZE_RANGE,fvalue);
    cout<<"GL_ALIASED_POINT_SIZE_RANGE="<<fvalue[0]<<","<<fvalue[1]<<". Range (lo to hi) of aliased point sizes. Min is (1,1)"<<endl;
    // check range.

    glGetFloatv(vtkgl::SMOOTH_POINT_SIZE_RANGE,fvalue);
    cout<<"GL_SMOOTH_POINT_SIZE_RANGE (GL_POINT_SIZE_RANGE in 1.1)="<<fvalue[0]<<","<<fvalue[1]<<". Range (lo to hi) of antialiased point sizes. Min is (1,1)"<<endl;
    // check range.

    glGetFloatv(vtkgl::SMOOTH_POINT_SIZE_GRANULARITY,fvalue);
    cout<<"GL_SMOOTH_POINT_SIZE_GRANULARITY (GL_POINT_SIZE_GRANULARITY in 1.1)="<<fvalue[0]<<". Antialiased point size granularity."<<endl;

    glGetFloatv(vtkgl::ALIASED_LINE_WIDTH_RANGE,fvalue);
    cout<<"GL_ALIASED_LINE_WIDTH_RANGE="<<fvalue[0]<<","<<fvalue[1]<<". Range (lo to hi) of aliased line widths. Min is (1,1)"<<endl;
    // check range.

    glGetFloatv(vtkgl::SMOOTH_LINE_WIDTH_RANGE,fvalue);
    cout<<"GL_SMOOTH_LINE_WIDTH_RANGE (GL_LINE_WIDTH_RANGE in 1.1)="<<fvalue[0]<<","<<fvalue[1]<<". Range (lo to hi) of antialiased line widths. Min is (1,1)"<<endl;
    // check range.

    glGetFloatv(vtkgl::SMOOTH_LINE_WIDTH_GRANULARITY,fvalue);
    cout<<"GL_SMOOTH_LINE_WIDTH_GRANULARITY (GL_LINE_WIDTH_GRANULARITY in 1.1)="<<fvalue[0]<<". Antialiased line width granularity."<<endl;

    glGetIntegerv(vtkgl::MAX_ELEMENTS_INDICES,value);
    cout<<"GL_MAX_ELEMENTS_INDICES="<<value[0]<<" . Recommended maximum number of DrawRangeElements indices."<<endl;

    glGetIntegerv(vtkgl::MAX_ELEMENTS_VERTICES,value);
    cout<<"GL_MAX_ELEMENTS_VERTICES="<<value[0]<<" . Recommended maximum number of DrawRangeElements vertices."<<endl;

    if(extensions->LoadSupportedExtension("GL_ARB_imaging"))
      {
      cout<<"imaging subset is supported."<<endl;

      glGetIntegerv(vtkgl::MAX_COLOR_MATRIX_STACK_DEPTH,value);
      cout<<"GL_MAX_COLOR_MATRIX_STACK_DEPTH="<<value[0]<<" . Maximum color matrix stack depth. Min is 2."<<endl;
      CheckMinValidValue(value[0],2);

      vtkgl::GetConvolutionParameteriv(vtkgl::CONVOLUTION_1D,
                                       vtkgl::MAX_CONVOLUTION_WIDTH,value);

      cout<<"for GL_CONVOLUTION_1D, GL_MAX_CONVOLUTION_WIDTH="<<value[0]<<" . Maximum width of the convolution filter. Min is 3."<<endl;
      CheckMinValidValue(value[0],3);

      vtkgl::GetConvolutionParameteriv(vtkgl::CONVOLUTION_2D,
                                       vtkgl::MAX_CONVOLUTION_WIDTH,value);
      cout<<"for GL_CONVOLUTION_2D, GL_MAX_CONVOLUTION_WIDTH="<<value[0]<<" . Maximum width of the convolution filter. Min is 3."<<endl;
      CheckMinValidValue(value[0],3);

      vtkgl::GetConvolutionParameteriv(vtkgl::CONVOLUTION_2D,
                                       vtkgl::MAX_CONVOLUTION_HEIGHT,value);
      cout<<"for GL_CONVOLUTION_2D, GL_MAX_CONVOLUTION_HEIGHT="<<value[0]<<" . Maximum height of the convolution filter. Min is 3."<<endl;
      CheckMinValidValue(value[0],3);

      vtkgl::GetConvolutionParameteriv(vtkgl::SEPARABLE_2D,
                                       vtkgl::MAX_CONVOLUTION_WIDTH,value);
      cout<<"for GL_SEPARABLE_2D, GL_MAX_CONVOLUTION_WIDTH="<<value[0]<<" . Maximum width of the convolution filter. Min is 3."<<endl;
      CheckMinValidValue(value[0],3);

      vtkgl::GetConvolutionParameteriv(vtkgl::SEPARABLE_2D,
                                       vtkgl::MAX_CONVOLUTION_HEIGHT,value);
      cout<<"for GL_SEPARABLE_2D, GL_MAX_CONVOLUTION_HEIGHT="<<value[0]<<" . Maximum height of the convolution filter. Min is 3."<<endl;
      CheckMinValidValue(value[0],3);
      }
    else
      {
      cout<<"imaging subset is not supported."<<endl;
      }
    }
  if(extensions->LoadSupportedExtension("GL_VERSION_1_3"))
    {
    cout<<endl<<"OpenGL 1.3 Implementation dependent values : "<<endl;
    glGetIntegerv(vtkgl::MAX_CUBE_MAP_TEXTURE_SIZE,value);
    cout<<"GL_MAX_CUBE_MAP_TEXTURE_SIZE="<<value[0]<<" . Maximum cube map texture image dimension. Min is 16."<<endl;
    CheckMinValidValue(value[0],16);
    cout<<"It means that the maximum cube map texture size is "<<value[0]<<"x"<<value[0]<<endl;
    cout<<"It also means that "<<(value[0]+1)<<"x1 is too large"<<endl;

    glGetIntegerv(vtkgl::MAX_TEXTURE_UNITS,value);
    cout<<"GL_MAX_TEXTURE_UNITS="<<value[0]<<" . Number of (fixed-function) texture units. Min is 2. Max is 32."<<endl;
    CheckMinValidValue(value[0],2);
    CheckMaxValidValue(value[0],32);

    glGetIntegerv(vtkgl::SAMPLE_BUFFERS,value);
    cout<<"GL_SAMPLE_BUFFERS="<<value[0]<<" . Number of multisample buffers. Min is 0."<<endl;
    CheckMinValidValue(value[0],0);

    glGetIntegerv(vtkgl::SAMPLES,value);
    cout<<"GL_SAMPLES="<<value[0]<<" . Coverage mask size. Min is 0."<<endl;
    CheckMinValidValue(value[0],0);

    GLint count;

    glGetIntegerv(vtkgl::NUM_COMPRESSED_TEXTURE_FORMATS,&count);
    cout<<"GL_NUM_COMPRESSED_TEXTURE_FORMATS="<<count<<" . Number of enumerated compressed texture formats."<<endl;
    CheckMinValidValue(count,0);

    if(count>0)
      {
      GLint *ivalues=new GLint[count];
      glGetIntegerv(vtkgl::COMPRESSED_TEXTURE_FORMATS,ivalues);
      cout<<"GL_COMPRESSED_TEXTURE_FORMATS (Enumerated compressed texture formats)=";
      int i=0;
      while(i<count)
        {
        cout<<" "<<TextureCompressionFormat(ivalues[i])<<"(0x"<< hex << ivalues[i] << dec << ")";
        ++i;
        }
      delete[] ivalues;
      cout<<endl;
      }
    }
  if(extensions->LoadSupportedExtension("GL_VERSION_1_4"))
    {
    cout<<endl<<"OpenGL 1.4 Implementation dependent values : "<<endl;
    glGetFloatv(vtkgl::MAX_TEXTURE_LOD_BIAS,fvalue);
    cout<<"GL_MAX_TEXTURE_LOD_BIAS="<<fvalue[0]<<" . Maximum absolute texture level of detail bias. Min is 2.0."<<endl;
    CheckMinValidFValue(fvalue[0],2.0);
    }

  depth_texture_supported=extensions->ExtensionSupported("GL_VERSION_1_4") ||
    extensions->ExtensionSupported("GL_ARB_depth_texture");

  if(extensions->LoadSupportedExtension("GL_VERSION_1_5"))
    {
    cout<<endl<<"OpenGL 1.5 Implementation dependent values : "<<endl;

    vtkgl::GetQueryiv(vtkgl::SAMPLES_PASSED,vtkgl::QUERY_COUNTER_BITS,value);
    cout<<"GL_QUERY_COUNTER_BITS="<<value[0]<<" . Occlusion query counter bits. Max is 32."<<endl;
    CheckMaxValidValue(value[0],32);
    }

  if(extensions->LoadSupportedExtension("GL_VERSION_2_0"))
    {
    cout<<endl<<"OpenGL 2.0 Implementation dependent values : "<<endl;

    svalue=glGetString(vtkgl::SHADING_LANGUAGE_VERSION);
    cout<<"GL_SHADING_LANGUAGE_VERSION="<<svalue<<" . Shading Language version supported."<<endl;

    glGetIntegerv(vtkgl::MAX_VERTEX_ATTRIBS,value);
    cout<<"GL_MAX_VERTEX_ATTRIBS="<<value[0]<<" . Number of active vertex attributes. Min is 16."<<endl;
    CheckMinValidValue(value[0],16);

    glGetIntegerv(vtkgl::MAX_VERTEX_UNIFORM_COMPONENTS,value);
    cout<<"GL_MAX_VERTEX_UNIFORM_COMPONENTS="<<value[0]<<" . Number of words for vertex shader uniform variables. Min is 512."<<endl;
    CheckMinValidValue(value[0],512);

    glGetIntegerv(vtkgl::MAX_VARYING_FLOATS,value);
    cout<<"GL_MAX_VARYING_FLOATS="<<value[0]<<" . Number of floats for varying variables. Min is 32."<<endl;
    CheckMinValidValue(value[0],32);

    glGetIntegerv(vtkgl::MAX_COMBINED_TEXTURE_IMAGE_UNITS,value);
    cout<<"GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS="<<value[0]<<" . Total number of texture units accessible by the GL. Min is 2."<<endl;
    CheckMinValidValue(value[0],2);

    glGetIntegerv(vtkgl::MAX_VERTEX_TEXTURE_IMAGE_UNITS,value);
    cout<<"GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS="<<value[0]<<" . Number of texture image units accessible by a vertex shader. Min is 0."<<endl;
    CheckMinValidValue(value[0],0);

    glGetIntegerv(vtkgl::MAX_TEXTURE_IMAGE_UNITS,value);
    cout<<"GL_MAX_TEXTURE_IMAGE_UNITS="<<value[0]<<" . Number of texture image units accessible by fragment processing. Min is 2."<<endl;
    CheckMinValidValue(value[0],2);

    glGetIntegerv(vtkgl::MAX_TEXTURE_COORDS,value);
    cout<<"GL_MAX_TEXTURE_COORDS="<<value[0]<<" . Number of texture coordinate sets. Min is 2."<<endl;
    CheckMinValidValue(value[0],2);

    glGetIntegerv(vtkgl::MAX_FRAGMENT_UNIFORM_COMPONENTS,value);
    cout<<"GL_MAX_FRAGMENT_UNIFORM_COMPONENTS="<<value[0]<<" . Number of words for fragment shader uniform variables. Min is 64."<<endl;
    CheckMinValidValue(value[0],64);

    glGetIntegerv(vtkgl::MAX_DRAW_BUFFERS,value);
    cout<<"GL_MAX_DRAW_BUFFERS="<<value[0]<<" . Maximum number of active draw buffers. Min is 1."<<endl;
    CheckMinValidValue(value[0],1);
    }


  srgb_texture_supported=extensions->ExtensionSupported("GL_VERSION_2_1") ||
    extensions->ExtensionSupported("GL_EXT_texture_sRGB");
  float_texture_supported=extensions->LoadSupportedExtension("GL_ARB_texture_float")==1;
  integer_texture_supported=extensions->ExtensionSupported("GL_EXT_texture_integer")==1;

  ARB_texture_rectangle_supported=extensions->LoadSupportedExtension("GL_ARB_texture_rectangle")==1;

  if(ARB_texture_rectangle_supported)
    {
    cout<<endl<<"GL_ARB_texture_rectangle extension Implementation dependent values : "<<endl;

    glGetIntegerv(vtkgl::MAX_RECTANGLE_TEXTURE_SIZE_ARB,value);
    cout<<"MAX_RECTANGLE_TEXTURE_SIZE_ARB="<<value[0]<<" . Maximum rectangle texture image dimension. Min is 64."<<endl;
    CheckMinValidValue(value[0],64);
    cout<<"It means that the maximum rectangle texture size is "<<value[0]<<"x"<<value[0]<<endl;
    cout<<"It also means that "<<(value[0]+1)<<"x1 is too large"<<endl;
    }

  if(extensions->LoadSupportedExtension("GL_EXT_framebuffer_object"))
    {
    cout<<endl<<"GL_EXT_framebuffer_object extension Implementation dependent values : "<<endl;

    glGetIntegerv(vtkgl::MAX_COLOR_ATTACHMENTS_EXT,value);
    cout<<"MAX_COLOR_ATTACHMENTS_EXT="<<value[0]<<" . Maximum number of attachment points to color buffers when using framebuffer objects. Min is 1."<<endl;
    CheckMinValidValue(value[0],1);
    glGetIntegerv(vtkgl::MAX_RENDERBUFFER_SIZE_EXT,value);
    cout<<"MAX_RENDERBUFFER_SIZE_EXT="<<value[0]<<" . Maximum width and height of renderbuffers supported by the implementation. Min is 1."<<endl;
    CheckMinValidValue(value[0],1);
    }
  extensions->Delete();
  renwin->Delete();
}

int windowSize[2]={512,511};

int main(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
{
  cout << "CTEST_FULL_OUTPUT (Avoid ctest truncation of output)" << endl;

  int multisample=0;
  while(multisample<2)
    {
    int alpha=0;
    while(alpha<2)
      {
      int index=0;
      while(index<2)
        {
        TestVisual(multisample,alpha,windowSize[index],windowSize[index]);
        ++index;
        }
      ++alpha;
      }
    ++multisample;
    }

  vtkRenderWindow *renwin = vtkRenderWindow::New();
  renwin->SetAlphaBitPlanes(1);
  renwin->SetSize(250, 250);

  vtkRenderer *renderer = vtkRenderer::New();
  renwin->AddRenderer(renderer);

  // Force a Render here so that we can call glGetString reliably:
  //
  renwin->Render();

  if(vtkgl::TexImage3D!=0)
    {
    QueryTexture3D();
    }
  QueryTexture2D();
  QueryTexture1D();

  if(ARB_texture_rectangle_supported)
    {
    QueryTexture2DRectangle();
    }

  // Check if non-power-of-two texture is supported based on glError not
  // on the OpenGL version returned by the driver or on the list of
  // extensions returned by the driver.

  // clean glError
  GLenum errorCode=glGetError();
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16,64,64,0, GL_RGBA, GL_FLOAT, NULL );
  errorCode=glGetError();
  if(errorCode!=GL_NO_ERROR)
    {
    cout << "Loading a power-of-two texture failed with the following error:"
         << OpenGLErrorMessage2(errorCode) <<endl;
    }
  glTexImage2D(GL_TEXTURE_2D,0,GL_RGBA16,64,65,0, GL_RGBA, GL_FLOAT, NULL );
  errorCode=glGetError();
  if(errorCode!=GL_NO_ERROR)
    {
    cout << "Loading a none-power-of-two texture failed with the following"
         << "error:" << OpenGLErrorMessage2(errorCode) <<endl;
    }




  TestTextureFormatsAndFBO();

  renderer->Delete();
  renwin->Delete();

  return 0; // 0==passed, always pass.
}
