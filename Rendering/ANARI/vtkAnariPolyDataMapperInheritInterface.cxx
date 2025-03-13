// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAnariPolyDataMapperInheritInterface.h"

#include "vtkProperty.h"

//----------------------------------------------------------------------------
vtkAnariPolyDataMapperInheritInterface::~vtkAnariPolyDataMapperInheritInterface() {}

//----------------------------------------------------------------------------
void vtkAnariPolyDataMapperInheritInterface::SetDevice(
  anari::Device& device, anari::Extensions& extensions, const char* const* anariExtensionStrings)
{
  this->AnariDevice = device;
  this->AnariExtensions = extensions;
  this->AnariExtensionStrings = anariExtensionStrings;
}

//----------------------------------------------------------------------------
int vtkAnariPolyDataMapperInheritInterface::GetSurfaceRepresentation(vtkProperty* property) const
{
  return property->GetRepresentation();
}

//----------------------------------------------------------------------------
vtkAnariPolyDataMapperInheritInterface::ParameterFlags
vtkAnariPolyDataMapperInheritInterface::GetBaseUpdateResponsibility() const
{
  return ParameterFlags();
}

//----------------------------------------------------------------------------
anari::Geometry vtkAnariPolyDataMapperInheritInterface::InitializeSpheres(vtkPolyData*,
  vtkProperty*, std::vector<vec3>&, std::vector<uint32_t>&, double, vtkDataArray*,
  vtkPiecewiseFunction*, std::vector<vec2>&, std::vector<float>&, std::vector<vec4>&, int)
{
  return anari::newObject<anari::Geometry>(this->AnariDevice, "sphere");
}

//----------------------------------------------------------------------------
anari::Geometry vtkAnariPolyDataMapperInheritInterface::InitializeCurves(vtkPolyData*, vtkProperty*,
  std::vector<vec3>&, std::vector<uint32_t>&, double, vtkDataArray*, vtkPiecewiseFunction*,
  std::vector<vec2>&, std::vector<float>&, std::vector<vec4>&, int)
{
  return anari::newObject<anari::Geometry>(this->AnariDevice, "curve");
}

//----------------------------------------------------------------------------
anari::Geometry vtkAnariPolyDataMapperInheritInterface::InitializeCylinders(vtkPolyData*,
  vtkProperty*, std::vector<vec3>&, std::vector<uint32_t>&, double, vtkDataArray*,
  vtkPiecewiseFunction*, std::vector<vec2>&, std::vector<float>&, std::vector<vec4>&, int)
{
  return anari::newObject<anari::Geometry>(this->AnariDevice, "cylinder");
}

//----------------------------------------------------------------------------
anari::Geometry vtkAnariPolyDataMapperInheritInterface::InitializeTriangles(vtkPolyData*,
  vtkProperty*, std::vector<vec3>&, std::vector<uint32_t>&, std::vector<vec3>&, std::vector<vec2>&,
  std::vector<float>&, std::vector<vec4>&, int)
{
  return anari::newObject<anari::Geometry>(this->AnariDevice, "triangle");
}

//----------------------------------------------------------------------------
const char* vtkAnariPolyDataMapperInheritInterface::GetSpheresPostfix() const
{
  return "_spheres_";
}

//----------------------------------------------------------------------------
const char* vtkAnariPolyDataMapperInheritInterface::GetCurvesPostfix() const
{
  return "_curves_";
}

//----------------------------------------------------------------------------
const char* vtkAnariPolyDataMapperInheritInterface::GetCylindersPostfix() const
{
  return "_cylinders_";
}

//----------------------------------------------------------------------------
const char* vtkAnariPolyDataMapperInheritInterface::GetTrianglesPostfix() const
{
  return "_triangles_";
}
