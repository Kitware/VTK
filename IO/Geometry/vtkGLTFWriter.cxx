/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGLTFWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGLTFWriter.h"
#include "vtkGLTFWriterUtils.h"

#include <cstdio>
#include <memory>
#include <sstream>

#include "vtk_jsoncpp.h"

#include "vtkArrayDispatch.h"
#include "vtkAssemblyPath.h"
#include "vtkBase64OutputStream.h"
#include "vtkCamera.h"
#include "vtkCollectionRange.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectTreeIterator.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageReader.h"
#include "vtkInformation.h"
#include "vtkJPEGReader.h"
#include "vtkLogger.h"
#include "vtkMapper.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkPNGReader.h"
#include "vtkPNGWriter.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRendererCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkTexture.h"
#include "vtkTriangleFilter.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

#include <fstream>

vtkStandardNewMacro(vtkGLTFWriter);

vtkGLTFWriter::vtkGLTFWriter()
{
  this->FileName = nullptr;
  this->TextureBaseDirectory = nullptr;
  this->InlineData = false;
  this->SaveNormal = false;
  this->SaveBatchId = false;
  this->SaveTextures = true;
}

vtkGLTFWriter::~vtkGLTFWriter()
{
  this->SetFileName(nullptr);
  this->SetTextureBaseDirectory(nullptr);
}

