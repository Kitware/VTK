/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLSLShaderDeviceAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkGLSLShaderDeviceAdapter.h"

#include "vtkCollection.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkGLSLShaderProgram.h"
#include "vtkObjectFactory.h"

#include <map>
#include <string>

#include "vtkgl.h"

class vtkGLSLShaderDeviceAdapter::vtkInternal
{
public:
  typedef std::map<std::string, int> MapOfStringToInt;
  MapOfStringToInt AttributeIndicesCache;
};

vtkStandardNewMacro(vtkGLSLShaderDeviceAdapter);

#define GLSL_SHADER_DEVICE_ADAPTER(msg) \
  /* cout << __LINE__ << " vtkGLSLShaderDeviceAdapter " << msg << endl; */

//---------------------------------------------------------------------------
vtkGLSLShaderDeviceAdapter::vtkGLSLShaderDeviceAdapter()
{
  GLSL_SHADER_DEVICE_ADAPTER("constructor");
  this->Internal = new vtkInternal();
}

//---------------------------------------------------------------------------
vtkGLSLShaderDeviceAdapter::~vtkGLSLShaderDeviceAdapter()
{
  GLSL_SHADER_DEVICE_ADAPTER("destructor");
  delete this->Internal;
}

//---------------------------------------------------------------------------
static inline GLenum VTK2SignedOpenGLType(int type)
{
  switch (type)
    {
#if VTK_SIZEOF_CHAR == 1
    case VTK_CHAR:              return GL_BYTE;
    case VTK_UNSIGNED_CHAR:     return GL_BYTE;
#elif VTK_SIZE_OF_CHAR == 2
    case VTK_CHAR:              return GL_SHORT;
    case VTK_UNSIGNED CHAR:     return GL_SHORT;
#endif

#if VTK_SIZEOF_SHORT == 1
    case VTK_SHORT:             return GL_BYTE;
    case VTK_UNSIGNED_SHORT:    return GL_BYTE;
#elif VTK_SIZEOF_SHORT == 2
    case VTK_SHORT:             return GL_SHORT;
    case VTK_UNSIGNED_SHORT:    return GL_SHORT;
#elif VTK_SIZEOF_SHORT == 4
    case VTK_SHORT:             return GL_INT;
    case VTK_UNSIGNED_SHORT:    return GL_INT;
#endif

#if VTK_SIZEOF_INT == 2
    case VTK_INT:               return GL_SHORT;
    case VTK_UNSIGNED_INT:      return GL_SHORT;
#elif VTK_SIZEOF_INT == 4
    case VTK_INT:               return GL_INT;
    case VTK_UNSIGNED_INT:      return GL_INT;
#endif

#if VTK_SIZEOF_ID_TYPE == 4
    case VTK_ID_TYPE:           return GL_INT;
#endif

#if VTK_SIZEOF_LONG == 4
    case VTK_LONG:              return GL_INT;
    case VTK_UNSIGNED_LONG:     return GL_INT;
#endif

#if VTK_SIZEOF_FLOAT == 4
    case VTK_FLOAT:             return GL_FLOAT;
#elif VTK_SIZEOF_FLOAT == 8
    case VTK_FLOAT:             return GL_DOUBLE;
#endif

#if VTK_SIZEOF_DOUBLE == 4
    case VTK_DOUBLE:            return GL_FLOAT;
#elif VTK_SIZEOF_DOUBLE == 8
    case VTK_DOUBLE:            return GL_DOUBLE;
#endif

    default:                    return GL_FALSE;
    }
}

//---------------------------------------------------------------------------
int vtkGLSLShaderDeviceAdapter::GetAttributeLocation(const char *attributeName)
{
  vtkGLSLShaderProgram* glslProgram =
    vtkGLSLShaderProgram::SafeDownCast(this->ShaderProgram);
  if (glslProgram && glslProgram->GetProgram())
    {
    GLSL_SHADER_DEVICE_ADAPTER(
      "GetAttributeLocation Program " << glslProgram->GetProgram());
    return vtkgl::GetAttribLocation(glslProgram->GetProgram(), attributeName);
    }
  return -1;
}

//---------------------------------------------------------------------------
void vtkGLSLShaderDeviceAdapter::PrepareForRender()
{
  this->Internal->AttributeIndicesCache.clear();
}

