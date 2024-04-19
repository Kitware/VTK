// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkGLTFExporter.h"
#include "vtkGLTFWriterUtils.h"

#include <cstdio>
#include <memory>
#include <sstream>

#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

#include "vtkAssemblyPath.h"
#include "vtkBase64OutputStream.h"
#include "vtkCamera.h"
#include "vtkCollectionRange.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageFlip.h"
#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkObjectFactory.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkTexture.h"
#include "vtkTriangleFilter.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGLTFExporter);

vtkGLTFExporter::vtkGLTFExporter()
{
  this->FileName = nullptr;
  this->InlineData = false;
  this->SaveNormal = false;
  this->SaveBatchId = false;
}

vtkGLTFExporter::~vtkGLTFExporter()
{
  delete[] this->FileName;
}

namespace
{

vtkPolyData* findPolyData(vtkDataObject* input)
{
  // do we have polydata?
  vtkPolyData* pd = vtkPolyData::SafeDownCast(input);
  if (pd)
  {
    return pd;
  }
  vtkCompositeDataSet* cd = vtkCompositeDataSet::SafeDownCast(input);
  if (cd)
  {
    vtkSmartPointer<vtkCompositeDataIterator> iter;
    iter.TakeReference(cd->NewIterator());
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
      if (pd)
      {
        return pd;
      }
    }
  }
  return nullptr;
}

