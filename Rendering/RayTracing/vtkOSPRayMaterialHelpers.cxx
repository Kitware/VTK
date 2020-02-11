/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOSPRayMaterialHelpers.cpp

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOSPRayMaterialHelpers.h"
#include "vtkImageData.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkProperty.h"
#include "vtkTexture.h"

#include "RTWrapper/RTWrapper.h"

#include <limits>

//------------------------------------------------------------------------------
OSPTexture vtkOSPRayMaterialHelpers::NewTexture2D(RTW::Backend* backend, const osp::vec2i& size,
  const OSPTextureFormat type, void* data, const uint32_t _flags, size_t sizeOf)
{
  auto texture = ospNewTexture("texture2d");
  if (texture == nullptr)
    return nullptr;

  auto flags = _flags; // because the input value is declared const, use a copy

  bool sharedBuffer = flags & OSP_TEXTURE_SHARED_BUFFER;

  flags &= ~OSP_TEXTURE_SHARED_BUFFER;

  const auto texelBytes = sizeOf;
  const auto totalTexels = size.x * size.y;
  const auto totalBytes = totalTexels * texelBytes;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
  auto data_handle =
    ospNewData(totalBytes, OSP_RAW, data, sharedBuffer ? OSP_DATA_SHARED_BUFFER : 0);
#pragma GCC diagnostic pop

  ospCommit(data_handle);
  ospSetObject(texture, "data", data_handle);
  ospRelease(data_handle);

  ospSet1i(texture, "type", static_cast<int>(type));
  ospSet1i(texture, "flags", static_cast<int>(flags));
  ospSet2i(texture, "size", size.x, size.y);
  ospCommit(texture);

  return texture;
}

//------------------------------------------------------------------------------
OSPTexture vtkOSPRayMaterialHelpers::VTKToOSPTexture(
  RTW::Backend* backend, vtkImageData* vColorTextureMap)
{
  if (backend == nullptr)
  {
    return OSPTexture2D();
  }

  int xsize = vColorTextureMap->GetExtent()[1];
  int ysize = vColorTextureMap->GetExtent()[3];
  int scalartype = vColorTextureMap->GetScalarType();
  int comps = vColorTextureMap->GetNumberOfScalarComponents();

  OSPTexture t2d = nullptr;

  if (scalartype == VTK_UNSIGNED_CHAR || scalartype == VTK_CHAR || scalartype == VTK_SIGNED_CHAR)
  {
    OSPTextureFormat format[4] = { OSP_TEXTURE_R8, OSP_TEXTURE_RGB8, OSP_TEXTURE_RGB8,
      OSP_TEXTURE_RGBA8 };
    std::vector<unsigned char> chars;

    if (comps == 2 || comps > 4)
    {
      // no native formats, we need to copy the components to a 3 channels texture
      chars.resize((xsize + 1) * (ysize + 1) * 3, 0);
      unsigned char* oc = chars.data();
      unsigned char* ptr =
        reinterpret_cast<unsigned char*>(vColorTextureMap->GetScalarPointer(0, 0, 0));
      for (int i = 0; i <= xsize; ++i)
      {
        for (int j = 0; j <= ysize; ++j)
        {
          for (int k = 0; k < comps && k < 3; k++)
          {
            oc[k] = ptr[k];
          }
          ptr += comps;
          oc += 3;
        }
      }
      comps = 3;
    }

    t2d = vtkOSPRayMaterialHelpers::NewTexture2D(backend, osp::vec2i{ xsize + 1, ysize + 1 },
      format[comps - 1], chars.size() > 0 ? chars.data() : vColorTextureMap->GetScalarPointer(),
      OSP_TEXTURE_FILTER_NEAREST, sizeof(char) * comps);
  }
  else if (scalartype == VTK_FLOAT)
  {
    OSPTextureFormat format[4] = { OSP_TEXTURE_R32F, OSP_TEXTURE_RGB32F, OSP_TEXTURE_RGB32F,
      OSP_TEXTURE_RGBA32F };
    std::vector<float> floats;
    if (comps == 2 || comps > 4)
    {
      // no native formats, we need to copy the components to a 3 channels texture
      floats.resize((xsize + 1) * (ysize + 1) * 3, 0);
      float* of = floats.data();
      for (int i = 0; i <= ysize; ++i)
      {
        for (int j = 0; j <= xsize; ++j)
        {
          for (int k = 0; k < comps && k < 3; k++)
          {
            of[k] = vColorTextureMap->GetScalarComponentAsFloat(j, i, 0, k);
          }
          of += 3;
        }
      }
      comps = 3;
    }
    t2d = vtkOSPRayMaterialHelpers::NewTexture2D(backend, osp::vec2i{ xsize + 1, ysize + 1 },
      format[comps - 1], floats.size() > 0 ? floats.data() : vColorTextureMap->GetScalarPointer(),
      OSP_TEXTURE_FILTER_NEAREST, sizeof(float) * comps);
  }
  else
  {
    // All other types are converted to float
    int newComps = comps;
    OSPTextureFormat format[4] = { OSP_TEXTURE_R32F, OSP_TEXTURE_RGB32F, OSP_TEXTURE_RGB32F,
      OSP_TEXTURE_RGBA32F };

    if (comps == 2 || comps > 4)
    {
      newComps = 3;
    }

    float multiplier = 1.f;
    float shift = 0.f;

    // 16-bits integer are not supported yet in OSPRay
    switch (scalartype)
    {
      case VTK_SHORT:
        shift += std::numeric_limits<short>::min();
        multiplier /= std::numeric_limits<unsigned short>::max();
        break;
      case VTK_UNSIGNED_SHORT:
        multiplier /= std::numeric_limits<unsigned short>::max();
        break;
      default:
        break;
    }

    std::vector<float> floats;
    floats.resize((xsize + 1) * (ysize + 1) * newComps, 0);
    float* of = floats.data();
    for (int i = 0; i <= ysize; ++i)
    {
      for (int j = 0; j <= xsize; ++j)
      {
        for (int k = 0; k < newComps && k < comps; k++)
        {
          of[k] = (vColorTextureMap->GetScalarComponentAsFloat(j, i, 0, k) + shift) * multiplier;
        }
        of += newComps;
      }
    }
    t2d = vtkOSPRayMaterialHelpers::NewTexture2D(backend, osp::vec2i{ xsize + 1, ysize + 1 },
      format[newComps - 1], floats.data(), OSP_TEXTURE_FILTER_NEAREST, sizeof(float) * newComps);
  }

  if (t2d != nullptr)
  {
    ospCommit(t2d);
  }

  return t2d;
}

