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
#include "vtkDataArray.h"
#include "vtkGLTFWriterUtils.h"

#include <cstdio>
#include <memory>
#include <sstream>

#include "vtkUnsignedShortArray.h"
#include <vtk_nlohmannjson.h>
#include VTK_NLOHMANN_JSON(json.hpp)

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

namespace
{
struct FileHeader
{
  uint32_t Magic = 0x46546C67; // glTF
  uint32_t Version = 2;
  uint32_t Length;
};

struct ChunkHeader
{
  void SetBin(uint32_t length)
  {
    this->Length = length;
    this->Type = 0x004E4942; // BIN
  }
  void SetJSON(uint32_t length)
  {
    this->Length = length;
    this->Type = 0x4E4F534A; // JSON
  }
  uint32_t Length;
  uint32_t Type;
};

}

vtkStandardNewMacro(vtkGLTFWriter);

vtkGLTFWriter::vtkGLTFWriter()
{
  this->FileName = nullptr;
  this->TextureBaseDirectory = nullptr;
  this->InlineData = false;
  this->SaveNormal = false;
  this->SaveBatchId = false;
  this->SaveTextures = true;
  this->SaveActivePointColor = false;
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
  fa->GetTypedTuple(0, v.data());
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

std::string WriteTextureBufferAndView(const char* gltfRelativeTexturePath, const char* texturePath,
  bool inlineData, nlohmann::json& buffers, nlohmann::json& bufferViews)
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
    // no need to flip Y the texture as we flip the texture coordinates

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
    vtksys::ifstream textureStream(texturePath, ios::binary);
    if (textureStream.fail())
    {
      return mimeType; /* empty mimeType signals error*/
    }
    textureStream.seekg(0, ios::end);
    byteLength = textureStream.tellg();
    // mimeType from extension
    mimeType = GetMimeType(texturePath);
  }

  nlohmann::json buffer;
  nlohmann::json view;

  buffer["byteLength"] = byteLength;
  buffer["uri"] = result;
  buffers.emplace_back(buffer);

  // write the buffer views
  view["buffer"] = buffers.size() - 1;
  view["byteOffset"] = 0;
  view["byteLength"] = byteLength;
  bufferViews.emplace_back(view);

  return mimeType;
}

std::map<int, int> vtkToGLType = { { VTK_UNSIGNED_CHAR, GL_UNSIGNED_BYTE },
  { VTK_UNSIGNED_SHORT, GL_UNSIGNED_SHORT }, { VTK_FLOAT, GL_FLOAT } };

int GetGLType(vtkDataArray* da)
{
  int vtkType = da->GetDataType();
  if (vtkToGLType.find(vtkType) == vtkToGLType.end())
  {
    vtkLog(WARNING, "No GL type mapping for VTK type: " << vtkType);
    return GL_UNSIGNED_BYTE;
  }
  return vtkToGLType[vtkType];
}