void WriteMesh(nlohmann::json& accessors, nlohmann::json& buffers, nlohmann::json& bufferViews,
  nlohmann::json& meshes, nlohmann::json& nodes, vtkPolyData* pd, vtkActor* aPart,
  const char* fileName, bool inlineData, bool saveNormal, bool saveBatchId)
{
  vtkNew<vtkTriangleFilter> trif;
  trif->SetInputData(pd);
  trif->Update();
  vtkPolyData* tris = trif->GetOutput();

  // write the point locations
  size_t pointAccessor = 0;
  {
    vtkDataArray* da = tris->GetPoints()->GetData();
    vtkGLTFWriterUtils::WriteBufferAndView(
      da, fileName, inlineData, buffers, bufferViews, GLTF_ARRAY_BUFFER);

    // write the accessor
    nlohmann::json acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "VEC3";
    acc["componentType"] = GL_FLOAT;
    acc["count"] = da->GetNumberOfTuples();
    double range[6];
    tris->GetPoints()->GetBounds(range);
    nlohmann::json mins;
    mins.emplace_back(range[0]);
    mins.emplace_back(range[2]);
    mins.emplace_back(range[4]);
    nlohmann::json maxs;
    maxs.emplace_back(range[1]);
    maxs.emplace_back(range[3]);
    maxs.emplace_back(range[5]);
    acc["min"] = mins;
    acc["max"] = maxs;
    pointAccessor = accessors.size();
    accessors.emplace_back(acc);
  }

  std::vector<vtkDataArray*> arraysToSave;
  if (saveBatchId)
  {
    vtkDataArray* a;
    if ((a = pd->GetPointData()->GetArray("_BATCHID")))
    {
      arraysToSave.push_back(a);
    }
  }
  if (saveNormal)
  {
    vtkDataArray* a;
    if ((a = pd->GetPointData()->GetArray("NORMAL")))
    {
      arraysToSave.push_back(a);
    }
  }
  size_t userAccessorsStart = accessors.size();
  for (size_t i = 0; i < arraysToSave.size(); ++i)
  {
    vtkDataArray* da = arraysToSave[i];
    vtkGLTFWriterUtils::WriteBufferAndView(
      da, fileName, inlineData, buffers, bufferViews, GLTF_ARRAY_BUFFER);

    // write the accessor
    nlohmann::json acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = da->GetNumberOfComponents() == 3 ? "VEC3" : "SCALAR";
    acc["componentType"] = GL_FLOAT;
    acc["count"] = da->GetNumberOfTuples();
    accessors.emplace_back(acc);
  }

  // if we have vertex colors then write them out
  size_t vertColorAccessor = 0;
  aPart->GetMapper()->MapScalars(tris, 1.0);
  bool vertColor = aPart->GetMapper()->GetColorMapColors() != nullptr;
  if (vertColor)
  {
    vtkUnsignedCharArray* da = aPart->GetMapper()->GetColorMapColors();
    vtkGLTFWriterUtils::WriteBufferAndView(
      da, fileName, inlineData, buffers, bufferViews, GLTF_ARRAY_BUFFER);

    // write the accessor
    nlohmann::json acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "VEC4";
    acc["componentType"] = GL_UNSIGNED_BYTE;
    acc["normalized"] = true;
    acc["count"] = da->GetNumberOfTuples();
    vertColorAccessor = accessors.size();
    accessors.emplace_back(acc);
  }

  // if we have tcoords then write them out
  // first check for colortcoords
  size_t tcoordAccessor = 0;
  vtkFloatArray* tcoords = aPart->GetMapper()->GetColorCoordinates();
  if (!tcoords)
  {
    tcoords = vtkFloatArray::SafeDownCast(tris->GetPointData()->GetTCoords());
  }
  if (tcoords)
  {
    vtkFloatArray* da = tcoords;
    vtkGLTFWriterUtils::WriteBufferAndView(
      tcoords, fileName, inlineData, buffers, bufferViews, GLTF_ARRAY_BUFFER);

    // write the accessor
    nlohmann::json acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = da->GetNumberOfComponents() == 3 ? "VEC3" : "VEC2";
    acc["componentType"] = GL_FLOAT;
    acc["normalized"] = false;
    acc["count"] = da->GetNumberOfTuples();
    tcoordAccessor = accessors.size();
    accessors.emplace_back(acc);
  }

  // to store the primitives
  nlohmann::json prims;

  // write out the verts
  if (tris->GetVerts() && tris->GetVerts()->GetNumberOfCells())
  {
    nlohmann::json aprim;
    aprim["mode"] = 0;
    nlohmann::json attribs;

    vtkCellArray* da = tris->GetVerts();
    vtkGLTFWriterUtils::WriteCellBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    nlohmann::json acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = da->GetNumberOfCells();
    aprim["indices"] = accessors.size();
    accessors.emplace_back(acc);

    attribs["POSITION"] = pointAccessor;
    size_t userAccessor = userAccessorsStart;
    for (size_t i = 0; i < arraysToSave.size(); ++i)
    {
      attribs[arraysToSave[i]->GetName()] = userAccessor++;
    }
    if (vertColor)
    {
      attribs["COLOR_0"] = vertColorAccessor;
    }
    if (tcoords)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.emplace_back(aprim);
  }

  // write out the lines
  if (tris->GetLines() && tris->GetLines()->GetNumberOfCells())
  {
    nlohmann::json aprim;
    aprim["mode"] = 1;
    nlohmann::json attribs;

    vtkCellArray* da = tris->GetLines();
    vtkGLTFWriterUtils::WriteCellBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    nlohmann::json acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = da->GetNumberOfCells() * 2;
    aprim["indices"] = accessors.size();
    accessors.emplace_back(acc);

    attribs["POSITION"] = pointAccessor;
    size_t userAccessor = userAccessorsStart;
    for (size_t i = 0; i < arraysToSave.size(); ++i)
    {
      attribs[arraysToSave[i]->GetName()] = userAccessor++;
    }
    if (vertColor)
    {
      attribs["COLOR_0"] = vertColorAccessor;
    }
    if (tcoords)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.emplace_back(aprim);
  }

  // write out the triangles
  if (tris->GetPolys() && tris->GetPolys()->GetNumberOfCells())
  {
    nlohmann::json aprim;
    aprim["mode"] = 4;
    nlohmann::json attribs;

    vtkCellArray* da = tris->GetPolys();
    vtkGLTFWriterUtils::WriteCellBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    nlohmann::json acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = da->GetNumberOfCells() * 3;
    aprim["indices"] = accessors.size();
    accessors.emplace_back(acc);

    attribs["POSITION"] = pointAccessor;
    size_t userAccessor = userAccessorsStart;
    for (size_t i = 0; i < arraysToSave.size(); ++i)
    {
      attribs[arraysToSave[i]->GetName()] = userAccessor++;
    }
    if (vertColor)
    {
      attribs["COLOR_0"] = vertColorAccessor;
    }
    if (tcoords)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.emplace_back(aprim);
  }

  nlohmann::json amesh;
  std::string meshName = "mesh" + std::to_string(meshes.size());
  amesh["name"] = meshName;
  amesh["primitives"] = prims;
  meshes.emplace_back(amesh);

  // write out an actor
  nlohmann::json child;
  vtkMatrix4x4* amat = aPart->GetMatrix();
  if (!amat->IsIdentity())
  {
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        child["matrix"].emplace_back(amat->GetElement(j, i));
      }
    }
  }
  child["mesh"] = meshes.size() - 1;
  child["name"] = meshName;
  nodes.emplace_back(child);
}

