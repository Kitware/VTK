// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOpenGLShaderDeclaration
 * @brief
 *
 */

#ifndef vtkOpenGLShaderDeclaration_h
#define vtkOpenGLShaderDeclaration_h

#include "vtkRenderingOpenGL2Module.h"
#include "vtkStringToken.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKRENDERINGOPENGL2_EXPORT vtkOpenGLShaderDeclaration
{
public:
  enum class GLSLAttributeType
  {
    Mat3,
    Mat4,
    SamplerBuffer,
    SamplerCube,
    Sampler2D,
    Sampler1D,
    Scalar,
    Vec2,
    Vec3,
    Vec4,
  };

  enum class GLSLDataType
  {
    Unsigned,
    Integer,
    Float,
  };

  enum class GLSLPrecisionType
  {
    Low,
    Medium,
    High,
    None,
  };

  enum class GLSLQualifierType
  {
    Uniform,
    In,
    Out
  };
  GLSLQualifierType QualifierType;
  GLSLPrecisionType PrecisionType;
  GLSLDataType DataType;
  GLSLAttributeType AttributeType;
  vtkStringToken VariableName;
  vtkOpenGLShaderDeclaration() = default;
  vtkOpenGLShaderDeclaration(GLSLQualifierType qual, GLSLPrecisionType prec, GLSLDataType dtype,
    GLSLAttributeType attr, vtkStringToken varName)
    : QualifierType(qual)
    , PrecisionType(prec)
    , DataType(dtype)
    , AttributeType(attr)
    , VariableName(varName)
  {
  }

  friend std::ostream& operator<<(std::ostream& os, const vtkOpenGLShaderDeclaration& decl)
  {
    const std::string space = " ";
    switch (decl.QualifierType)
    {
      case GLSLQualifierType::Uniform:
        os << "uniform";
        break;
      case GLSLQualifierType::In:
        os << "in";
        break;
      case GLSLQualifierType::Out:
      default:
        os << "out";
        break;
    }

    switch (decl.PrecisionType)
    {
      case GLSLPrecisionType::Low:
        os << space;
        os << "lowp";
        break;
      case GLSLPrecisionType::Medium:
        os << space;
        os << "mediump";
        break;
      case GLSLPrecisionType::High:
        os << space;
        os << "highp";
        break;
      case GLSLPrecisionType::None:
      default:
        break;
    }

    if (decl.AttributeType != GLSLAttributeType::Scalar)
    {
      switch (decl.DataType)
      {
        case GLSLDataType::Unsigned:
          os << space;
          os << "u";
          break;
        case GLSLDataType::Integer:
          os << space;
          os << "i";
          break;
        case GLSLDataType::Float:
        default:
          os << space;
          break;
      }
    }

    switch (decl.AttributeType)
    {
      case GLSLAttributeType::Mat3:
        os << "mat3";
        break;
      case GLSLAttributeType::Mat4:
        os << "mat4";
        break;
      case GLSLAttributeType::SamplerBuffer:
        os << "samplerBuffer";
        break;
      case GLSLAttributeType::SamplerCube:
        os << "samplerCube";
        break;
      case GLSLAttributeType::Sampler2D:
        os << "sampler2D";
        break;
      case GLSLAttributeType::Sampler1D:
        os << "sampler1D";
        break;
      case GLSLAttributeType::Scalar:
        switch (decl.DataType)
        {
          case GLSLDataType::Unsigned:
            os << space;
            os << "uint";
            break;
          case GLSLDataType::Integer:
            os << space;
            os << "int";
            break;
          case GLSLDataType::Float:
          default:
            os << space;
            os << "float";
            break;
        }
        break;
      case GLSLAttributeType::Vec2:
        os << "vec2";
        break;
      case GLSLAttributeType::Vec3:
        os << "vec3";
        break;
      case GLSLAttributeType::Vec4:
      default:
        os << "vec4";
        break;
    }
    os << space << decl.VariableName.Data() << ";";
    return os;
  }
};

VTK_ABI_NAMESPACE_END
#endif // vtkOpenGLShaderDeclaration_h