void WriteMesh(nlohmann::json& accessors, nlohmann::json& buffers, nlohmann::json& bufferViews,
  nlohmann::json& meshes, nlohmann::json& nodes, vtkPolyData* pd, const char* fileName,
  bool inlineData, bool saveNormal, bool saveBatchId, bool saveActivePointColor)
{
  vtkNew<vtkTriangleFilter> trif;
  trif->SetInputData(pd);
  trif->Update();
  vtkPolyData* tris = trif->GetOutput();

  // write the point locations
  size_t pointAccessor = 0;
  {
    vtkDataArray* da = tris->GetPoints()->GetData();
    vtkGLTFWriterUtils::WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

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
  vtkNew<vtkFloatArray> normals;
  vtkNew<vtkUnsignedCharArray> ucColor0;
  vtkNew<vtkUnsignedShortArray> usColor0;
  vtkNew<vtkFloatArray> fColor0;
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
    vtkDataArray* a = pd->GetPointData()->GetNormals();
    if (a)
    {
      normals->ShallowCopy(a);
      normals->SetName("NORMAL");
      arraysToSave.push_back(normals);
    }
  }
  if (saveActivePointColor)
  {
    vtkDataArray* da = pd->GetPointData()->GetScalars();
    auto uca = vtkUnsignedCharArray::SafeDownCast(da);
    auto usa = vtkUnsignedShortArray::SafeDownCast(da);
    auto fa = vtkFloatArray::SafeDownCast(da);
    static const char* gltfColorName = "COLOR_0";
    if (uca)
    {
      ucColor0->ShallowCopy(uca);
      ucColor0->SetName(gltfColorName);
      arraysToSave.push_back(ucColor0);
    }
    else if (usa)
    {
      usColor0->ShallowCopy(usa);
      usColor0->SetName(gltfColorName);
      arraysToSave.push_back(usColor0);
    }
    else if (fa)
    {
      fColor0->ShallowCopy(fa);
      fColor0->SetName(gltfColorName);
      arraysToSave.push_back(fColor0);
    }
    else
    {
      vtkLog(WARNING,
        "Color array has to be unsigned char, unsigned short or float "
        "with 3 or 4 components. Got: "
          << (da ? da->GetClassName() : "nullptr")
          << " number of components: " << (da ? da->GetNumberOfComponents() : 0));
    }
  }
  size_t userAccessorsStart = accessors.size();
  for (size_t i = 0; i < arraysToSave.size(); ++i)
  {
    vtkDataArray* da = arraysToSave[i];
    vtkGLTFWriterUtils::WriteBufferAndView(da, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    nlohmann::json acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = da->GetNumberOfComponents() == 4
      ? "VEC4"
      : (da->GetNumberOfComponents() == 3 ? "VEC3" : "SCALAR");
    acc["componentType"] = GetGLType(da);
    acc["count"] = da->GetNumberOfTuples();
    accessors.emplace_back(acc);
  }

  // if we have tcoords then write them out
  // first check for colortcoords
  size_t tcoordAccessor = 0;
  vtkDataArray* tcoords = tris->GetPointData()->GetTCoords();
  if (tcoords)
  {
    // if there is a valid texture image flipY the tcoords
    auto flipY = vtk::TakeSmartPointer(tcoords->NewInstance());
    flipY->DeepCopy(tcoords);
    FlipYTCoords(flipY);
    vtkGLTFWriterUtils::WriteBufferAndView(flipY, fileName, inlineData, buffers, bufferViews);

    // write the accessor
    nlohmann::json acc;
    acc["bufferView"] = bufferViews.size() - 1;
    acc["byteOffset"] = 0;
    acc["type"] = tcoords->GetNumberOfComponents() == 3 ? "VEC3" : "VEC2";
    acc["componentType"] = GL_FLOAT;
    acc["normalized"] = false;
    acc["count"] = tcoords->GetNumberOfTuples();
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

  // write out a surface
  nlohmann::json child;
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
  nlohmann::json& samplers, nlohmann::json& images, bool inlineData,
  std::map<std::string, size_t>& textureMap, const char* textureBaseDirectory,
  const char* textureFileName, const char* gltfFileName)
{
  size_t textureSource = 0;
  if (textureMap.find(textureFileName) == textureMap.end())
  {
    // compute the relative texture base directory from the gltFile
    // initially they are either absolute or relative to the CWD
    std::string gltfFullPath = vtksys::SystemTools::CollapseFullPath(gltfFileName);
    std::string gltfFullDir = vtksys::SystemTools::GetFilenamePath(gltfFullPath);
    std::string texturePath = std::string(textureBaseDirectory) + "/" + textureFileName;
    std::string textureFullPath = vtksys::SystemTools::CollapseFullPath(texturePath);
    if (!vtksys::SystemTools::FileExists(textureFullPath, true /*isFile*/))
    {
      vtkLog(WARNING, "Invalid texture file: " << textureFullPath);
      return;
    }
    std::string gltfRelativeTexturePath =
      vtksys::SystemTools::RelativePath(gltfFullDir, textureFullPath);
    std::string mimeType = WriteTextureBufferAndView(
      gltfRelativeTexturePath.c_str(), texturePath.c_str(), inlineData, buffers, bufferViews);
    if (mimeType.empty())
    {
      return;
    }

    // write the image
    nlohmann::json img;
    img["bufferView"] = bufferViews.size() - 1;
    img["mimeType"] = mimeType;
    images.emplace_back(img);

    textureSource = images.size() - 1;
    textureMap[textureFileName] = textureSource;

    // write the sampler
    nlohmann::json smp;
    smp["magFilter"] = GL_NEAREST;
    smp["minFilter"] = GL_NEAREST;
    smp["wrapS"] = GL_CLAMP_TO_EDGE;
    smp["wrapT"] = GL_CLAMP_TO_EDGE;
    // use vtkTexture defaults
    smp["wrapS"] = GL_REPEAT;
    smp["wrapT"] = GL_REPEAT;
    smp["magFilter"] = GL_NEAREST;
    smp["minFilter"] = GL_NEAREST;
    samplers.emplace_back(smp);
  }
  else
  {
    textureSource = textureMap[textureFileName];
  }

  nlohmann::json texture;
  texture["source"] = textureSource;
  texture["sampler"] = textureSource;
  textures.emplace_back(texture);
}

void WriteMaterial(
  vtkPolyData* pd, nlohmann::json& materials, size_t textureIndex, bool haveTexture)
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

  std::vector<float> dcolor = GetFieldAsFloat(pd, "diffuse_color", { 1, 1, 1 });
  std::vector<float> scolor = GetFieldAsFloat(pd, "specular_color", { 0, 0, 0 });
  float transparency = GetFieldAsFloat(pd, "transparency", { 0 })[0];
  float shininess = GetFieldAsFloat(pd, "shininess", { 0 })[0];
  model["baseColorFactor"].emplace_back(dcolor[0]);
  model["baseColorFactor"].emplace_back(dcolor[1]);
  model["baseColorFactor"].emplace_back(dcolor[2]);
  model["baseColorFactor"].emplace_back(1 - transparency);
  model["metallicFactor"] = shininess;
  model["roughnessFactor"] = 1.0;
  mat["pbrMetallicRoughness"] = model;
  materials.emplace_back(mat);
}
}