void WriteCamera(nlohmann::json& cameras, vtkRenderer* ren)
{
  vtkCamera* cam = ren->GetActiveCamera();
  nlohmann::json acamera;
  nlohmann::json camValues;
  camValues["znear"] = cam->GetClippingRange()[0];
  camValues["zfar"] = cam->GetClippingRange()[1];
  if (cam->GetParallelProjection())
  {
    acamera["type"] = "orthographic";
    camValues["xmag"] = cam->GetParallelScale() * ren->GetTiledAspectRatio();
    camValues["ymag"] = cam->GetParallelScale();
    acamera["orthographic"] = camValues;
  }
  else
  {
    acamera["type"] = "perspective";
    camValues["yfov"] = vtkMath::RadiansFromDegrees(cam->GetViewAngle());
    camValues["aspectRatio"] = ren->GetTiledAspectRatio();
    acamera["perspective"] = camValues;
  }
  cameras.emplace_back(acamera);
}

void WriteTexture(nlohmann::json& buffers, nlohmann::json& bufferViews, nlohmann::json& textures,
  nlohmann::json& samplers, nlohmann::json& images, vtkPolyData* pd, vtkActor* aPart,
  const char* fileName, bool inlineData, std::map<vtkUnsignedCharArray*, size_t>& textureMap,
  bool saveNaNValues)
{
  // do we have a texture
  aPart->GetMapper()->MapScalars(pd, 1.0);
  vtkImageData* id = aPart->GetMapper()->GetColorTextureMap();
  vtkTexture* t = nullptr;
  if (!id && aPart->GetTexture())
  {
    t = aPart->GetTexture();
    id = t->GetInput();
  }

  vtkUnsignedCharArray* da = nullptr;
  if (id && id->GetPointData()->GetScalars())
  {
    da = vtkUnsignedCharArray::SafeDownCast(id->GetPointData()->GetScalars());
  }
  if (!da)
  {
    return;
  }

  size_t textureSource = 0;

  if (textureMap.find(da) == textureMap.end())
  {
    textureMap[da] = textures.size();
    vtkSmartPointer<vtkImageData> imageDataToWrite;
    if (!saveNaNValues)
    {
      // Remove the NaN color value from the texture since the interpolation implementation of
      // some external viewers such as MeshLab or Powerpoint have an issue
      // that can cause color clipping.
      // This new feature can be used as a workaround of this issue:
      // [https://gitlab.kitware.com/paraview/paraview/-/issues/22500]
      imageDataToWrite = vtkSmartPointer<vtkImageData>::Take(vtkImageData::New());
      imageDataToWrite->ShallowCopy(id);
      int* newExtent = id->GetExtent();
      // y2 is the component for the Image Height. It disables NaN values.
      // See vtkImageData::SetExtent doc
      newExtent[3] = 0;
      imageDataToWrite->SetExtent(newExtent);
      imageDataToWrite->Squeeze();
    }
    else
    {
      imageDataToWrite = id;
    }
    // flip Y
    vtkNew<vtkTrivialProducer> triv;
    triv->SetOutput(imageDataToWrite);
    triv->SetOutput(id);
    vtkNew<vtkImageFlip> flip;
    flip->SetFilteredAxis(1);
    flip->SetInputConnection(triv->GetOutputPort());

    // convert to png
    vtkNew<vtkPNGWriter> png;
    png->SetCompressionLevel(5);
    png->SetInputConnection(flip->GetOutputPort());
    png->WriteToMemoryOn();
    png->Write();
    da = png->GetResult();

    vtkGLTFWriterUtils::WriteBufferAndView(
      da, fileName, inlineData, buffers, bufferViews, GLTF_ARRAY_BUFFER);

    // write the image
    nlohmann::json img;
    img["bufferView"] = bufferViews.size() - 1;
    img["mimeType"] = "image/png";
    images.emplace_back(img);

    textureSource = images.size() - 1;
  }
  else
  {
    textureSource = textureMap[da];
  }

  // write the sampler
  nlohmann::json smp;
  smp["magFilter"] = GL_NEAREST;
  smp["minFilter"] = GL_NEAREST;
  smp["wrapS"] = GL_CLAMP_TO_EDGE;
  smp["wrapT"] = GL_CLAMP_TO_EDGE;
  if (t)
  {
    smp["wrapS"] = t->GetRepeat() ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    smp["wrapT"] = t->GetRepeat() ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    smp["magFilter"] = t->GetInterpolate() ? GL_LINEAR : GL_NEAREST;
    smp["minFilter"] = t->GetInterpolate() ? GL_LINEAR : GL_NEAREST;
  }
  samplers.emplace_back(smp);

  nlohmann::json texture;
  texture["source"] = textureSource;
  texture["sampler"] = samplers.size() - 1;
  textures.emplace_back(texture);
}