namespace
{
// The functor that implements the algorithm:
struct FlipYTCoordsWorker
{
  template <typename InOutArrayT>
  void operator()(InOutArrayT* inArray) const
  {
    // TupleRanges iterate tuple-by-tuple:
    auto inRange = vtk::DataArrayTupleRange(inArray);

    const vtk::TupleIdType numTuples = inRange.size();

    for (vtk::TupleIdType tupleId = 0; tupleId < numTuples; ++tupleId)
    {
      auto inTuple = inRange[tupleId];
      inTuple[1] = 1 - inTuple[1];
    }
  }
};

// Code to call the dispatcher:
void FlipYTCoords(vtkDataArray* inOutArray)
{
  // Create an alias for a dispatcher that handles three arrays and only
  // generates code for cases where all three arrays use float or double:
  using FastPathTypes = vtkArrayDispatch::Reals;
  using Dispatcher = vtkArrayDispatch::DispatchByValueType<FastPathTypes>;

  // Create the functor:
  FlipYTCoordsWorker worker;

  // Check if the arrays are using float/double, and if so,
  // run an optimized specialization of the algorithm.
  if (!Dispatcher::Execute(inOutArray, worker))
  {
    // If Execute(...) fails, the arrays don't match the constraints.
    // Run the algorithm using the slower vtkDataArray double API instead:
    worker(inOutArray);
  }
}

std::string GetFieldAsString(vtkDataObject* obj, const char* name)
{
  vtkFieldData* fd = obj->GetFieldData();
  if (!fd)
  {
    return std::string();
  }
  vtkStringArray* sa = vtkStringArray::SafeDownCast(fd->GetAbstractArray(name));
  if (!sa)
  {
    return std::string();
  }
  return sa->GetValue(0);
}

std::vector<float> GetFieldAsFloat(
  vtkDataObject* obj, const char* name, const std::vector<float>& d)
{
  vtkFieldData* fd = obj->GetFieldData();
  if (!fd)
  {
    return d;
  }
  vtkFloatArray* fa = vtkFloatArray::SafeDownCast(fd->GetAbstractArray(name));
  if (!fa)
  {
    return d;
  }
  std::vector<float> v(d.size());
  fa->GetTypedTuple(0, &v[0]);
  return v;
}

vtkSmartPointer<vtkImageReader2> SetupTextureReader(const std::string& texturePath)
{
  std::string ext = vtksys::SystemTools::GetFilenameLastExtension(texturePath);
  vtkSmartPointer<vtkImageReader2> reader;
  if (ext == ".png")
  {
    reader = vtkSmartPointer<vtkPNGReader>::New();
  }
  else if (ext == ".jpg")
  {
    reader = vtkSmartPointer<vtkJPEGReader>::New();
  }
  else
  {
    vtkLog(ERROR, "Invalid type for texture file: " << texturePath);
    return nullptr;
  }
  reader->SetFileName(texturePath.c_str());
  return reader;
}

std::string GetMimeType(const char* textureFileName)
{
  std::string ext = vtksys::SystemTools::GetFilenameLastExtension(textureFileName);
  if (ext == ".png")
  {
    return "image/png";
  }
  else if (ext == ".jpg")
  {
    return "image/jpeg";
  }
  else
  {
    vtkLog(ERROR, "Invalid mime type for texture file: " << textureFileName);
    return "";
  }
}

std::string WriteBufferAndView(const char* gltfRelativeTexturePath, const char* texturePath,
  bool inlineData, Json::Value& buffers, Json::Value& bufferViews)
{
  // if inline then base64 encode the data. In this case we need to read the texture
  std::string result;
  std::string mimeType;
  unsigned int byteLength = 0;
  if (inlineData)
  {
    vtkSmartPointer<vtkTexture> t;
    vtkSmartPointer<vtkImageData> id;

    auto textureReader = SetupTextureReader(texturePath);
    vtkNew<vtkTexture> texture;
    texture->SetInputConnection(textureReader->GetOutputPort());
    texture->Update();
    t = texture;
    id = t->GetInput();

    vtkUnsignedCharArray* da = nullptr;
    if (id && id->GetPointData()->GetScalars())
    {
      da = vtkUnsignedCharArray::SafeDownCast(id->GetPointData()->GetScalars());
    }
    if (!da)
    {
      return mimeType; /*empty mimeType signals error*/
    }

    vtkNew<vtkTrivialProducer> triv;
    triv->SetOutput(id);
    // no need to flip Y the texture as we filp the texture coordinates

    // convert to png
    vtkNew<vtkPNGWriter> png;
    png->SetCompressionLevel(5);
    png->SetInputConnection(triv->GetOutputPort());
    png->WriteToMemoryOn();
    png->Write();
    da = png->GetResult();

    mimeType = "image/png";

    result = "data:application/octet-stream;base64,";
    std::ostringstream toString;
    vtkNew<vtkBase64OutputStream> ostr;
    ostr->SetStream(&toString);
    ostr->StartWriting();
    vtkGLTFWriterUtils::WriteValues(da, ostr);
    ostr->EndWriting();
    result += toString.str();
    unsigned int count = da->GetNumberOfTuples() * da->GetNumberOfComponents();
    byteLength = da->GetElementComponentSize() * count;
  }
  else
  {
    // otherwise we only refer to the image file.
    result = gltfRelativeTexturePath;
    // byte length
    std::ifstream textureStream(texturePath, ios::binary);
    if (textureStream.fail())
    {
      return mimeType; /* empty mimeType signals error*/
    }
    textureStream.seekg(0, ios::end);
    byteLength = textureStream.tellg();
    // mimeType from extension
    mimeType = GetMimeType(texturePath);
  }

  Json::Value buffer;
  Json::Value view;

  buffer["byteLength"] = static_cast<Json::Value::Int64>(byteLength);
  buffer["uri"] = result;
  buffers.append(buffer);

  // write the buffer views
  view["buffer"] = buffers.size() - 1;
  view["byteOffset"] = 0;
  view["byteLength"] = static_cast<Json::Value::Int64>(byteLength);
  bufferViews.append(view);

  return mimeType;
}

void WriteMesh(Json::Value& accessors, Json::Value& buffers, Json::Value& bufferViews,
  Json::Value& meshes, Json::Value& nodes, vtkPolyData* pd, const char* fileName, bool inlineData,
  bool saveNormal, bool saveBatchId)
{
  vtkNew<vtkTriangleFilter> trif;
  trif->SetInputData(pd);
  trif->Update();
  vtkPolyData* tris = trif->GetOutput();

  // write the point locations
  int pointAccessor = 0;
  {
    vtkDataArray* da = tris->GetPoints()->GetData();
    vtkGLTFWriterUtils::WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "VEC3";
    acc["componentType"] = GL_FLOAT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfTuples());
    double range[6];
    tris->GetPoints()->GetBounds(range);
    Json::Value mins;
    mins.append(range[0]);
    mins.append(range[2]);
    mins.append(range[4]);
    Json::Value maxs;
    maxs.append(range[1]);
    maxs.append(range[3]);
    maxs.append(range[5]);
    acc["min"] = mins;
    acc["max"] = maxs;
    pointAccessor = accessors.size();
    accessors.append(acc);
  }

