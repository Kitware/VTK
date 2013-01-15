/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLPainterDeviceAdapter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkOpenGLPainterDeviceAdapter.h"

#include "vtkDataArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLExtensionManager.h"
#include "vtkRenderer.h"
#include "vtkgl.h"

#include <algorithm>

#include "vtkOpenGL.h"

vtkStandardNewMacro(vtkOpenGLPainterDeviceAdapter);

//-----------------------------------------------------------------------------
vtkOpenGLPainterDeviceAdapter::vtkOpenGLPainterDeviceAdapter()
{
  this->PointSize = 1.0;
  this->RangeNear = 0.0;
  this->RangeFar = 1.0;
  this->MaxStencil = 0;
  this->Initialized = false;
}

//-----------------------------------------------------------------------------
vtkOpenGLPainterDeviceAdapter::~vtkOpenGLPainterDeviceAdapter()
{
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

// This array is a map from VTK primitive identifiers (VTK_VERTEX, VTK_LINE,
// etc.) to OpenGL polygon primitive identifiers (GL_POINTS, GL_LINES, etc.)
// Note that some VTK polygon types (e.g. VTK_EMPTY_CELL and VTK_PIXEL) have no
// analogue in OpenGL.  Using them will undoubtedly result in an OpenGL error.
static const GLenum VTK2OpenGLPrimitive[] = {
  static_cast<GLenum>(0xFFFF),       // 0 - VTK_EMPTY_CELL
  GL_POINTS,            // 1 - VTK_VERTEX
  GL_POINTS,            // 2 - VTK_POLY_VERTEX
  GL_LINES,             // 3 - VTK_LINE
  GL_LINE_STRIP,        // 4 - VTK_POLY_LINE
  GL_TRIANGLES,         // 5 - VTK_TRIANGLE
  GL_TRIANGLE_STRIP,    // 6 - VTK_TRIANGLE_STRIP
  GL_POLYGON,           // 7 - VTK_POLYGON
  static_cast<GLenum>(0xFFFF),       // 8 - VTK_PIXEL
  GL_QUADS,             // 9 - VTK_QUAD
  GL_LINE_LOOP          // 10 - VTK_TETRA
};

static inline GLenum VTK2OpenGLType(int type)
{
  switch (type)
    {
#if VTK_SIZEOF_CHAR == 1
    case VTK_CHAR:              return GL_BYTE;
    case VTK_UNSIGNED_CHAR:     return GL_UNSIGNED_BYTE;
#elif VTK_SIZE_OF_CHAR == 2
    case VTK_CHAR:              return GL_SHORT;
    case VTK_UNSIGNED CHAR:     return GL_UNSIGNED_SHORT;
#endif

#if VTK_SIZEOF_SHORT == 1
    case VTK_SHORT:             return GL_BYTE;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_BYTE;
#elif VTK_SIZEOF_SHORT == 2
    case VTK_SHORT:             return GL_SHORT;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_SHORT;
#elif VTK_SIZEOF_SHORT == 4
    case VTK_SHORT:             return GL_INT;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_INT == 2
    case VTK_INT:               return GL_SHORT;
    case VTK_UNSIGNED_INT:      return GL_UNSIGNED_SHORT;
#elif VTK_SIZEOF_INT == 4
    case VTK_INT:               return GL_INT;
    case VTK_UNSIGNED_INT:      return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_LONG == 4
    case VTK_LONG:              return GL_INT;
    case VTK_UNSIGNED_LONG:     return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_ID_TYPE == 4
    case VTK_ID_TYPE:           return GL_INT;
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

static inline GLenum VTK2UnsignedOpenGLType(int type)
{
  switch (type)
    {
#if VTK_SIZEOF_CHAR == 1
    case VTK_CHAR:              return GL_UNSIGNED_BYTE;
    case VTK_UNSIGNED_CHAR:     return GL_UNSIGNED_BYTE;
#elif VTK_SIZE_OF_CHAR == 2
    case VTK_CHAR:              return GL_UNSIGNED_SHORT;
    case VTK_UNSIGNED CHAR:     return GL_UNSIGNED_SHORT;
#endif

#if VTK_SIZEOF_SHORT == 1
    case VTK_SHORT:             return GL_UNSIGNED_BYTE;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_BYTE;
#elif VTK_SIZEOF_SHORT == 2
    case VTK_SHORT:             return GL_UNSIGNED_SHORT;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_SHORT;
#elif VTK_SIZEOF_SHORT == 4
    case VTK_SHORT:             return GL_UNSIGNED_INT;
    case VTK_UNSIGNED_SHORT:    return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_INT == 2
    case VTK_INT:               return GL_UNSIGNED_SHORT;
    case VTK_UNSIGNED_INT:      return GL_UNSIGNED_SHORT;
#elif VTK_SIZEOF_INT == 4
    case VTK_INT:               return GL_UNSIGNED_INT;
    case VTK_UNSIGNED_INT:      return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_ID_TYPE == 4
    case VTK_ID_TYPE:           return GL_UNSIGNED_INT;
#endif

#if VTK_SIZEOF_LONG == 4
    case VTK_LONG:              return GL_UNSIGNED_INT;
    case VTK_UNSIGNED_LONG:     return GL_UNSIGNED_INT;
#endif

    default:                    return GL_FALSE;
    }
}


//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::BeginPrimitive(int mode)
{
  glBegin(VTK2OpenGLPrimitive[mode]);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::EndPrimitive()
{
  glEnd();
}

//-----------------------------------------------------------------------------
int vtkOpenGLPainterDeviceAdapter::IsAttributesSupported(int attribute)
{
  switch(attribute)
    {
  case vtkDataSetAttributes::NUM_ATTRIBUTES:
  case vtkDataSetAttributes::NORMALS:
  case vtkDataSetAttributes::SCALARS:
  case vtkDataSetAttributes::TCOORDS:
  case vtkDataSetAttributes::EDGEFLAG:
    return 1;
    }
  return 0;
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::SendAttribute(int index, int numcomp,
  int type, const void *attribute, vtkIdType offset/*=0*/)
{
  switch (index)
    {
  case vtkDataSetAttributes::NUM_ATTRIBUTES:     // Vertex
      if ((numcomp < 2) || (numcomp > 4))
        {
        vtkErrorMacro("Bad number of components.");
        return;
        }
      switch (VTK2SignedOpenGLType(type))
        {
        case GL_SHORT:
          switch (numcomp)
            {
            case 2:
              glVertex2sv(static_cast<const GLshort *>(attribute) + offset);
              break;
            case 3:
              glVertex3sv(static_cast<const GLshort *>(attribute) + offset);
              break;
            case 4:
              glVertex4sv(static_cast<const GLshort *>(attribute) + offset);
              break;
            }
          break;
        case GL_INT:
          switch (numcomp)
            {
            case 2:
              glVertex2iv(static_cast<const GLint *>(attribute) + offset);
              break;
            case 3:
              glVertex3iv(static_cast<const GLint *>(attribute) + offset);
              break;
            case 4:
              glVertex4iv(static_cast<const GLint *>(attribute) + offset);
              break;
            }
          break;
        case GL_FLOAT:
          switch (numcomp)
            {
            case 2:
              glVertex2fv(static_cast<const GLfloat *>(attribute) + offset);
              break;
            case 3:
              glVertex3fv(static_cast<const GLfloat *>(attribute) + offset);
              break;
            case 4:
              glVertex4fv(static_cast<const GLfloat *>(attribute) + offset);
              break;
            }
          break;
        case GL_DOUBLE:
          switch (numcomp)
            {
            case 2:
              glVertex2dv(static_cast<const GLdouble *>(attribute) + offset);
              break;
            case 3:
              glVertex3dv(static_cast<const GLdouble *>(attribute) + offset);
              break;
            case 4:
              glVertex4dv(static_cast<const GLdouble *>(attribute) + offset);
              break;
            }
          break;
        default:
          vtkErrorMacro("Unsupported type for vertices: " << type);
          return;
        }
      break;
  case vtkDataSetAttributes::NORMALS:     // Normal
      if (numcomp != 3)
        {
        vtkErrorMacro("Bad number of components.");
        return;
        }
      switch (VTK2SignedOpenGLType(type))
        {
        case GL_BYTE:
          glNormal3bv(static_cast<const GLbyte *>(attribute) + offset);
          break;
        case GL_SHORT:
          glNormal3sv(static_cast<const GLshort *>(attribute) + offset);
          break;
        case GL_INT:
          glNormal3iv(static_cast<const GLint *>(attribute) + offset);
          break;
        case GL_FLOAT:
          glNormal3fv(static_cast<const GLfloat *>(attribute) + offset);
          break;
        case GL_DOUBLE:
          glNormal3dv(static_cast<const GLdouble *>(attribute) + offset);
          break;
        default:
          vtkErrorMacro("Unsupported type for normals: " << type);
          return;
        }
      break;
  case vtkDataSetAttributes::SCALARS:     // Color
      if ((numcomp != 3) && (numcomp != 4))
        {
        vtkErrorMacro("Bad number of components.");
        return;
        }
      switch (VTK2OpenGLType(type))
        {
        case GL_BYTE:
          switch (numcomp)
            {
            case 3: glColor3bv(static_cast<const GLbyte *>(attribute) + offset);
              break;
            case 4: glColor4bv(static_cast<const GLbyte *>(attribute) + offset);
              break;
            }
          break;
        case GL_UNSIGNED_BYTE:
          switch (numcomp)
            {
            case 3: glColor3ubv(static_cast<const GLubyte *>(attribute) + offset);
              break;
            case 4: glColor4ubv(static_cast<const GLubyte *>(attribute) + offset);
              break;
            }
          break;
        case GL_SHORT:
          switch (numcomp)
            {
            case 3: glColor3sv(static_cast<const GLshort *>(attribute) + offset);
              break;
            case 4: glColor4sv(static_cast<const GLshort *>(attribute) + offset);
              break;
            }
          break;
        case GL_UNSIGNED_SHORT:
          switch (numcomp)
            {
            case 3: glColor3usv(static_cast<const GLushort *>(attribute) + offset);
              break;
            case 4: glColor4usv(static_cast<const GLushort *>(attribute) + offset);
              break;
            }
          break;
        case GL_INT:
          switch (numcomp)
            {
            case 3: glColor3iv(static_cast<const GLint *>(attribute) + offset);
              break;
            case 4: glColor4iv(static_cast<const GLint *>(attribute) + offset);
              break;
            }
          break;
        case GL_UNSIGNED_INT:
          switch (numcomp)
            {
            case 3: glColor3uiv(static_cast<const GLuint *>(attribute) + offset);
              break;
            case 4: glColor4uiv(static_cast<const GLuint *>(attribute) + offset);
              break;
            }
          break;
        case GL_FLOAT:
          switch (numcomp)
            {
            case 3: glColor3fv(static_cast<const GLfloat *>(attribute) + offset);
              break;
            case 4: glColor4fv(static_cast<const GLfloat *>(attribute) + offset);
              break;
            }
          break;
        case GL_DOUBLE:
          switch (numcomp)
            {
            case 3: glColor3dv(static_cast<const GLdouble *>(attribute) + offset);
              break;
            case 4: glColor4dv(static_cast<const GLdouble *>(attribute) + offset);
              break;
            }
          break;
        default:
          vtkErrorMacro("Unsupported type for colors: " << type);
          return;
        }
      break;

  case vtkDataSetAttributes::TCOORDS:     // Texture Coordinate
      if ((numcomp < 1) || (numcomp > 4))
        {
        vtkErrorMacro("Bad number of components.");
        return;
        }
      switch (VTK2SignedOpenGLType(type))
        {
        case GL_SHORT:
          switch (numcomp)
            {
            case 1:
              glTexCoord1sv(static_cast<const GLshort *>(attribute) + offset);
              break;
            case 2:
              glTexCoord2sv(static_cast<const GLshort *>(attribute) + offset);
              break;
            case 3:
              glTexCoord3sv(static_cast<const GLshort *>(attribute) + offset);
              break;
            case 4:
              glTexCoord4sv(static_cast<const GLshort *>(attribute) + offset);
              break;
            }
          break;
        case GL_INT:
          switch (numcomp)
            {
            case 1:
              glTexCoord1iv(static_cast<const GLint *>(attribute) + offset);
              break;
            case 2:
              glTexCoord2iv(static_cast<const GLint *>(attribute) + offset);
              break;
            case 3:
              glTexCoord3iv(static_cast<const GLint *>(attribute) + offset);
              break;
            case 4:
              glTexCoord4iv(static_cast<const GLint *>(attribute) + offset);
              break;
            }
          break;
        case GL_FLOAT:
          switch (numcomp)
            {
            case 1:
              glTexCoord1fv(static_cast<const GLfloat *>(attribute) + offset);
              break;
            case 2:
              glTexCoord2fv(static_cast<const GLfloat *>(attribute) + offset);
              break;
            case 3:
              glTexCoord3fv(static_cast<const GLfloat *>(attribute) + offset);
              break;
            case 4:
              glTexCoord4fv(static_cast<const GLfloat *>(attribute) + offset);
              break;
            }
          break;
        case GL_DOUBLE:
          switch (numcomp)
            {
            case 1:
              glTexCoord1dv(static_cast<const GLdouble *>(attribute) + offset);
              break;
            case 2:
              glTexCoord2dv(static_cast<const GLdouble *>(attribute) + offset);
              break;
            case 3:
              glTexCoord3dv(static_cast<const GLdouble *>(attribute) + offset);
              break;
            case 4:
              glTexCoord4dv(static_cast<const GLdouble *>(attribute) + offset);
              break;
            }
          break;
        default:
          vtkErrorMacro("Unsupported type for texture coordinates: " << type);
          return;
        }
      break;
    case vtkDataSetAttributes::EDGEFLAG:        // Edge Flag
      if (numcomp != 1)
        {
        vtkErrorMacro("Bad number of components.");
        return;
        }
       switch (type)
        {
        vtkTemplateMacro(glEdgeFlag(static_cast<GLboolean>(
                          reinterpret_cast<const VTK_TT*>(attribute)[offset])));
        }
      break;
    default:
      vtkErrorMacro("Unsupported attribute index: " << index);
      return;
    };
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::SendMultiTextureCoords(int numcomp,
  int type, const void *attribute, int idx, vtkIdType offset)
{
  if(! vtkgl::MultiTexCoord2d)
    {
    vtkErrorMacro("MultiTexturing not supported.");
    return;
    }

  if ((numcomp < 1) || (numcomp > 4))
    {
    vtkErrorMacro("Bad number of components.");
    return;
    }

  int textureIndex = vtkgl::TEXTURE0 + idx;
  switch (VTK2SignedOpenGLType(type))
    {
    case GL_SHORT:
      switch (numcomp)
        {
        case 1:
          vtkgl::MultiTexCoord1sv(textureIndex, static_cast<const GLshort *>(attribute) + offset);
          break;
        case 2:
          vtkgl::MultiTexCoord2sv(textureIndex, static_cast<const GLshort *>(attribute) + offset);
          break;
        case 3:
          vtkgl::MultiTexCoord3sv(textureIndex, static_cast<const GLshort *>(attribute) + offset);
          break;
        case 4:
          vtkgl::MultiTexCoord4sv(textureIndex, static_cast<const GLshort *>(attribute) + offset);
          break;
        }
      break;
    case GL_INT:
      switch (numcomp)
        {
        case 1:
          vtkgl::MultiTexCoord1iv(textureIndex, static_cast<const GLint *>(attribute) + offset);
          break;
        case 2:
          vtkgl::MultiTexCoord2iv(textureIndex, static_cast<const GLint *>(attribute) + offset);
          break;
        case 3:
          vtkgl::MultiTexCoord3iv(textureIndex, static_cast<const GLint *>(attribute) + offset);
          break;
        case 4:
          vtkgl::MultiTexCoord4iv(textureIndex, static_cast<const GLint *>(attribute) + offset);
          break;
        }
      break;
    case GL_FLOAT:
      switch (numcomp)
        {
        case 1:
          vtkgl::MultiTexCoord1fv(textureIndex, static_cast<const GLfloat *>(attribute) + offset);
          break;
        case 2:
          vtkgl::MultiTexCoord2fv(textureIndex, static_cast<const GLfloat *>(attribute) + offset);
          break;
        case 3:
          vtkgl::MultiTexCoord3fv(textureIndex, static_cast<const GLfloat *>(attribute) + offset);
          break;
        case 4:
          vtkgl::MultiTexCoord4fv(textureIndex, static_cast<const GLfloat *>(attribute) + offset);
          break;
        }
      break;
    case GL_DOUBLE:
      switch (numcomp)
        {
        case 1:
          vtkgl::MultiTexCoord1dv(textureIndex, static_cast<const GLdouble *>(attribute) + offset);
          break;
        case 2:
          vtkgl::MultiTexCoord2dv(textureIndex, static_cast<const GLdouble *>(attribute) + offset);
          break;
        case 3:
          vtkgl::MultiTexCoord3dv(textureIndex, static_cast<const GLdouble *>(attribute) + offset);
          break;
        case 4:
          vtkgl::MultiTexCoord4dv(textureIndex, static_cast<const GLdouble *>(attribute) + offset);
          break;
        }
      break;
    default:
      vtkErrorMacro("Unsupported type for texture coordinates: " << type);
      return;
    }

}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::SendMaterialProperties(int components,
                                                           int type,
                                                           const void *ambient,
                                                           const void *diffuse,
                                                           const void *specular,
                                                           const void *specular_power)
{
  this->SendMaterialPropertiesForFace(GL_FRONT_AND_BACK,
                                      components,
                                      type,
                                      ambient,
                                      diffuse,
                                      specular,
                                      specular_power);
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::SendMaterialPropertiesForFace(unsigned int face,
                                                                  int components,
                                                                  int type,
                                                                  const void *ambient,
                                                                  const void *diffuse,
                                                                  const void *specular,
                                                                  const void *specular_power)
{
  if(!(components == 3 || components == 4))
    {
    vtkErrorMacro("Bad number of components.");
    return;
    }

  switch(VTK2OpenGLType(type))
    {
    case GL_FLOAT:
      {
      if(components == 3)
        {
        const GLfloat ambient4[] = {
          static_cast<const GLfloat *>(ambient)[0],
          static_cast<const GLfloat *>(ambient)[1],
          static_cast<const GLfloat *>(ambient)[2],
          static_cast<GLfloat>(1)
        };
        glMaterialfv(face, GL_AMBIENT, ambient4);

        const GLfloat diffuse4[] = {
          static_cast<const GLfloat *>(diffuse)[0],
          static_cast<const GLfloat *>(diffuse)[1],
          static_cast<const GLfloat *>(diffuse)[2],
          static_cast<GLfloat>(1)
        };
        glMaterialfv(face, GL_DIFFUSE, diffuse4);

        const GLfloat specular4[] = {
          static_cast<const GLfloat *>(specular)[0],
          static_cast<const GLfloat *>(specular)[1],
          static_cast<const GLfloat *>(specular)[2],
          static_cast<GLfloat>(1)
        };
        glMaterialfv(face, GL_SPECULAR, specular4);
        }
      else if(components == 4)
        {
        glMaterialfv(face, GL_AMBIENT, static_cast<const GLfloat *>(ambient));
        glMaterialfv(face, GL_DIFFUSE, static_cast<const GLfloat *>(diffuse));
        glMaterialfv(face, GL_SPECULAR, static_cast<const GLfloat *>(specular));
        }

      glMaterialfv(face, GL_SHININESS, static_cast<const GLfloat *>(specular_power));
      }
      break;
    case GL_DOUBLE:
      {
        GLfloat ambient_alpha = 1.0f;
        GLfloat diffuse_alpha = 1.0f;
        GLfloat specular_alpha = 1.0f;

        if(components == 4)
          {
          ambient_alpha =
            static_cast<GLfloat>(static_cast<const double *>(ambient)[3]);
          diffuse_alpha =
            static_cast<GLfloat>(static_cast<const double *>(ambient)[3]);
          specular_alpha =
            static_cast<GLfloat>(static_cast<const double *>(ambient)[3]);
          }

        const GLfloat ambient4[] = {
          static_cast<GLfloat>(static_cast<const double *>(ambient)[0]),
          static_cast<GLfloat>(static_cast<const double *>(ambient)[1]),
          static_cast<GLfloat>(static_cast<const double *>(ambient)[2]),
          ambient_alpha
        };
        glMaterialfv(face, GL_AMBIENT, ambient4);

        const GLfloat diffuse4[] = {
          static_cast<GLfloat>(static_cast<const double *>(diffuse)[0]),
          static_cast<GLfloat>(static_cast<const double *>(diffuse)[1]),
          static_cast<GLfloat>(static_cast<const double *>(diffuse)[2]),
          diffuse_alpha
        };
        glMaterialfv(face, GL_AMBIENT, diffuse4);

        const GLfloat specular4[] = {
          static_cast<GLfloat>(static_cast<const double *>(specular)[0]),
          static_cast<GLfloat>(static_cast<const double *>(specular)[1]),
          static_cast<GLfloat>(static_cast<const double *>(specular)[2]),
          specular_alpha
        };
        glMaterialfv(face, GL_AMBIENT, specular4);

        GLfloat specular_power_float =
          static_cast<GLfloat>(static_cast<const double *>(specular_power)[0]
        );
        glMaterialfv(face, GL_SHININESS, &specular_power_float);
      }
      break;
    default:
      vtkErrorMacro("Unsupported type for material properties: " << type);
    }
}

//-----------------------------------------------------------------------------

void vtkOpenGLPainterDeviceAdapter::SetAttributePointer(int index,
              int numcomponents,
              int type, int stride,
              const void *pointer)
{
  GLenum gltype;

  switch (index)
    {
  case vtkDataSetAttributes::NUM_ATTRIBUTES:     // Vertex
      gltype = VTK2SignedOpenGLType(type);
      switch (gltype)
        {
        case GL_SHORT:
        case GL_INT:
        case GL_FLOAT:
        case GL_DOUBLE:
          break;
        default:
          vtkErrorMacro("Unsupported type for vertices: " << type);
          return;
        }
      glVertexPointer(numcomponents, gltype, stride, pointer);
      break;
  case vtkDataSetAttributes::NORMALS:     // Normal
      gltype = VTK2SignedOpenGLType(type);
      switch (gltype)
        {
        case GL_BYTE:
        case GL_SHORT:
        case GL_INT:
        case GL_FLOAT:
        case GL_DOUBLE:
          break;
        default:
          vtkErrorMacro("Unsupported type for normals: " << type);
          return;
        }
      if (numcomponents != 3)
        {
        vtkErrorMacro("Unsupported number of components for normals.");
        return;
        }
      glNormalPointer(gltype, stride, pointer);
      break;
  case vtkDataSetAttributes::SCALARS:     // Color
      gltype = VTK2OpenGLType(type);
      switch (gltype)
        {
        case GL_BYTE:
        case GL_UNSIGNED_BYTE:
        case GL_SHORT:
        case GL_UNSIGNED_SHORT:
        case GL_INT:
        case GL_UNSIGNED_INT:
        case GL_FLOAT:
        case GL_DOUBLE:
          break;
        default:
          vtkErrorMacro("Unsupported type for colors: " << type);
          return;
        }
      glColorPointer(numcomponents, gltype, stride, pointer);
      break;
  case vtkDataSetAttributes::TCOORDS:     // Texture Coordinate
      gltype = VTK2SignedOpenGLType(type);
      switch (gltype)
        {
        case GL_SHORT:
        case GL_INT:
        case GL_FLOAT:
        case GL_DOUBLE:
          break;
        default:
          vtkErrorMacro("Unsupported type for texture coordinates: " << type);
          return;
        }
      glTexCoordPointer(numcomponents, gltype, stride, pointer);
      break;
    case vtkDataSetAttributes::EDGEFLAG:        // Edge flag
      if (numcomponents != 1)
        {
        vtkErrorMacro("Edge flag must have one component.");
        return;
        }
      // Flag must be conformant to GLboolean
      if ((type == VTK_FLOAT) || (type == GL_DOUBLE))
        {
        vtkErrorMacro("Unsupported type for edge flag: " << type);
        return;
        }
      // Thus is an unfriendly way to force the array to be conformant to
      // a GLboolean array.  At the very least there should be some indication
      // in VTK outside of OpenGL to determine which VTK type to use.
      switch (type)
        {
        vtkTemplateMacro(if (sizeof(VTK_TT) != sizeof(GLboolean))
                           {
                           vtkErrorMacro(<< "Unsupported tyep for edge flag: "
                                         << type);
                           return;
                           }
                         );
        }
      glEdgeFlagPointer(stride, pointer);
      break;
    default:
      vtkErrorMacro("Unsupported attribute index: " << index);
      return;
    };
}

//-----------------------------------------------------------------------------

void vtkOpenGLPainterDeviceAdapter::EnableAttributeArray(int index)
{
  switch (index)
    {
  case vtkDataSetAttributes::NUM_ATTRIBUTES:
      glEnableClientState(GL_VERTEX_ARRAY);  break;
  case vtkDataSetAttributes::NORMALS:
      glEnableClientState(GL_NORMAL_ARRAY);  break;
  case vtkDataSetAttributes::SCALARS:
      glEnableClientState(GL_COLOR_ARRAY);  break;
  case vtkDataSetAttributes::TCOORDS:
      glEnableClientState(GL_TEXTURE_COORD_ARRAY);  break;
  case vtkDataSetAttributes::EDGEFLAG:
      glEnableClientState(GL_EDGE_FLAG_ARRAY);  break;
    default:
      vtkErrorMacro("Unsupported attribute index: " << index);
      return;
    };
}

void vtkOpenGLPainterDeviceAdapter::DisableAttributeArray(int index)
{
  switch (index)
    {
  case vtkDataSetAttributes::NUM_ATTRIBUTES:
      glDisableClientState(GL_VERTEX_ARRAY);  break;
  case vtkDataSetAttributes::NORMALS:
      glDisableClientState(GL_NORMAL_ARRAY);  break;
  case vtkDataSetAttributes::SCALARS:
      glDisableClientState(GL_COLOR_ARRAY);  break;
  case vtkDataSetAttributes::TCOORDS:
      glDisableClientState(GL_TEXTURE_COORD_ARRAY);  break;
  case vtkDataSetAttributes::EDGEFLAG:
      glDisableClientState(GL_EDGE_FLAG_ARRAY);  break;
    default:
      vtkErrorMacro("Unsupported attribute index: " << index);
      return;
    };
}

//-----------------------------------------------------------------------------

void vtkOpenGLPainterDeviceAdapter::DrawArrays(int mode, vtkIdType first,
                                               vtkIdType count)
{
  glDrawArrays(VTK2OpenGLPrimitive[mode],static_cast<GLint>(first),
               static_cast<GLsizei>(count));
}

void vtkOpenGLPainterDeviceAdapter::DrawElements(int mode, vtkIdType count,
                                                 int type, void *indices)
{
  GLenum gltype = VTK2UnsignedOpenGLType(type);
  switch (gltype)
    {
    case GL_UNSIGNED_BYTE:
    case GL_UNSIGNED_SHORT:
    case GL_UNSIGNED_INT:
      break;
    default:
      if (type == VTK_ID_TYPE)
        {
        // This seems really inefficient for handling vtkIdType when they
        // are 64 bit, but OpenGL does not handle 64 bit indices.  What
        // else can I do?
        vtkIdType *oldarray = static_cast<vtkIdType *>(indices);
        GLuint *newarray = new GLuint[count];
        std::copy(oldarray, oldarray + count, newarray);
        glDrawElements(VTK2OpenGLPrimitive[mode], static_cast<GLsizei>(count),
                       GL_UNSIGNED_INT, newarray);
        delete[] newarray;
        return;
        }
      else
        {
        vtkErrorMacro("Invalid type for indices.");
        return;
        }
    }
  glDrawElements(VTK2OpenGLPrimitive[mode],static_cast<GLsizei>(count), gltype,
                 indices);
}

//-----------------------------------------------------------------------------

int vtkOpenGLPainterDeviceAdapter::Compatible(vtkRenderer *renderer)
{
  return renderer->IsA("vtkOpenGLRenderer");
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::MakeLighting(int mode)
{
  if (mode)
    {
    glEnable(GL_LIGHTING);
    }
  else
    {
    glDisable(GL_LIGHTING);
    }
}

//-----------------------------------------------------------------------------
int vtkOpenGLPainterDeviceAdapter::QueryLighting()
{
  if (glIsEnabled(GL_LIGHTING))
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::MakeMultisampling(int mode)
{
  if (mode)
    {
    glEnable(vtkgl::MULTISAMPLE);
    }
  else
    {
    glDisable(vtkgl::MULTISAMPLE);
    }
}

//-----------------------------------------------------------------------------
int vtkOpenGLPainterDeviceAdapter::QueryMultisampling()
{
  if (glIsEnabled(vtkgl::MULTISAMPLE))
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::MakeBlending(int mode)
{
  if (mode)
    {
    glEnable(GL_BLEND);
    }
  else
    {
    glDisable(GL_BLEND);
    }
}

//-----------------------------------------------------------------------------
int vtkOpenGLPainterDeviceAdapter::QueryBlending()
{
  if (glIsEnabled(GL_BLEND))
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::MakeVertexEmphasis(bool mode)
{
  if (mode)
    {
    float s;
    glGetFloatv(GL_POINT_SIZE, &s);
    this->PointSize = s;
    glPointSize(4.0); //make verts large enough to be sure to overlap cell

    float nf[2];   //put verts just in front of associated cells
    glGetFloatv(GL_DEPTH_RANGE, nf);
    this->RangeNear = nf[0];
    this->RangeFar = nf[1];
    glDepthRange(0.0, nf[1]*0.999999);
    glDepthMask(GL_FALSE); //prevent verts from interfering with each other
    }
  else
    {
    glPointSize(static_cast<GLfloat>(this->PointSize));
    glDepthRange(this->RangeNear, this->RangeFar);
    glDepthMask(GL_TRUE);
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::WriteStencil(vtkIdType value)
{
  if (this->MaxStencil)
    {
    value = value % this->MaxStencil + 1;
    if (value == 1)
      {
      glClearStencil(0); //start over so don't write into some previous area
      }
    glStencilFunc(GL_ALWAYS, static_cast<GLint>(value), this->MaxStencil);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::TestStencil(vtkIdType value)
{
  if (this->MaxStencil)
    {
    value = value % this->MaxStencil + 1;
    glStencilFunc(GL_EQUAL, static_cast<GLint>(value), this->MaxStencil);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLPainterDeviceAdapter::Stencil(int on)
{
  if (on)
    {
    glEnable(GL_STENCIL_TEST);
    GLint stencilbits;
    glGetIntegerv(GL_STENCIL_BITS, &stencilbits);
    this->MaxStencil = (1<<stencilbits)-1;
    }
  else
    {
    glDisable(GL_STENCIL_TEST);
    }
}