std::string vtkGLTFWriter::WriteToString()
{
  std::ostringstream result;
  this->WriteToStream(result, this->GetInput());
  return result.str();
}

void vtkGLTFWriter::WriteData()
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

  this->WriteToStream(output, this->GetInput());
  output.close();
}

void vtkGLTFWriter::WriteToStream(ostream& output, vtkDataObject* vtkNotUsed(data))
{
  vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(this->GetInput());
  if (mb == nullptr)
  {
    vtkErrorMacro(<< "We need vtkMultiBlockDataSet input but got: "
                  << this->GetInput()->GetClassName());
    return;
  }
  WriteToStreamMultiBlock(output, mb);
}

void vtkGLTFWriter::WriteToStreamMultiBlock(ostream& output, vtkMultiBlockDataSet* mb)
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
  std::map<std::string, size_t> textureMap;

  vtkNew<vtkRenderer> ren;
  double bounds[6];
  mb->GetBounds(bounds);
  ren->ResetCamera(bounds);

  // setup the camera data in case we need to use it later
  nlohmann::json anode;
  anode["camera"] = cameras.size(); // camera node
  vtkMatrix4x4* mat = ren->GetActiveCamera()->GetModelViewTransformMatrix();
  for (int i = 0; i < 4; ++i)
  {
    for (int j = 0; j < 4; ++j)
    {
      anode["matrix"].emplace_back(mat->GetElement(j, i));
    }
  }
  anode["name"] = "Camera Node";

  // setup renderer group node
  nlohmann::json rendererNode;
  rendererNode["name"] = "Renderer Node";

  auto buildingIt = vtk::TakeSmartPointer(mb->NewTreeIterator());
  buildingIt->VisitOnlyLeavesOff();
  buildingIt->TraverseSubTreeOff();

  bool foundVisibleProp = false;
  // all buildings
  for (buildingIt->InitTraversal(); !buildingIt->IsDoneWithTraversal(); buildingIt->GoToNextItem())
  {
    auto building = vtkMultiBlockDataSet::SafeDownCast(buildingIt->GetCurrentDataObject());
    // all parts of a buildings
    auto it = vtk::TakeSmartPointer(building->NewIterator());
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
      auto pd = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
      if (pd)
      {
        if (pd->GetNumberOfCells() > 0)
        {
          foundVisibleProp = true;
          WriteMesh(accessors, buffers, bufferViews, meshes, nodes, pd, this->FileName,
            this->InlineData, this->SaveNormal, this->SaveBatchId, this->SaveActivePointColor);
          rendererNode["children"].emplace_back(nodes.size() - 1);
          size_t oldTextureCount = textures.size();
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
      else
      {
        if (it->GetCurrentDataObject())
        {
          vtkLog(
            WARNING, "Expecting vtkPolyData, got: " << it->GetCurrentDataObject()->GetClassName());
        }
        else
        {
          vtkLog(WARNING, "Expecting vtkPolyData, got: NULL");
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

  output << std::setw(4) << root;
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
