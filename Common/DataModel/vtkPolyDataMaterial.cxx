// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkPolyDataMaterial.h"

#include "vtkDataObject.h"
#include "vtkDoubleArray.h"
#include "vtkFieldData.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkStringArray.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkPolyDataMaterial);

const char* const vtkPolyDataMaterial::TEXTURE_URI = "texture_uri";
const char* const vtkPolyDataMaterial::DIFFUSE_COLOR = "diffuse_color";
const char* const vtkPolyDataMaterial::SPECULAR_COLOR = "specular_color";
const char* const vtkPolyDataMaterial::TRANSPARENCY = "transparency";
const char* const vtkPolyDataMaterial::SHININESS = "shininess";

//------------------------------------------------------------------------------
void vtkPolyDataMaterial::SetField(vtkDataObject* obj, const char* name, const char* value)
{
  vtkFieldData* fd = obj->GetFieldData();
  if (!fd)
  {
    vtkNew<vtkFieldData> newfd;
    obj->SetFieldData(newfd);
  }
  vtkNew<vtkStringArray> sa;
  sa->SetNumberOfTuples(1);
  sa->SetValue(0, value);
  sa->SetName(name);
  fd->AddArray(sa);
}

//------------------------------------------------------------------------------
void vtkPolyDataMaterial::SetField(
  vtkDataObject* obj, const char* name, double* value, vtkIdType components)
{
  vtkFieldData* fd = obj->GetFieldData();
  if (!fd)
  {
    vtkNew<vtkFieldData> newfd;
    obj->SetFieldData(newfd);
  }
  vtkNew<vtkDoubleArray> da;
  da->SetNumberOfComponents(components);
  da->SetNumberOfTuples(1);
  da->SetTypedTuple(0, value);
  da->SetName(name);
  fd->AddArray(da);
}

std::vector<double> vtkPolyDataMaterial::GetField(
  vtkDataObject* obj, const char* name, const std::vector<double>& defaultResult)
{
  std::vector<double> result;
  vtkFieldData* fd = obj->GetFieldData();
  if (fd)
  {
    vtkDoubleArray* da = vtkDoubleArray::SafeDownCast(fd->GetAbstractArray(name));
    if (da)
    {
      result.resize(defaultResult.size());
      da->GetTypedTuple(0, result.data());
    }
    else
    {
      result = defaultResult;
    }
  }
  else
  {
    result = defaultResult;
  }
  return result;
}

std::vector<float> vtkPolyDataMaterial::GetField(
  vtkDataObject* obj, const char* name, const std::vector<float>& defaultResult)
{
  std::vector<float> result;
  std::vector<double> r, d;
  std::transform(defaultResult.begin(), defaultResult.end(), std::back_inserter(d),
    [](float f) { return static_cast<double>(f); });
  r = GetField(obj, name, d);
  std::transform(r.begin(), r.end(), std::back_inserter(result),
    [](double value) { return static_cast<float>(value); });
  return result;
}

std::vector<std::string> vtkPolyDataMaterial::GetField(vtkDataObject* obj, const char* name)
{
  vtkFieldData* fd = obj->GetFieldData();
  std::vector<std::string> result;
  if (!fd)
  {
    return result;
  }
  vtkStringArray* sa = vtkStringArray::SafeDownCast(fd->GetAbstractArray(name));
  if (!sa)
  {
    return result;
  }
  for (int i = 0; i < sa->GetNumberOfTuples(); ++i)
    result.push_back(sa->GetValue(i));
  return result;
}

void vtkPolyDataMaterial::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os, indent);
}

VTK_ABI_NAMESPACE_END
