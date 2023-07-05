// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkOSPRayMaterialHelpers.h"
#include "vtkImageData.h"
#include "vtkOSPRayMaterialLibrary.h"
#include "vtkOSPRayRendererNode.h"
#include "vtkProperty.h"
#include "vtkTexture.h"

#include "RTWrapper/RTWrapper.h"

#include <limits>

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
OSPTexture vtkOSPRayMaterialHelpers::NewTexture2D(RTW::Backend* backend, const osp::vec2i& size,
  const OSPTextureFormat type, void* data, const uint32_t _flags)
{
  auto texture = ospNewTexture("texture2d");
  if (texture == nullptr)
  {
    return nullptr;
  }

  auto flags = _flags; // because the input value is declared const, use a copy

  OSPDataType dataType = OSP_UNKNOWN;
  if (type == OSP_TEXTURE_R32F)
  {
    dataType = OSP_FLOAT;
  }
  else if (type == OSP_TEXTURE_RGB32F)
  {
    dataType = OSP_VEC3F;
  }
  else if (type == OSP_TEXTURE_RGBA32F)
  {
    dataType = OSP_VEC4F;
  }
  else if (type == OSP_TEXTURE_R8)
  {
    dataType = OSP_UCHAR;
  }
  else if (type == OSP_TEXTURE_RGB8 || type == OSP_TEXTURE_SRGB)
  {
    dataType = OSP_VEC3UC;
  }
  else if (type == OSP_TEXTURE_RGBA8 || type == OSP_TEXTURE_SRGBA)
  {
    dataType = OSP_VEC4UC;
  }
  else
  {
    throw std::runtime_error("vtkOSPRayMaterialHelpers::NewTexture2D: Unknown texture format");
  }

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#endif
  auto data_handle = ospNewCopyData2D(data, dataType, size.x, size.y);
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  ospCommit(data_handle);
  ospSetObject(texture, "data", data_handle);
  ospRelease(data_handle);

  ospSetInt(texture, "format", static_cast<int>(type));
  if (flags & OSP_TEXTURE_FILTER_NEAREST)
  {
    ospSetInt(texture, "filter", OSP_TEXTURE_FILTER_NEAREST);
  }
  ospCommit(texture);

  return texture;
}

//------------------------------------------------------------------------------
OSPTexture vtkOSPRayMaterialHelpers::VTKToOSPTexture(
  RTW::Backend* backend, vtkImageData* vColorTextureMap, bool isSRGB)
{
  if (backend == nullptr)
  {
    return OSPTexture2D();
  }

  int xsize = vColorTextureMap->GetExtent()[1] - vColorTextureMap->GetExtent()[0];
  int ysize = vColorTextureMap->GetExtent()[3] - vColorTextureMap->GetExtent()[2];

  if (xsize < 0 || ysize < 0)
  {
    return nullptr;
  }
  int scalartype = vColorTextureMap->GetScalarType();
  int comps = vColorTextureMap->GetNumberOfScalarComponents();

  OSPTexture t2d = nullptr;

  if (scalartype == VTK_UNSIGNED_CHAR || scalartype == VTK_CHAR || scalartype == VTK_SIGNED_CHAR)
  {
    OSPTextureFormat formatSRGB[4] = { OSP_TEXTURE_L8, OSP_TEXTURE_LA8, OSP_TEXTURE_SRGB,
      OSP_TEXTURE_SRGBA };
    OSPTextureFormat formatLinear[4] = { OSP_TEXTURE_R8, OSP_TEXTURE_RGB8, OSP_TEXTURE_RGB8,
      OSP_TEXTURE_RGBA8 };
    std::vector<unsigned char> chars;

    if ((!isSRGB && comps == 2) || comps > 4)
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
      isSRGB ? formatSRGB[comps - 1] : formatLinear[comps - 1],
      chars.empty() ? vColorTextureMap->GetScalarPointer() : chars.data(),
      OSP_TEXTURE_FILTER_NEAREST);
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
      format[comps - 1], floats.empty() ? vColorTextureMap->GetScalarPointer() : floats.data(),
      OSP_TEXTURE_FILTER_NEAREST);
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
      format[newComps - 1], floats.data(), OSP_TEXTURE_FILTER_NEAREST);
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
    vtkGenericWarningMacro("No material Library in this renderer. Using obj by default.");
    return NewMaterial(orn, oRenderer, "obj");
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
            ospSetInt(oMaterial, param.first.c_str(), static_cast<int>(values[0]));
          }
        }
        break;
        case vtkOSPRayMaterialLibrary::ParameterType::FLOAT:
        case vtkOSPRayMaterialLibrary::ParameterType::NORMALIZED_FLOAT:
        {
          auto values = ml->GetDoubleShaderVariable(nickname, param.first);
          if (values.size() == 1)
          {
            ospSetFloat(oMaterial, param.first.c_str(), static_cast<float>(values[0]));
          }
        }
        break;
        case vtkOSPRayMaterialLibrary::ParameterType::FLOAT_DATA:
        {
          auto values = ml->GetDoubleShaderVariable(nickname, param.first);
          if (!values.empty())
          {
            std::vector<float> fvalues(values.begin(), values.end());
            OSPData data = ospNewCopyData1D(fvalues.data(), OSP_VEC3F, fvalues.size() / 3);
            ospSetObject(oMaterial, param.first.c_str(), data);
          }
        }
        break;
        case vtkOSPRayMaterialLibrary::ParameterType::VEC2:
        {
          auto values = ml->GetDoubleShaderVariable(nickname, param.first);
          if (values.size() == 2)
          {
            std::vector<float> fvalues(values.begin(), values.end());
            ospSetVec2f(oMaterial, param.first.c_str(), fvalues[0], fvalues[1]);
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
            ospSetVec3f(oMaterial, param.first.c_str(), fvalues[0], fvalues[1], fvalues[2]);
          }
        }
        break;
        case vtkOSPRayMaterialLibrary::ParameterType::VEC4:
        {
          auto values = ml->GetDoubleShaderVariable(nickname, param.first);
          if (values.size() == 4)
          {
            std::vector<float> fvalues(values.begin(), values.end());
            ospSetVec4f(
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
            ospRelease(t2d);
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
      "Warning: unrecognized material \"" << implname << "\", using a default obj");
    return NewMaterial(orn, oRenderer, "obj");
  }

  ospCommit(oMaterial);
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
  result = ospNewMaterial(rendererType.c_str(), ospMatName.c_str());

  if (!result)
  {
    vtkGenericWarningMacro(
      "OSPRay failed to create material: " << ospMatName << ". Trying obj instead.");
    result = ospNewMaterial(rendererType.c_str(), "obj");
  }

  ospCommit(result);
  return result;
}
VTK_ABI_NAMESPACE_END