  std::vector<vtkDataArray*> arraysToSave;
  vtkNew<vtkFloatArray> normals;
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
  int userAccessorsStart = accessors.size();
  for (size_t i = 0; i < arraysToSave.size(); ++i)
  {
    vtkDataArray* da = arraysToSave[i];
    vtkGLTFWriterUtils::WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = da->GetNumberOfComponents() == 3 ? "VEC3" : "SCALAR";
    acc["componentType"] = GL_FLOAT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfTuples());
    accessors.append(acc);
  }

  // if we have tcoords then write them out
  // first check for colortcoords
  int tcoordAccessor = -1;
  vtkDataArray* tcoords = tris->GetPointData()->GetTCoords();
  if (tcoords)
  {
    // if there is a valid texture image flipY the tcoords
    auto flipY = vtk::TakeSmartPointer(tcoords->NewInstance());
    flipY->DeepCopy(tcoords);
    FlipYTCoords(flipY);
    vtkGLTFWriterUtils::WriteBufferAndView(flipY, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = tcoords->GetNumberOfComponents() == 3 ? "VEC3" : "VEC2";
    acc["componentType"] = GL_FLOAT;
    acc["normalized"] = false;
    acc["count"] = static_cast<Json::Value::Int64>(tcoords->GetNumberOfTuples());
    tcoordAccessor = accessors.size();
    accessors.append(acc);
  }

  // to store the primitives
  Json::Value prims;

  // write out the verts
  if (tris->GetVerts() && tris->GetVerts()->GetNumberOfCells())
  {
    Json::Value aprim;
    aprim["mode"] = 0;
    Json::Value attribs;

    vtkCellArray* da = tris->GetVerts();
    vtkGLTFWriterUtils::WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfCells());
    aprim["indices"] = accessors.size();
    accessors.append(acc);

    attribs["POSITION"] = pointAccessor;
    int userAccessor = userAccessorsStart;
    for (size_t i = 0; i < arraysToSave.size(); ++i)
    {
      attribs[arraysToSave[i]->GetName()] = userAccessor++;
    }
    if (tcoordAccessor >= 0)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.append(aprim);
  }

  // write out the lines
  if (tris->GetLines() && tris->GetLines()->GetNumberOfCells())
  {
    Json::Value aprim;
    aprim["mode"] = 1;
    Json::Value attribs;

    vtkCellArray* da = tris->GetLines();
    vtkGLTFWriterUtils::WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfCells() * 2);
    aprim["indices"] = accessors.size();
    accessors.append(acc);

    attribs["POSITION"] = pointAccessor;
    int userAccessor = userAccessorsStart;
    for (size_t i = 0; i < arraysToSave.size(); ++i)
    {
      attribs[arraysToSave[i]->GetName()] = userAccessor++;
    }
    if (tcoordAccessor >= 0)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.append(aprim);
  }

  // write out the triangles
  if (tris->GetPolys() && tris->GetPolys()->GetNumberOfCells())
  {
    Json::Value aprim;
    aprim["mode"] = 4;
    Json::Value attribs;

    vtkCellArray* da = tris->GetPolys();
    vtkGLTFWriterUtils::WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    Json::Value acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = "SCALAR";
    acc["componentType"] = GL_UNSIGNED_INT;
    acc["count"] = static_cast<Json::Value::Int64>(da->GetNumberOfCells() * 3);
    aprim["indices"] = accessors.size();
    accessors.append(acc);

    attribs["POSITION"] = pointAccessor;
    int userAccessor = userAccessorsStart;
    for (size_t i = 0; i < arraysToSave.size(); ++i)
    {
      attribs[arraysToSave[i]->GetName()] = userAccessor++;
    }
    if (tcoordAccessor >= 0)
    {
      attribs["TEXCOORD_0"] = tcoordAccessor;
    }
    aprim["attributes"] = attribs;
    prims.append(aprim);
  }

  Json::Value amesh;
  char meshNameBuffer[32];
  sprintf(meshNameBuffer, "mesh%d", meshes.size());
  amesh["name"] = meshNameBuffer;
  amesh["primitives"] = prims;
  meshes.append(amesh);

  // write out a surface
  Json::Value child;
  child["mesh"] = meshes.size() - 1;
  child["name"] = meshNameBuffer;
  nodes.append(child);
}

