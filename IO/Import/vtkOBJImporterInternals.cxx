/*=========================================================================
  Program:   Visualization Toolkit
  Module:    vtkOBJImporterInternals.cxx
  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkOBJImporterInternals.h"
#include "vtkBMPReader.h"
#include "vtkJPEGReader.h"
#include "vtkOBJImporter.h"
#include "vtkPNGReader.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTIFFReader.h"
#include "vtkTexture.h"
#include "vtkTransform.h"
#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>

#if defined(_WIN32)
#pragma warning(disable : 4800)
#endif

namespace
{
int localVerbosity = 0;
}

void obj_set_material_defaults(vtkOBJImportedMaterial* mtl)
{
  mtl->amb[0] = 0.2;
  mtl->amb[1] = 0.2;
  mtl->amb[2] = 0.2;
  mtl->diff[0] = 0.8;
  mtl->diff[1] = 0.8;
  mtl->diff[2] = 0.8;
  mtl->spec[0] = 1.0;
  mtl->spec[1] = 1.0;
  mtl->spec[2] = 1.0;
  mtl->map_Kd_scale[0] = 1.0;
  mtl->map_Kd_scale[1] = 1.0;
  mtl->map_Kd_scale[2] = 1.0;
  mtl->illum = 2;
  mtl->reflect = 0.0;
  mtl->trans = 1;
  mtl->glossy = 98;
  mtl->specularPower = 0;
  mtl->refract_index = 1;
  mtl->texture_filename[0] = '\0';

  if (localVerbosity > 0)
  {
    vtkGenericWarningMacro("Created a default vtkOBJImportedMaterial, texture filename is "
      << std::string(mtl->texture_filename));
  }
}

// check if the texture file referenced exists
// some files references png when they ship with jpg
// so check for that as well
void checkTextureMapFile(vtkOBJImportedMaterial* current_mtl, std::string& texturePath)
{
  // try texture as specified
  bool bFileExistsNoPath = vtksys::SystemTools::FileExists(current_mtl->texture_filename);
  std::vector<std::string> path_and_file(2);
  path_and_file[0] = texturePath;
  path_and_file[1] = std::string(current_mtl->texture_filename);
  std::string joined = vtksys::SystemTools::JoinPath(path_and_file);
  bool bFileExistsInPath = vtksys::SystemTools::FileExists(joined);
  // if the file does not exist and it has a png extension try for jpg instead
  if (!(bFileExistsNoPath || bFileExistsInPath))
  {
    if (vtksys::SystemTools::GetFilenameLastExtension(current_mtl->texture_filename) == ".png")
    {
      // try jpg
      std::string jpgName =
        vtksys::SystemTools::GetFilenameWithoutLastExtension(current_mtl->texture_filename) +
        ".jpg";
      bFileExistsNoPath = vtksys::SystemTools::FileExists(jpgName);
      path_and_file[0] = texturePath;
      path_and_file[1] = jpgName;
      joined = vtksys::SystemTools::JoinPath(path_and_file);
      bFileExistsInPath = vtksys::SystemTools::FileExists(joined);
      if (bFileExistsInPath || bFileExistsNoPath)
      {
        current_mtl->texture_filename = jpgName;
      }
    }
    if (!(bFileExistsNoPath || bFileExistsInPath))
    {
      vtkGenericWarningMacro(<< "mtl file " << current_mtl->name
                             << " requests texture file that appears not to exist: "
                             << current_mtl->texture_filename << "; texture path: " << texturePath
                             << "\n");
    }
  }
}

namespace
{

class Token
{
public:
  enum TokenType
  {
    Number = 1,
    String,
    Space,
    LineEnd
  };

  TokenType Type;
  double NumberValue = 0.0;
  std::string StringValue = "";
};

bool tokenGetString(size_t& t, std::vector<Token>& tokens, std::string& result)
{
  // must have two more tokens and the next token must be a space
  if (tokens.size() <= t + 2 || tokens[t + 1].Type != Token::Space ||
    tokens[t + 2].Type != Token::String)
  {
    vtkGenericWarningMacro("bad syntax");
    return false;
  }
  result = tokens[t + 2].StringValue;
  t += 2;
  return true;
}

bool tokenGetNumber(size_t& t, std::vector<Token>& tokens, double& result)
{
  // must have two more tokens and the next token must be a space
  if (tokens.size() <= t + 2 || tokens[t + 1].Type != Token::Space ||
    tokens[t + 2].Type != Token::Number)
  {
    vtkGenericWarningMacro("bad syntax");
    return false;
  }
  result = tokens[t + 2].NumberValue;
  t += 2;
  return true;
}

bool tokenGetVector(
  size_t& t, std::vector<Token>& tokens, double* result, size_t resultSize, size_t minNums)
{
  // must have two more tokens and the next token must be a space
  if (tokens.size() <= t + 2 * minNums)
  {
    vtkGenericWarningMacro("bad syntax");
    return false;
  }
  // parse the following numbers
  size_t count = 0;
  while (tokens.size() > t + 2 && tokens[t + 1].Type == Token::Space &&
    tokens[t + 2].Type == Token::Number)
  {
    result[count] = tokens[t + 2].NumberValue;
    t += 2;
    count++;
  }

  // if any values provided then copy the first value to any missing values
  if (count)
  {
    for (size_t i = count; i < resultSize; ++i)
    {
      result[i] = result[count - 1];
    }
  }

  return true;
}

bool tokenGetTexture(size_t& t, std::vector<Token>& tokens, vtkOBJImportedMaterial* current_mtl,
  std::string& texturePath)
{
  // parse the next tokens looking for
  // texture options must all be on one line
  current_mtl->texture_filename = "";
  for (size_t tt = t + 1; tt < tokens.size(); ++tt)
  {
    if (tokens[tt].Type == Token::Space)
    {
      continue;
    }
    if (tokens[tt].Type == Token::LineEnd)
    {
      t = tt;
      return false;
    }

    // string value
    if (tokens[tt].StringValue == "-s")
    {
      tokenGetVector(tt, tokens, current_mtl->map_Kd_scale, 3, 1);
      continue;
    }
    if (tokens[tt].StringValue == "-o")
    {
      tokenGetVector(tt, tokens, current_mtl->map_Kd_offset, 3, 1);
      continue;
    }
    if (tokens[tt].StringValue == "-mm")
    {
      double tmp[2];
      tokenGetVector(tt, tokens, tmp, 2, 1);
      continue;
    }

    // if we got here then must be name of texture file
    // or an unknown option, we combine all tokens
    // form this point forward as they may be a filename
    // with spaces in them
    current_mtl->texture_filename += tokens[tt].StringValue;
    ++tt;
    while (tokens[tt].Type != Token::LineEnd)
    {
      current_mtl->texture_filename += tokens[tt].StringValue;
      ++tt;
    }
    checkTextureMapFile(current_mtl, texturePath);
    t = tt;
    return true;
  }

  return false;
}
}

#include "mtlsyntax.cxx"
std::vector<vtkOBJImportedMaterial*> vtkOBJPolyDataProcessor::ParseOBJandMTL(
  std::string Filename, int& result_code)
{
  std::vector<vtkOBJImportedMaterial*> listOfMaterials;
  result_code = 0;

  if (Filename.empty())
  {
    return listOfMaterials;
  }

  vtksys::ifstream in(Filename.c_str(), std::ios::in | std::ios::binary);
  if (!in)
  {
    return listOfMaterials;
  }

  std::vector<Token> tokens;
  std::string contents;
  in.seekg(0, std::ios::end);
  contents.resize(in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents[0], contents.size());
  in.close();

  // watch for BOM
  if (contents[0] == -17 && contents[1] == -69 && contents[2] == -65)
  {
    result_code = parseMTL(contents.c_str() + 3, tokens);
  }
  else
  {
    result_code = parseMTL(contents.c_str(), tokens);
  }

  // now handle the token stream
  vtkOBJImportedMaterial* current_mtl = nullptr;
  for (size_t t = 0; t < tokens.size(); ++t)
  {
    if (tokens[t].Type == Token::Number)
    {
      vtkErrorMacro("Number found outside of a command or option on token# "
        << t << " with number " << tokens[t].NumberValue);
      break;
    }
    if (tokens[t].Type == Token::Space || tokens[t].Type == Token::LineEnd)
    {
      continue;
    }

    // string value
    std::string lcstr = tokens[t].StringValue;
    std::transform(lcstr.begin(), lcstr.end(), lcstr.begin(), ::tolower);
    if (tokens[t].StringValue == "newmtl")
    {
      current_mtl = (new vtkOBJImportedMaterial);
      listOfMaterials.push_back(current_mtl);
      obj_set_material_defaults(current_mtl);
      tokenGetString(t, tokens, current_mtl->name);
      continue;
    }
    if (tokens[t].StringValue == "Ka")
    {
      tokenGetVector(t, tokens, current_mtl->amb, 3, 1);
      continue;
    }
    if (tokens[t].StringValue == "Kd")
    {
      tokenGetVector(t, tokens, current_mtl->diff, 3, 1);
      continue;
    }
    if (tokens[t].StringValue == "Ks")
    {
      tokenGetVector(t, tokens, current_mtl->spec, 3, 1);
      continue;
    }
    if (tokens[t].StringValue == "Ns")
    {
      tokenGetNumber(t, tokens, current_mtl->specularPower);
      continue;
    }
    if (tokens[t].StringValue == "d")
    {
      tokenGetNumber(t, tokens, current_mtl->trans);
      continue;
    }
    if (tokens[t].StringValue == "illum")
    {
      double tmp;
      if (tokenGetNumber(t, tokens, tmp))
      {
        current_mtl->illum = static_cast<int>(tmp);
      }
      continue;
    }
    if (lcstr == "map_ka" || lcstr == "map_kd")
    {
      tokenGetTexture(t, tokens, current_mtl, this->TexturePath);
      continue;
    }

    // vtkErrorMacro("Unknown command in mtl file at token# " <<
    //   t << " and value " << tokens[t].StringValue);
    // consume to the end of the line
    while (t < tokens.size() && tokens[t].Type != Token::LineEnd)
    {
      ++t;
    }
  }

  return listOfMaterials;
}

void bindTexturedPolydataToRenderWindow(
  vtkRenderWindow* renderWindow, vtkRenderer* renderer, vtkOBJPolyDataProcessor* reader)
{
  if (nullptr == (renderWindow))
  {
    vtkErrorWithObjectMacro(reader, "RenderWindow is null, failure!");
    return;
  }
  if (nullptr == (renderer))
  {
    vtkErrorWithObjectMacro(reader, "Renderer is null, failure!");
    return;
  }
  if (nullptr == (reader))
  {
    vtkErrorWithObjectMacro(reader, "vtkOBJPolyDataProcessor is null, failure!");
    return;
  }

  reader->actor_list.clear();
  reader->actor_list.reserve(reader->GetNumberOfOutputPorts());

  // keep track of textures used and if multiple parts use the same
  // texture, then have the actors use the same texture. This saves memory
  // etc and makes exporting more efficient.
  std::map<std::string, vtkSmartPointer<vtkTexture> > knownTextures;

  for (int port_idx = 0; port_idx < reader->GetNumberOfOutputPorts(); port_idx++)
  {
    vtkPolyData* objPoly = reader->GetOutput(port_idx);

    vtkSmartPointer<vtkPolyDataMapper> mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    mapper->SetInputData(objPoly);

    vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    vtkDebugWithObjectMacro(reader,
      "Grabbed objPoly " << objPoly << ", port index " << port_idx << "\n"
                         << "numPolys = " << objPoly->GetNumberOfPolys()
                         << " numPoints = " << objPoly->GetNumberOfPoints());

    // For each named material, load and bind the texture, add it to the renderer

    std::string textureFilename = reader->GetTextureFilename(port_idx);

    auto kti = knownTextures.find(textureFilename);
    if (kti == knownTextures.end())
    {
      vtkSmartPointer<vtkTIFFReader> tex_tiff_Loader = vtkSmartPointer<vtkTIFFReader>::New();
      vtkSmartPointer<vtkBMPReader> tex_bmp_Loader = vtkSmartPointer<vtkBMPReader>::New();
      vtkSmartPointer<vtkJPEGReader> tex_jpg_Loader = vtkSmartPointer<vtkJPEGReader>::New();
      vtkSmartPointer<vtkPNGReader> tex_png_Loader = vtkSmartPointer<vtkPNGReader>::New();
      int bIsReadableBMP = tex_bmp_Loader->CanReadFile(textureFilename.c_str());
      int bIsReadableJPEG = tex_jpg_Loader->CanReadFile(textureFilename.c_str());
      int bIsReadablePNG = tex_png_Loader->CanReadFile(textureFilename.c_str());
      int bIsReadableTIFF = tex_tiff_Loader->CanReadFile(textureFilename.c_str());

      if (!textureFilename.empty())
      {
        if (bIsReadableJPEG)
        {
          tex_jpg_Loader->SetFileName(textureFilename.c_str());
          tex_jpg_Loader->Update();
          vtkSmartPointer<vtkTexture> vtk_texture = vtkSmartPointer<vtkTexture>::New();
          vtk_texture->AddInputConnection(tex_jpg_Loader->GetOutputPort());
          actor->SetTexture(vtk_texture);
          knownTextures[textureFilename] = vtk_texture;
        }
        else if (bIsReadablePNG)
        {
          tex_png_Loader->SetFileName(textureFilename.c_str());
          tex_png_Loader->Update();
          vtkSmartPointer<vtkTexture> vtk_texture = vtkSmartPointer<vtkTexture>::New();
          vtk_texture->AddInputConnection(tex_png_Loader->GetOutputPort());
          actor->SetTexture(vtk_texture);
          knownTextures[textureFilename] = vtk_texture;
        }
        else if (bIsReadableBMP)
        {
          tex_bmp_Loader->SetFileName(textureFilename.c_str());
          tex_bmp_Loader->Update();
          vtkSmartPointer<vtkTexture> vtk_texture = vtkSmartPointer<vtkTexture>::New();
          vtk_texture->AddInputConnection(tex_bmp_Loader->GetOutputPort());
          actor->SetTexture(vtk_texture);
          knownTextures[textureFilename] = vtk_texture;
        }
        else if (bIsReadableTIFF)
        {
          tex_tiff_Loader->SetFileName(textureFilename.c_str());
          tex_tiff_Loader->Update();
          vtkSmartPointer<vtkTexture> vtk_texture = vtkSmartPointer<vtkTexture>::New();
          vtk_texture->AddInputConnection(tex_tiff_Loader->GetOutputPort());
          actor->SetTexture(vtk_texture);
          knownTextures[textureFilename] = vtk_texture;
        }
        else
        {
          if (!textureFilename
                 .empty()) // OK to have no texture image, but if its not empty it ought to exist.
          {
            vtkErrorWithObjectMacro(
              reader, "Nonexistent texture image type!? imagefile: " << textureFilename);
          }
        }
      }
    }
    else // this is a texture we already have seen
    {
      actor->SetTexture(kti->second);
    }

    vtkSmartPointer<vtkProperty> properties = vtkSmartPointer<vtkProperty>::New();

    vtkOBJImportedMaterial* raw_mtl_data = reader->GetMaterial(port_idx);
    if (raw_mtl_data)
    {
      // handle texture coordinate transforms
      if (actor->GetTexture() &&
        (raw_mtl_data->map_Kd_scale[0] != 1 || raw_mtl_data->map_Kd_scale[1] != 1 ||
          raw_mtl_data->map_Kd_scale[2] != 1))
      {
        vtkNew<vtkTransform> tf;
        tf->Scale(raw_mtl_data->map_Kd_scale[0], raw_mtl_data->map_Kd_scale[1],
          raw_mtl_data->map_Kd_scale[2]);
        actor->GetTexture()->SetTransform(tf);
      }

      properties->SetDiffuseColor(raw_mtl_data->diff);
      properties->SetSpecularColor(raw_mtl_data->spec);
      properties->SetAmbientColor(raw_mtl_data->amb);
      properties->SetOpacity(raw_mtl_data->trans);
      properties->SetInterpolationToPhong();
      switch (raw_mtl_data->illum)
      {
        case 0:
          properties->SetLighting(false);
          properties->SetDiffuse(0);
          properties->SetSpecular(0);
          properties->SetAmbient(1.0);
          properties->SetColor(properties->GetDiffuseColor());
          break;
        case 1:
          properties->SetDiffuse(1.0);
          properties->SetSpecular(0);
          properties->SetAmbient(1.0);
          break;
        default:
        case 2:
          properties->SetDiffuse(1.0);
          properties->SetSpecular(1.0);
          properties->SetAmbient(1.0);
          // blinn to phong ~= 4.0
          properties->SetSpecularPower(raw_mtl_data->specularPower / 4.0);
          break;
      }
      actor->SetProperty(properties);
    }
    renderer->AddActor(actor);

    // properties->ShadingOn(); // use ShadingOn() if loading vtkMaterial from xml
    // available in mtl parser are:
    //    double amb[3];
    //    double diff[3];
    //    double spec[3];
    //    double reflect;
    //    double refract;
    //    double trans;
    //    double shiny;
    //    double glossy;
    //    double refract_index;

    reader->actor_list.push_back(actor); // keep a handle on actors to animate later
  }
  /** post-condition of this function: the renderer has had a bunch of actors added to it */
}

vtkOBJImportedMaterial::vtkOBJImportedMaterial()
{
  this->name = "x";
  obj_set_material_defaults(this);
}
