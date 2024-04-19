// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

#include "vtkGeoProjection.h"

#include "vtkObjectFactory.h"

#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "vtk_libproj.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGeoProjection);

static int vtkGeoProjectionNumProj = -1;

//------------------------------------------------------------------------------
class vtkGeoProjection::vtkInternals
{
public:
  const char* GetKeyAt(int index)
  {
    if (static_cast<int>(this->OptionalParameters.size()) > index)
    {
      std::map<std::string, std::string>::iterator iter = this->OptionalParameters.begin();
      int nbIter = index;
      while (nbIter > 0)
      {
        --nbIter;
        ++iter;
      }
      return iter->first.c_str();
    }
    return nullptr;
  }

  const char* GetValueAt(int index)
  {
    if (static_cast<int>(this->OptionalParameters.size()) > index)
    {
      std::map<std::string, std::string>::iterator iter = this->OptionalParameters.begin();
      int nbIter = index;
      while (nbIter > 0)
      {
        --nbIter;
        ++iter;
      }
      return iter->second.c_str();
    }
    return nullptr;
  }

  std::map<std::string, std::string> OptionalParameters;
  PJ_PROJ_INFO ProjInfo;
};

//------------------------------------------------------------------------------
int vtkGeoProjection::GetNumberOfProjections()
{
  if (vtkGeoProjectionNumProj < 0)
  {
    vtkGeoProjectionNumProj = 0;
    for (const PJ_LIST* pj = proj_list_operations(); pj && pj->id; ++pj)
      ++vtkGeoProjectionNumProj;
  }
  return vtkGeoProjectionNumProj;
}
//------------------------------------------------------------------------------
const char* vtkGeoProjection::GetProjectionName(int projection)
{
  if (projection < 0 || projection >= vtkGeoProjection::GetNumberOfProjections())
    return nullptr;

  return proj_list_operations()[projection].id;
}
//------------------------------------------------------------------------------
const char* vtkGeoProjection::GetProjectionDescription(int projection)
{
  if (projection < 0 || projection >= vtkGeoProjection::GetNumberOfProjections())
    return nullptr;

  return proj_list_operations()[projection].descr[0];
}
//------------------------------------------------------------------------------
vtkGeoProjection::vtkGeoProjection()
{
  this->Name = nullptr;
  this->SetName("latlong");
  this->CentralMeridian = 0.;
  this->Projection = nullptr;
  this->ProjectionMTime = 0;
  this->PROJ4String = nullptr;
  this->SetPROJ4String("");
  this->Internals = new vtkInternals();
}
//------------------------------------------------------------------------------
vtkGeoProjection::~vtkGeoProjection()
{
  this->SetName(nullptr);
  this->SetPROJ4String(nullptr);
  if (this->Projection)
  {
    proj_destroy(this->Projection);
  }
  delete this->Internals;
  this->Internals = nullptr;
}
//------------------------------------------------------------------------------
void vtkGeoProjection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Name: " << this->Name << "\n";
  os << indent << "CentralMeridian: " << this->CentralMeridian << "\n";
  os << indent << "PROJ4String: " << this->PROJ4String << "\n";
  os << indent << "Projection: " << this->Projection << "\n";
  os << indent << "Optional parameters:\n";
  for (int i = 0; i < this->GetNumberOfOptionalParameters(); i++)
  {
    os << indent << " - " << this->GetOptionalParameterKey(i) << " = "
       << this->GetOptionalParameterValue(i) << "\n";
  }
}
//------------------------------------------------------------------------------
int vtkGeoProjection::GetIndex()
{
  int i = 0;
  for (const PJ_LIST* proj = proj_list_operations(); proj && proj->id; ++proj, ++i)
  {
    if (!strcmp(proj->id, this->Name))
    {
      return i;
    }
  }
  return -1;
}
//------------------------------------------------------------------------------
const char* vtkGeoProjection::GetDescription()
{
  this->UpdateProjection();
  if (!this->Projection)
  {
    return nullptr;
  }
  return this->Internals->ProjInfo.description;
}
//------------------------------------------------------------------------------
projPJ vtkGeoProjection::GetProjection()
{
  this->UpdateProjection();
  return this->Projection;
}

//------------------------------------------------------------------------------
int vtkGeoProjection::UpdateProjection()
{
  if (this->GetMTime() <= this->ProjectionMTime)
  {
    return 0;
  }

  if (this->Projection)
  {
    proj_destroy(this->Projection);
    this->Projection = nullptr;
  }

  if (this->PROJ4String && strlen(this->PROJ4String))
  {
    this->Projection = proj_create(PJ_DEFAULT_CTX, this->PROJ4String);
    if (!this->Projection)
    {
      vtkErrorMacro("Cannot set projection with string " << this->PROJ4String);
    }
  }
  else
  {
    if (!this->Name || !strlen(this->Name))
    {
      return 1;
    }

    if (!strcmp(this->Name, "latlong"))
    {
      // latlong is "null" projection.
      return 0;
    }

    int argSize = 3 + this->GetNumberOfOptionalParameters();
    const char** pjArgs = new const char*[argSize];
    std::string projSpec("+proj=");
    projSpec += this->Name;
    std::string ellpsSpec("+ellps=clrk66");
    std::string meridSpec;
    std::ostringstream os;
    os << "+lon_0=" << this->CentralMeridian;
    meridSpec = os.str();
    pjArgs[0] = projSpec.c_str();
    pjArgs[1] = ellpsSpec.c_str();
    pjArgs[2] = meridSpec.c_str();

    // Add optional parameters
    std::vector<std::string> stringHolder(
      this->GetNumberOfOptionalParameters()); // Keep string ref in memory
    for (int i = 0; i < this->GetNumberOfOptionalParameters(); i++)
    {
      std::ostringstream param;
      param << "+" << this->GetOptionalParameterKey(i);
      param << "=" << this->GetOptionalParameterValue(i);
      stringHolder[i] = param.str();
      pjArgs[3 + i] = stringHolder[i].c_str();
    }
    this->Projection = proj_create_argv(PJ_DEFAULT_CTX, argSize, const_cast<char**>(pjArgs));
    delete[] pjArgs;
  }
  this->ProjectionMTime = this->GetMTime();
  if (this->Projection)
  {
    this->Internals->ProjInfo = proj_pj_info(this->Projection);
    return 0;
  }
  return 1;
}

//------------------------------------------------------------------------------
void vtkGeoProjection::SetOptionalParameter(const char* key, const char* value)
{
  if (key != nullptr && value != nullptr)
  {
    this->Internals->OptionalParameters[key] = value;
    this->Modified();
  }
  else
  {
    vtkErrorMacro("Invalid Optional Parameter Key/Value pair. None can be null");
  }
}

//------------------------------------------------------------------------------
void vtkGeoProjection::RemoveOptionalParameter(const char* key)
{
  this->Internals->OptionalParameters.erase(key);
  this->Modified();
}
//------------------------------------------------------------------------------
int vtkGeoProjection::GetNumberOfOptionalParameters()
{
  return static_cast<int>(this->Internals->OptionalParameters.size());
}
//------------------------------------------------------------------------------
const char* vtkGeoProjection::GetOptionalParameterKey(int index)
{
  return this->Internals->GetKeyAt(index);
}
//------------------------------------------------------------------------------
const char* vtkGeoProjection::GetOptionalParameterValue(int index)
{
  return this->Internals->GetValueAt(index);
}
//------------------------------------------------------------------------------
void vtkGeoProjection::ClearOptionalParameters()
{
  this->Internals->OptionalParameters.clear();
  this->Modified();
}
VTK_ABI_NAMESPACE_END