void WriteCamera(Json::Value& cameras, vtkRenderer* ren)
{
  vtkCamera* cam = ren->GetActiveCamera();
  Json::Value acamera;
  Json::Value camValues;
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
  cameras.append(acamera);
}

void WriteTexture(Json::Value& buffers, Json::Value& bufferViews, Json::Value& textures,
  Json::Value& samplers, Json::Value& images, bool inlineData,
  std::map<std::string, unsigned int>& textureMap, const char* textureBaseDirectory,
  const char* textureFileName, const char* gltfFileName)
{
  unsigned int textureSource = 0;
  if (textureMap.find(textureFileName) == textureMap.end())
  {
    // compute the relative texture base directory from the gltFile
    // initially they are either absolute or relative to the CWD
    std::string gltfFullPath = vtksys::SystemTools::CollapseFullPath(gltfFileName);
    std::string gltfFullDir = vtksys::SystemTools::GetFilenamePath(gltfFullPath);
    std::string texturePath = std::string(textureBaseDirectory) + "/" + textureFileName;
    std::string textureFullPath = vtksys::SystemTools::CollapseFullPath(texturePath);
    std::string gltfRelativeTexturePath =
      vtksys::SystemTools::RelativePath(gltfFullDir, textureFullPath);
    std::string mimeType = WriteBufferAndView(
      gltfRelativeTexturePath.c_str(), texturePath.c_str(), inlineData, buffers, bufferViews);
    if (mimeType.empty())
    {
      return;
    }

    // write the image
    Json::Value img;
    img["bufferView"] = bufferViews.size() - 1;
    img["mimeType"] = mimeType;
    images.append(img);

    textureSource = images.size() - 1;
    textureMap[textureFileName] = textureSource;

    // write the sampler
    Json::Value smp;
    smp["magFilter"] = GL_NEAREST;
    smp["minFilter"] = GL_NEAREST;
    smp["wrapS"] = GL_CLAMP_TO_EDGE;
    smp["wrapT"] = GL_CLAMP_TO_EDGE;
    // use vtkTexture defaults
    smp["wrapS"] = GL_REPEAT;
    smp["wrapT"] = GL_REPEAT;
    smp["magFilter"] = GL_NEAREST;
    smp["minFilter"] = GL_NEAREST;
    samplers.append(smp);
  }
  else
  {
    textureSource = textureMap[textureFileName];
  }

  Json::Value texture;
  texture["source"] = textureSource;
  texture["sampler"] = textureSource;
  textures.append(texture);
}

void WriteMaterial(vtkPolyData* pd, Json::Value& materials, int textureIndex, bool haveTexture)
{
  Json::Value mat;
  Json::Value model;

  if (haveTexture)
  {
    Json::Value tex;
    tex["texCoord"] = 0; // TEXCOORD_0
    tex["index"] = textureIndex;
    model["baseColorTexture"] = tex;
  }

  std::vector<float> dcolor = GetFieldAsFloat(pd, "diffuse_color", { 1, 1, 1 });
  std::vector<float> scolor = GetFieldAsFloat(pd, "specular_color", { 0, 0, 0 });
  float transparency = GetFieldAsFloat(pd, "transparency", { 0 })[0];
  float shininess = GetFieldAsFloat(pd, "shininess", { 0 })[0];
  model["baseColorFactor"].append(dcolor[0]);
  model["baseColorFactor"].append(dcolor[1]);
  model["baseColorFactor"].append(dcolor[2]);
  model["baseColorFactor"].append(1 - transparency);
  model["metallicFactor"] = shininess;
  model["roughnessFactor"] = 1.0;
  mat["pbrMetallicRoughness"] = model;
  materials.append(mat);
}
}