void WriteMaterial(
  nlohmann::json& materials, size_t textureIndex, bool haveTexture, vtkActor* aPart)
{
  nlohmann::json mat;
  nlohmann::json model;

  if (haveTexture)
  {
    nlohmann::json tex;
    tex["texCoord"] = 0; // TEXCOORD_0
    tex["index"] = textureIndex;
    model["baseColorTexture"] = tex;
  }

  vtkProperty* prop = aPart->GetProperty();
  double dcolor[3];
  prop->GetDiffuseColor(dcolor);
  model["baseColorFactor"].emplace_back(dcolor[0]);
  model["baseColorFactor"].emplace_back(dcolor[1]);
  model["baseColorFactor"].emplace_back(dcolor[2]);
  model["baseColorFactor"].emplace_back(prop->GetOpacity());
  if (prop->GetInterpolation() == VTK_PBR)
  {
    model["metallicFactor"] = prop->GetMetallic();
    model["roughnessFactor"] = prop->GetRoughness();
  }
  else
  {
    model["metallicFactor"] = prop->GetSpecular();
    model["roughnessFactor"] = 1.0 / (1.0 + prop->GetSpecular() * 0.2 * prop->GetSpecularPower());
  }
  mat["pbrMetallicRoughness"] = model;
  materials.emplace_back(mat);
}

}

std::string vtkGLTFExporter::WriteToString()
{
  std::ostringstream result;

  this->WriteToStream(result);

  return result.str();
}

void vtkGLTFExporter::WriteData()
{
  vtksys::ofstream output;

  // make sure the user specified a FileName or FilePointer
  if (this->FileName == nullptr)
  {
    vtkErrorMacro(<< "Please specify FileName to use");
    return;
  }

  // try opening the files
  output.open(this->FileName);
  if (!output.is_open())
  {
    vtkErrorMacro("Unable to open file for gltf output.");
    return;
  }

  this->WriteToStream(output);
  output.close();
}