//---------------------------------------------------------------------------
void vtkGLSLShaderDeviceAdapter::SendAttribute(const char *attrname,
                                               int components,
                                               int type,
                                               const void *attribute,
                                               unsigned long offset)
{
  int index;
  vtkInternal::MapOfStringToInt::iterator iter =
    this->Internal->AttributeIndicesCache.find(attrname);
  if (iter == this->Internal->AttributeIndicesCache.end())
    {
    index = this->GetAttributeLocation(attrname);
    if (index < 0)
      {
      // failed.
      return;
      }
    this->Internal->AttributeIndicesCache[attrname]=index;
    }
  else
    {
    index = iter->second;
    }

  if (!attribute)
    {
    return;
    }

  if (components <=0 || components > 4)
    {
    vtkErrorMacro(<< components<< " components not supported.");
    return;
    }

  if (index >= 0)
    {
    switch (VTK2SignedOpenGLType(type))
      {
    case GL_SHORT:
      switch (components)
        {
        case 1:
          vtkgl::VertexAttrib1sv(index, static_cast<const GLshort *>(attribute)
                                 + offset);
          break;
        case 2:
          vtkgl::VertexAttrib2sv(index, static_cast<const GLshort *>(attribute)
                                 + offset);
          break;
        case 3:
          vtkgl::VertexAttrib3sv(index, static_cast<const GLshort *>(attribute)
                                 + offset);
          break;
        case 4:
          vtkgl::VertexAttrib4sv(index, static_cast<const GLshort *>(attribute)
                                 + offset);
          break;
        }
      break;
      case GL_FLOAT:
        switch(components)
          {
          case 1:
            GLSL_SHADER_DEVICE_ADAPTER( "SENDING " << components << " ATTRIBUTES "
                                        << static_cast<const float*>(attribute)[offset] );
            break;
          case 2:
            GLSL_SHADER_DEVICE_ADAPTER( "SENDING " << components << " ATTRIBUTES "
                                        << static_cast<const float*>(attribute)[offset] << " "
                                        << static_cast<const float*>(attribute)[offset+1] );
            break;
          case 3:
            GLSL_SHADER_DEVICE_ADAPTER( "SENDING " << components << " ATTRIBUTES "
                                        << static_cast<const float*>(attribute)[offset] << " "
                                        << static_cast<const float*>(attribute)[offset+1] << " "
                                        << static_cast<const float*>(attribute)[offset+2] );
            break;
          case 4:
            GLSL_SHADER_DEVICE_ADAPTER( "SENDING " << components << " ATTRIBUTES "
                                        << static_cast<const float*>(attribute)[offset] << " "
                                        << static_cast<const float*>(attribute)[offset+1] << " "
                                        << static_cast<const float*>(attribute)[offset+2] << " "
                                        << static_cast<const float*>(attribute)[offset+3] );
            break;
          default:
            GLSL_SHADER_DEVICE_ADAPTER( "SENDING " << components << " ATTRIBUTES "
                                        << static_cast<const float*>(attribute)[offset] << " UNSUPPORTED NUMBER OF COMPONENTS");
          }
        switch (components)
          {
          case 1:
            vtkgl::VertexAttrib1fv(index,
                                   static_cast<const GLfloat *>(attribute)
                                   + offset);
            break;
          case 2:
            vtkgl::VertexAttrib2fv(index,
                                   static_cast<const GLfloat *>(attribute)
                                   + offset);
            break;
          case 3:
            vtkgl::VertexAttrib3fv(index,
                                   static_cast<const GLfloat *>(attribute)
                                   + offset);
            break;
          case 4:
            vtkgl::VertexAttrib4fv(index,
                                   static_cast<const GLfloat *>(attribute)
                                   + offset);
            break;
          }
        break;
      case GL_DOUBLE:
        if(components == 3)
          {
          GLSL_SHADER_DEVICE_ADAPTER("SendingAttribute index " << index << " ["
                                     << static_cast<const GLdouble *>(attribute)[offset] << " "
                                     << static_cast<const GLdouble *>(attribute)[offset+1] << " "
                                     << static_cast<const GLdouble *>(attribute)[offset+2] << "]");
          }
        switch (components)
          {
          case 1:
            vtkgl::VertexAttrib1dv(index,
                                   static_cast<const GLdouble *>(attribute)
                                   + offset);
            break;
          case 2:
            vtkgl::VertexAttrib2dv(index,
                                   static_cast<const GLdouble *>(attribute)
                                   + offset);
            break;
          case 3:
            vtkgl::VertexAttrib3dv(index,
                                   static_cast<const GLdouble *>(attribute)
                                   + offset);
            break;
          case 4:
            vtkgl::VertexAttrib4dv(index,
                                   static_cast<const GLdouble *>(attribute)
                                   + offset);
            break;
          }
        break;
      default:
        vtkErrorMacro("Unsupported type for vertex attribute: " << type);
        return;
      }
    }
  else
    {
    vtkErrorMacro("Unsupported attribute index: " << index);
    }
  return;
};

//---------------------------------------------------------------------------
void vtkGLSLShaderDeviceAdapter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