std::string vtkGLTFWriter::WriteToString()
{
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(this->GetInput());
  if (mb == nullptr)
  {
    vtkErrorMacro(<< "This writer needs a vtkMultiBlockDataSet input");
    return std::string();
  }
  std::ostringstream result;

  this->WriteToStream(result, mb);

  return result.str();
}

void vtkGLTFWriter::WriteData()
{
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(this->GetInput());
  if (mb == nullptr)
  {
    vtkErrorMacro(<< "This writer needs a vtkMultiBlockDataSet input");
    return;
  }
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

  this->WriteToStream(output, mb);
  output.close();
}

void vtkGLTFWriter::WriteToStream(ostream& output, vtkMultiBlockDataSet* mb)
{
  Json::Value cameras;
  Json::Value bufferViews;
  Json::Value buffers;
  Json::Value accessors;
  Json::Value nodes;
  Json::Value meshes;
  Json::Value textures;
  Json::Value images;
  Json::Value samplers;
  Json::Value materials;
  std::vector<unsigned int> topNodes;

  // support sharing texture maps
  std::map<std::string, unsigned int> textureMap;

  vtkNew<vtkRenderer> ren;
  double bounds[6];
  mb->GetBounds(bounds);
  ren->ResetCamera(bounds);

  // setup the camera data in case we need to use it later
  Json::Value anode;
  anode["camera"] = cameras.size(); // camera node
  vtkMatrix4x4* mat = ren->GetActiveCamera()->GetModelViewTransformMatrix();
  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      anode["matrix"].append(mat->GetElement(j, i));
    }
  }
  anode["name"] = "Camera Node";

  // setup renderer group node
  Json::Value rendererNode;
  rendererNode["name"] = "Renderer Node";

  auto buildingIt = vtk::TakeSmartPointer(mb->NewTreeIterator());
  buildingIt->VisitOnlyLeavesOff();
  buildingIt->TraverseSubTreeOff();

  bool foundVisibleProp = false;
  // all buildings
  for (buildingIt->InitTraversal(); !buildingIt->IsDoneWithTraversal(); buildingIt->GoToNextItem())
  {
    auto building = vtkMultiBlockDataSet::SafeDownCast(buildingIt->GetCurrentDataObject());
    // all parts - actors (all parts of a buildings)
    auto it = vtk::TakeSmartPointer(building->NewIterator());
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
      auto pd = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
      if (!pd)
      {
        vtkLog(WARNING, "Expecting vtkPolyData but got: " << pd->GetClassName());
      }

      if (pd && pd->GetNumberOfCells() > 0)
      {
        foundVisibleProp = true;
        WriteMesh(accessors, buffers, bufferViews, meshes, nodes, pd, this->FileName,
          this->InlineData, this->SaveNormal, this->SaveBatchId);
        rendererNode["children"].append(nodes.size() - 1);
        unsigned int oldTextureCount = textures.size();
        std::string textureFileName = GetFieldAsString(pd, "texture_uri");
        if (this->SaveTextures && !textureFileName.empty())
        {
          WriteTexture(buffers, bufferViews, textures, samplers, images, this->InlineData,
            textureMap, this->TextureBaseDirectory, textureFileName.c_str(), this->FileName);
        }
        meshes[meshes.size() - 1]["primitives"][0]["material"] = materials.size();
        WriteMaterial(pd, materials, oldTextureCount, oldTextureCount != textures.size());
      }
    }
  }
  // only write the camera if we had visible nodes
  if (foundVisibleProp)
  {
    WriteCamera(cameras, ren);
    nodes.append(anode);
    rendererNode["children"].append(nodes.size() - 1);
    nodes.append(rendererNode);
    topNodes.push_back(nodes.size() - 1);
  }

  Json::Value root;
  Json::Value asset;
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

  Json::Value ascene;
  ascene["name"] = "Layer 0";
  Json::Value noderefs;
  for (auto i : topNodes)
  {
    noderefs.append(i);
  }
  ascene["nodes"] = noderefs;
  Json::Value scenes;
  scenes.append(ascene);
  root["scenes"] = scenes;

  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = "   ";
  std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
  writer->write(root, &output);
}

void vtkGLTFWriter::PrintSelf(ostream& os, vtkIndent indent)
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

int vtkGLTFWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkMultiBlockDataSet");
  return 1;
}