void vtkGLTFExporter::WriteToStream(ostream& output)
{
  nlohmann::json cameras;
  nlohmann::json bufferViews;
  nlohmann::json buffers;
  nlohmann::json accessors;
  nlohmann::json nodes;
  nlohmann::json meshes;
  nlohmann::json textures;
  nlohmann::json images;
  nlohmann::json samplers;
  nlohmann::json materials;

  std::vector<size_t> topNodes;

  // support sharing texture maps
  std::map<vtkUnsignedCharArray*, size_t> textureMap;

  for (auto ren : vtk::Range(this->RenderWindow->GetRenderers()))
  {
    if (this->ActiveRenderer && ren != this->ActiveRenderer)
    {
      // If ActiveRenderer is specified then ignore all other renderers
      continue;
    }
    if (!ren->GetDraw())
    {
      continue;
    }

    // setup the camera data in case we need to use it later
    // the glTF "nodes" list stores global transformations for objects in the
    // scene, so we need to invert the ModelViewTransformMatrix of the camera
    // (by a copy, to avoid mutating the renderer's camera)
    nlohmann::json anode;
    anode["camera"] = cameras.size(); // camera node
    vtkMatrix4x4* mat = ren->GetActiveCamera()->GetModelViewTransformMatrix();
    vtkNew<vtkMatrix4x4> inv;
    inv->DeepCopy(mat);
    inv->Invert();
    for (int i = 0; i < 4; ++i)
    {
      for (int j = 0; j < 4; ++j)
      {
        anode["matrix"].emplace_back(inv->GetElement(j, i));
      }
    }
    anode["name"] = "Camera Node";

    // setup renderer group node
    nlohmann::json rendererNode;
    rendererNode["name"] = "Renderer Node";

    vtkPropCollection* pc;
    vtkProp* aProp;
    pc = ren->GetViewProps();
    vtkCollectionSimpleIterator pit;
    bool foundVisibleProp = false;
    for (pc->InitTraversal(pit); (aProp = pc->GetNextProp(pit));)
    {
      if (!aProp->GetVisibility())
      {
        continue;
      }
      vtkNew<vtkActorCollection> ac;
      aProp->GetActors(ac);
      vtkActor* anActor;
      vtkCollectionSimpleIterator ait;
      for (ac->InitTraversal(ait); (anActor = ac->GetNextActor(ait));)
      {
        vtkAssemblyPath* apath;
        vtkActor* aPart;
        for (anActor->InitPathTraversal(); (apath = anActor->GetNextPath());)
        {
          aPart = static_cast<vtkActor*>(apath->GetLastNode()->GetViewProp());
          if (aPart->GetVisibility() && aPart->GetMapper() &&
            aPart->GetMapper()->GetInputAlgorithm())
          {
            aPart->GetMapper()->GetInputAlgorithm()->Update();
            vtkPolyData* pd = findPolyData(aPart->GetMapper()->GetInputDataObject(0, 0));
            if (pd && pd->GetNumberOfCells() > 0)
            {
              foundVisibleProp = true;
              WriteMesh(accessors, buffers, bufferViews, meshes, nodes, pd, aPart, this->FileName,
                this->InlineData, this->SaveNormal, this->SaveBatchId);
              rendererNode["children"].emplace_back(nodes.size() - 1);
              size_t oldTextureCount = textures.size();
              WriteTexture(buffers, bufferViews, textures, samplers, images, pd, aPart,
                this->FileName, this->InlineData, textureMap, this->SaveNaNValues);
              meshes[meshes.size() - 1]["primitives"][0]["material"] = materials.size();
              WriteMaterial(materials, oldTextureCount, oldTextureCount != textures.size(), aPart);
            }
          }
        }
      }
    }
    // only write the camera if we had visible nodes
    if (foundVisibleProp)
    {
      WriteCamera(cameras, ren);
      nodes.emplace_back(anode);
      rendererNode["children"].emplace_back(nodes.size() - 1);
      nodes.emplace_back(rendererNode);
      topNodes.push_back(nodes.size() - 1);
    }
  }

  nlohmann::json root;
  nlohmann::json asset;
  asset["generator"] = "VTK";
  asset["version"] = "2.0";
  root["asset"] = asset;

  root["scene"] = 0;
  root["cameras"] = cameras;
  root["nodes"] = nodes;
  root["meshes"] = meshes;
  root["buffers"] = buffers;
  root["bufferViews"] = bufferViews;
  root["accessors"] = accessors;
  if (!images.empty())
    root["images"] = images;
  if (!textures.empty())
    root["textures"] = textures;
  if (!samplers.empty())
    root["samplers"] = samplers;
  root["materials"] = materials;

  nlohmann::json ascene;
  ascene["name"] = "Layer 0";
  nlohmann::json noderefs;
  for (auto i : topNodes)
  {
    noderefs.emplace_back(i);
  }
  ascene["nodes"] = noderefs;
  nlohmann::json scenes;
  scenes.emplace_back(ascene);
  root["scenes"] = scenes;
  output << root;
}

void vtkGLTFExporter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << "InlineData: " << this->InlineData << "\n";
  if (this->FileName)
  {
    os << indent << "FileName: " << this->FileName << "\n";
  }
  else
  {
    os << indent << "FileName: (null)\n";
  }
}
VTK_ABI_NAMESPACE_END