//------------------------------------------------------------------------------
void vtkOSPRayMaterialHelpers::MakeMaterials(
  vtkOSPRayRendererNode* orn, OSPRenderer oRenderer, std::map<std::string, OSPMaterial>& mats)
{
  vtkOSPRayMaterialLibrary* ml = vtkOSPRayRendererNode::GetMaterialLibrary(orn->GetRenderer());
  if (!ml)
  {
    cout << "No material Library in this renderer." << endl;
    return;
  }
  std::set<std::string> nicknames = ml->GetMaterialNames();
  std::set<std::string>::iterator it = nicknames.begin();
  while (it != nicknames.end())
  {
    OSPMaterial newmat = vtkOSPRayMaterialHelpers::MakeMaterial(orn, oRenderer, *it);
    mats[*it] = newmat;
    ++it;
  }
}

//------------------------------------------------------------------------------
OSPMaterial vtkOSPRayMaterialHelpers::MakeMaterial(
  vtkOSPRayRendererNode* orn, OSPRenderer oRenderer, std::string nickname)
{
  RTW::Backend* backend = orn->GetBackend();
  OSPMaterial oMaterial;
  vtkOSPRayMaterialLibrary* ml = vtkOSPRayRendererNode::GetMaterialLibrary(orn->GetRenderer());
  if (!ml)
  {
    vtkGenericWarningMacro("No material Library in this renderer. Using OBJMaterial by default.");
    return NewMaterial(orn, oRenderer, "OBJMaterial");
  }

  const auto& dic = vtkOSPRayMaterialLibrary::GetParametersDictionary();

  std::string implname = ml->LookupImplName(nickname);

  if (dic.find(implname) != dic.end())
  {
    oMaterial = NewMaterial(orn, oRenderer, implname);

    const auto& paramList = dic.at(implname);
    for (auto param : paramList)
    {
      switch (param.second)
      {
        case vtkOSPRayMaterialLibrary::ParameterType::BOOLEAN:
        {
          auto values = ml->GetDoubleShaderVariable(nickname, param.first);
          if (values.size() == 1)
          {
            ospSet1i(oMaterial, param.first.c_str(), static_cast<int>(values[0]));
          }
        }
        break;
        case vtkOSPRayMaterialLibrary::ParameterType::FLOAT:
        case vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT:
        {
          auto values = ml->GetDoubleShaderVariable(nickname, param.first);
          if (values.size() == 1)
          {
            ospSet1f(oMaterial, param.first.c_str(), static_cast<float>(values[0]));
          }
        }
        break;
        case vtkOSPRayMaterialLibrary::ParameterType::FLOAT_DATA:
        {
          auto values = ml->GetDoubleShaderVariable(nickname, param.first);
          if (values.size() > 0)
          {
            std::vector<float> fvalues(values.begin(), values.end());
            OSPData data = ospNewData(fvalues.size() / 3, OSP_FLOAT3, fvalues.data());
            ospSetData(oMaterial, param.first.c_str(), data);
          }
        }
        break;
        case vtkOSPRayMaterialLibrary::ParameterType::VEC2:
        {
          auto values = ml->GetDoubleShaderVariable(nickname, param.first);
          if (values.size() == 2)
          {
            std::vector<float> fvalues(values.begin(), values.end());
            ospSet2f(oMaterial, param.first.c_str(), fvalues[0], fvalues[1]);
          }
        }
        break;
        case vtkOSPRayMaterialLibrary::ParameterType::VEC3:
        case vtkOSPRayMaterialLibrary::ParameterType::COLOR_RGB:
        {
          auto values = ml->GetDoubleShaderVariable(nickname, param.first);
          if (values.size() == 3)
          {
            std::vector<float> fvalues(values.begin(), values.end());
            ospSet3fv(oMaterial, param.first.c_str(), fvalues.data());
          }
        }
        break;
        case vtkOSPRayMaterialLibrary::ParameterType::VEC4:
        {
          auto values = ml->GetDoubleShaderVariable(nickname, param.first);
          if (values.size() == 4)
          {
            std::vector<float> fvalues(values.begin(), values.end());
            ospSet4f(
              oMaterial, param.first.c_str(), fvalues[0], fvalues[1], fvalues[2], fvalues[3]);
          }
        }
        break;
        case vtkOSPRayMaterialLibrary::ParameterType::TEXTURE:
        {
          vtkTexture* texname = ml->GetTexture(nickname, param.first);
          if (texname)
          {
            vtkImageData* vColorTextureMap = vtkImageData::SafeDownCast(texname->GetInput());
            OSPTexture t2d = vtkOSPRayMaterialHelpers::VTKToOSPTexture(backend, vColorTextureMap);
            ospSetObject(oMaterial, param.first.c_str(), static_cast<OSPTexture>(t2d));
          }
        }
        break;
        default:
          break;
      }
    }
  }
  else
  {
    vtkGenericWarningMacro(
      "Warning: unrecognized material \"" << implname.c_str() << "\", using a default OBJMaterial");
    return NewMaterial(orn, oRenderer, "OBJMaterial");
  }

  return oMaterial;
}

//------------------------------------------------------------------------------
OSPMaterial vtkOSPRayMaterialHelpers::NewMaterial(
  vtkOSPRayRendererNode* orn, OSPRenderer oRenderer, std::string ospMatName)
{
  RTW::Backend* backend = orn->GetBackend();
  OSPMaterial result = nullptr;

  if (backend == nullptr)
    return result;

  (void)oRenderer;
  const std::string rendererType = vtkOSPRayRendererNode::GetRendererType(orn->GetRenderer());
  result = ospNewMaterial2(rendererType.c_str(), ospMatName.c_str());

  if (!result)
  {
    vtkGenericWarningMacro(
      "OSPRay failed to create material: " << ospMatName << ". Trying OBJMaterial instead.");
    result = ospNewMaterial2(rendererType.c_str(), "OBJMaterial");
  }

  return result;
}
