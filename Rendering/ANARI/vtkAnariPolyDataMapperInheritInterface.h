// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAnariPolyDataMapperInheritInterface
 * @brief   Interface for inheriting classes for overriding mapper functionality
 *
 * Interface for inheriting classes for overriding mapper functionality
 *
 * @par Thanks:
 * Kevin Griffin kgriffin@nvidia.com for creating and contributing the class
 * and NVIDIA for supporting this work.
 */

#ifndef vtkAnariPolyDataMapperInheritInterface_h
#define vtkAnariPolyDataMapperInheritInterface_h

#include <anari/anari_cpp.hpp>
#include <anari/anari_cpp/ext/std.h>

#include <vector>

class vtkDataArray;
class vtkPiecewiseFunction;
class vtkPolyData;
class vtkProperty;

using uvec2 = anari::std_types::uvec2;
using uvec3 = anari::std_types::uvec3;
using vec2 = anari::std_types::vec2;
using vec3 = anari::std_types::vec3;
using vec4 = anari::std_types::vec4;
using mat4 = anari::std_types::mat4;

class vtkAnariPolyDataMapperInheritInterface
{
public:
  virtual ~vtkAnariPolyDataMapperInheritInterface();

  /**
   * Specifies parameter flags, which can be used by inheriting
   * classes to specify which parameter it takes responsibility for.
   */
  struct ParameterFlags
  {
    bool Positions = true;
    bool Indices = true;
    bool Normals = true;
    bool Scales = true;
    bool Texcoords = true;
    bool Colors = true;
  };

  /**
   * Set the ANARI device to write to.
   */
  virtual void SetDevice(
    anari::Device& device, anari::Extensions& extensions, const char* const* anariExtensionStrings);

  /**
   * Inheriting classes can own (override) representation choice,
   * creation of anari geometries and parameter updates thereof.
   */
  virtual int GetSurfaceRepresentation(vtkProperty* property) const;
  virtual ParameterFlags GetBaseUpdateResponsibility() const;

  virtual anari::Geometry InitializeSpheres(vtkPolyData* polyData, vtkProperty* property,
    std::vector<vec3>& vertices, std::vector<uint32_t>& indexArray, double pointSize,
    vtkDataArray* scaleArray, vtkPiecewiseFunction* scaleFunction, std::vector<vec2>& textureCoords,
    std::vector<float>& pointValueTextureCoords, std::vector<vec4>& pointColors, int cellFlag);
  virtual anari::Geometry InitializeCurves(vtkPolyData* polyData, vtkProperty* property,
    std::vector<vec3>& vertices, std::vector<uint32_t>& indexArray, double lineWidth,
    vtkDataArray* scaleArray, vtkPiecewiseFunction* scaleFunction, std::vector<vec2>& textureCoords,
    std::vector<float>& pointValueTextureCoords, std::vector<vec4>& pointColors, int cellFlag);
  virtual anari::Geometry InitializeCylinders(vtkPolyData* polyData, vtkProperty* property,
    std::vector<vec3>& vertices, std::vector<uint32_t>& indexArray, double lineWidth,
    vtkDataArray* scaleArray, vtkPiecewiseFunction* scaleFunction, std::vector<vec2>& textureCoords,
    std::vector<float>& pointValueTextureCoords, std::vector<vec4>& pointColors, int cellFlag);
  virtual anari::Geometry InitializeTriangles(vtkPolyData* polyData, vtkProperty* property,
    std::vector<vec3>& vertices, std::vector<uint32_t>& indexArray, std::vector<vec3>& normals,
    std::vector<vec2>& textureCoords, std::vector<float>& pointValueTextureCoords,
    std::vector<vec4>& pointColors, int cellFlag);

  /**
   * Inheriting classes can also influence the postfix of the created prim names.
   */
  virtual const char* GetSpheresPostfix() const;
  virtual const char* GetCurvesPostfix() const;
  virtual const char* GetCylindersPostfix() const;
  virtual const char* GetTrianglesPostfix() const;

  anari::Device AnariDevice{ nullptr };
  anari::Extensions AnariExtensions{};
  const char* const* AnariExtensionStrings{ nullptr };
};

#endif
