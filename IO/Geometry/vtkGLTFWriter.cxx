// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkByteSwap.h"
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
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkTriangleFilter.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnsignedIntArray.h"

#include "vtksys/FStream.hxx"
#include "vtksys/SystemTools.hxx"

namespace
{
VTK_ABI_NAMESPACE_BEGIN
struct FileHeader
{
  FileHeader(uint32_t l)
    : Length(l)
  {
  }
  uint32_t Magic = 0x46546C67; // glTF
  uint32_t Version = 2;
  uint32_t Length;
};

struct ChunkHeader
{
  void SetTypeBIN(uint32_t length)
  {
    this->Length = length;
    this->Type = 0x004E4942; // BIN
  }
  void SetTypeJSON(uint32_t length)
  {
    this->Length = length;
    this->Type = 0x4E4F534A; // JSON
  }
  uint32_t Length;
  uint32_t Type;
};

// pad at 4 bytes
inline size_t GetPaddingAt4Bytes(size_t size)
{
  return (4 - size % 4) % 4;
}

VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkGLTFWriter);

vtkGLTFWriter::vtkGLTFWriter()
{
  this->FileName = nullptr;
  this->TextureBaseDirectory = nullptr;
  this->PropertyTextureFile = nullptr;
  this->InlineData = false;
  this->SaveNormal = false;
  this->SaveBatchId = false;
  this->SaveTextures = true;
  this->RelativeCoordinates = false;
  this->CopyTextures = false;
  this->SaveActivePointColor = false;
  this->Binary = false;
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

std::string GetMimeType(const std::string& textureFileName)
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

std::string WriteTextureBufferAndView(const std::string& gltfFullDir,
  const std::string& textureFullPath, bool inlineData, bool copyTextures, nlohmann::json& buffers,
  nlohmann::json& bufferViews)
{
  std::string gltfRelativeTexturePath =
    vtksys::SystemTools::RelativePath(gltfFullDir, textureFullPath);
  // if inline then base64 encode the data. In this case we need to read the texture
  std::string result;
  std::string mimeType;
  unsigned int byteLength = 0;
  if (inlineData)
  {
    vtkSmartPointer<vtkTexture> t;
    vtkSmartPointer<vtkImageData> id;

    auto textureReader = SetupTextureReader(textureFullPath);
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
    if (copyTextures)
    {
      std::vector<std::string> paths = vtksys::SystemTools::SplitString(textureFullPath);
      vtksys::SystemTools::CopyFileAlways(
        textureFullPath, gltfFullDir + "/" + paths[paths.size() - 1]);
      result = paths[paths.size() - 1];
    }
    else
    {
      // otherwise we only refer to the image file.
      result = gltfRelativeTexturePath;
    }
    // byte length
    vtksys::ifstream textureStream(textureFullPath.c_str(), ios::binary);
    if (textureStream.fail())
    {
      return mimeType; /* empty mimeType signals error*/
    }
    textureStream.seekg(0, ios::end);
    byteLength = textureStream.tellg();
    // mimeType from extension
    mimeType = GetMimeType(textureFullPath);
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

int CopyStream(std::istream& in, std::ostream& out)
{
  const int BUF_SIZE = 4096;
  char buf[BUF_SIZE];
  int streamSize = 0;
  do
  {
    in.read(&buf[0], BUF_SIZE);
    out.write(&buf[0], in.gcount());
    streamSize += in.gcount();
  } while (in.gcount() == BUF_SIZE);
  return streamSize;
}

std::string WriteTextureBufferAndView(const std::string& textureFullPath,
  nlohmann::json& bufferViews, ostream& out, size_t* currentBufferOffset)
{
  std::string result;
  std::string mimeType;
  int byteLength = 0;

  // otherwise we only refer to the image file.
  result = textureFullPath;
  vtksys::ifstream textureStream(textureFullPath.c_str(), ios::binary);
  if (textureStream.fail())
  {
    return mimeType; /* empty mimeType signals error*/
  }
  // copy texture to the output
  byteLength = CopyStream(textureStream, out);
  // mimeType from extension
  mimeType = GetMimeType(textureFullPath);

  nlohmann::json view;

  // write the buffer views
  view["buffer"] = 0;
  view["byteOffset"] = *currentBufferOffset;
  view["byteLength"] = byteLength;
  bufferViews.emplace_back(view);
  *currentBufferOffset += byteLength;
  return mimeType;
}

void WriteBufferAndView(vtkDataArray* inda, nlohmann::json& bufferViews, ostream& out,
  size_t* currentBufferOffset, int bufferViewTarget)
{
  vtkDataArray* da = inda;

  // gltf does not support doubles so handle that
  if (inda->GetDataType() == VTK_DOUBLE)
  {
    da = vtkFloatArray::New();
    da->DeepCopy(inda);
  }

  vtkGLTFWriterUtils::WriteValues(da, out);

  nlohmann::json buffer;
  nlohmann::json view;

  unsigned int count = da->GetNumberOfTuples() * da->GetNumberOfComponents();
  unsigned int byteLength = da->GetElementComponentSize() * count;

  // write the buffer views
  view["buffer"] = 0;
  view["byteOffset"] = *currentBufferOffset;
  view["byteLength"] = byteLength;
  view["target"] = bufferViewTarget;
  bufferViews.emplace_back(view);

  // delete double to float conversion array
  if (da != inda)
  {
    da->Delete();
  }
  *currentBufferOffset += byteLength;
}

void WriteBufferAndView(vtkDataArray* da, const char* fileName, bool inlineData,
  nlohmann::json& buffers, nlohmann::json& bufferViews, bool binary, ostream& out,
  size_t* currentBufferOffset)
{
  if (binary)
  {
    WriteBufferAndView(da, bufferViews, out, currentBufferOffset, GLTF_ARRAY_BUFFER);
  }
  else
  {
    vtkGLTFWriterUtils::WriteBufferAndView(
      da, fileName, inlineData, buffers, bufferViews, GLTF_ARRAY_BUFFER);
  }
}

void WriteCellBufferAndView(
  vtkCellArray* ca, nlohmann::json& bufferViews, ostream& out, size_t* currentBufferOffset)
{
  vtkNew<vtkUnsignedIntArray> ia;
  vtkIdType npts;
  const vtkIdType* indx;
  for (ca->InitTraversal(); ca->GetNextCell(npts, indx);)
  {
    for (int j = 0; j < npts; ++j)
    {
      unsigned int value = static_cast<unsigned int>(indx[j]);
      ia->InsertNextValue(value);
    }
  }

  WriteBufferAndView(ia, bufferViews, out, currentBufferOffset, GLTF_ELEMENT_ARRAY_BUFFER);
}

void WriteCellBufferAndView(vtkCellArray* ca, const char* fileName, bool inlineData,
  nlohmann::json& buffers, nlohmann::json& bufferViews, bool binary, ostream& out,
  size_t* currentBufferOffset)
{
  if (binary)
  {
    WriteCellBufferAndView(ca, bufferViews, out, currentBufferOffset);
  }
  else
  {
    vtkGLTFWriterUtils::WriteCellBufferAndView(ca, fileName, inlineData, buffers, bufferViews);
  }
}

void WriteMesh(nlohmann::json& accessors, nlohmann::json& buffers, nlohmann::json& bufferViews,
  nlohmann::json& meshes, nlohmann::json& nodes, vtkPolyData* pd, const char* fileName,
  bool inlineData, bool saveNormal, bool saveBatchId, bool saveActivePointColor,
  bool structuralMetadataExtension, ostream& output, bool binary, size_t* currentBufferOffset)
{
  vtkNew<vtkTriangleFilter> trif;
  trif->SetInputData(pd);
  trif->Update();
  vtkPolyData* tris = trif->GetOutput();

  // write the point locations
  size_t pointAccessor = 0;
  {
    vtkDataArray* da = tris->GetPoints()->GetData();
    WriteBufferAndView(
      da, fileName, inlineData, buffers, bufferViews, binary, output, currentBufferOffset);
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
    WriteBufferAndView(
      da, fileName, inlineData, buffers, bufferViews, binary, output, currentBufferOffset);

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
    WriteBufferAndView(
      flipY, fileName, inlineData, buffers, bufferViews, binary, output, currentBufferOffset);
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
    WriteCellBufferAndView(
      da, fileName, inlineData, buffers, bufferViews, binary, output, currentBufferOffset);

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
    WriteCellBufferAndView(
      da, fileName, inlineData, buffers, bufferViews, binary, output, currentBufferOffset);

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

    if (structuralMetadataExtension)
    {
      aprim["extensions"] = { { "EXT_structural_metadata", { { "propertyTextures", { 0 } } } } };
    }

    vtkCellArray* da = tris->GetPolys();
    WriteCellBufferAndView(
      da, fileName, inlineData, buffers, bufferViews, binary, output, currentBufferOffset);

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
  nlohmann::json& samplers, nlohmann::json& images, bool inlineData, bool copyTextures,
  std::map<std::string, size_t>& textureMap, const char* textureBaseDirectory,
  const std::string& textureFileName, const char* gltfFileName, bool binary, ostream& out,
  size_t* currentBufferOffset)
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
    std::string mimeType = binary
      ? WriteTextureBufferAndView(textureFullPath, bufferViews, out, currentBufferOffset)
      : WriteTextureBufferAndView(
          gltfFullDir, textureFullPath, inlineData, copyTextures, buffers, bufferViews);
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

std::vector<std::string> vtkGLTFWriter::GetFieldAsStringVector(vtkDataObject* obj, const char* name)
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

  std::string extension = vtksys::SystemTools::GetFilenameLastExtension(this->FileName);
  if (extension == ".glb")
  {
    this->Binary = true;
  }

  // try opening the files
  output.open(this->FileName, ios::binary);
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

  nlohmann::json extensions;
  if (this->PropertyTextureFile)
  {
    vtksys::ifstream propertyTextureStream(this->PropertyTextureFile, ios::binary);
    if (propertyTextureStream.good())
    {
      try
      {
        extensions = nlohmann::json::parse(propertyTextureStream);
      }
      catch (nlohmann::json::parse_error& ex)
      {
        vtkLog(ERROR, << "Parse error " << this->PropertyTextureFile << "at byte " << ex.byte);
      }
    }
    else
    {
      vtkLog(WARNING, "Error: Cannot open property texture file: " << this->PropertyTextureFile);
    }
  }

  auto buildingIt = vtk::TakeSmartPointer(mb->NewTreeIterator());
  buildingIt->VisitOnlyLeavesOff();
  buildingIt->TraverseSubTreeOff();

  bool foundVisibleProp = false;
  if (this->RelativeCoordinates)
  {
    rendererNode["translation"] = { bounds[0], bounds[2], bounds[4] };
  }
  size_t binChunkOffset = 0;
  // all buildings
  std::string binChunkPath = vtksys::SystemTools::GetFilenamePath(this->FileName) + "/binChunk.bin";
  vtksys::ofstream binChunkOut;
  if (this->Binary)
  {
    binChunkOut.open(binChunkPath.c_str(), ios::binary);
  }
  for (buildingIt->InitTraversal(); !buildingIt->IsDoneWithTraversal(); buildingIt->GoToNextItem())
  {
    auto building = vtkMultiBlockDataSet::SafeDownCast(buildingIt->GetCurrentDataObject());
    // all parts of a buildings
    auto it = vtk::TakeSmartPointer(building->NewIterator());
    for (it->InitTraversal(); !it->IsDoneWithTraversal(); it->GoToNextItem())
    {
      vtkSmartPointer<vtkPolyData> pd = vtkPolyData::SafeDownCast(it->GetCurrentDataObject());
      if (pd)
      {
        if (pd->GetNumberOfCells() > 0)
        {
          if (this->RelativeCoordinates)
          {
            vtkNew<vtkTransform> transform;
            transform->Translate(-bounds[0], -bounds[2], -bounds[4]);
            vtkNew<vtkTransformFilter> transformFilter;
            transformFilter->SetTransform(transform);
            transformFilter->SetInputDataObject(pd);
            transformFilter->Update();
            pd = vtkPolyData::SafeDownCast(transformFilter->GetOutput());
          }
          foundVisibleProp = true;
          WriteMesh(accessors, buffers, bufferViews, meshes, nodes, pd, this->FileName,
            this->InlineData, this->SaveNormal, this->SaveBatchId, this->SaveActivePointColor,
            !extensions.empty(), binChunkOut, this->Binary, &binChunkOffset);
          rendererNode["children"].emplace_back(nodes.size() - 1);
          size_t oldTextureCount = textures.size();
          std::vector<std::string> textureFileNames = GetFieldAsStringVector(pd, "texture_uri");
          if (this->SaveTextures)
          {
            for (size_t i = 0; i < textureFileNames.size(); ++i)
            {
              std::string textureFileName = textureFileNames[i];
              WriteTexture(buffers, bufferViews, textures, samplers, images, this->InlineData,
                this->CopyTextures, textureMap, this->TextureBaseDirectory, textureFileName,
                this->FileName, this->Binary, binChunkOut, &binChunkOffset);
            }
          }
          if (this->Binary)
          {
            // pad at 4 bytes for the next mesh
            // accessor total byteOffset has to be a multiple of componentType length
            size_t paddingSizeNextMesh = GetPaddingAt4Bytes(binChunkOffset);
            if (paddingSizeNextMesh)
            {
              char paddingBIN[3] = { 0, 0, 0 };
              binChunkOut.write(paddingBIN, paddingSizeNextMesh);
              binChunkOffset += paddingSizeNextMesh;
            }
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
  binChunkOut.close();

  // only write the camera if we had visible nodes
  if (foundVisibleProp)
  {
    WriteCamera(cameras, ren);
    nodes.emplace_back(anode);
    rendererNode["children"].emplace_back(nodes.size() - 1);
    nodes.emplace_back(rendererNode);
    topNodes.push_back(nodes.size() - 1);
  }

  if (this->Binary)
  {
    // in this case there is only one buffer
    nlohmann::json buffer;
    buffer["byteLength"] = binChunkOffset;
    buffers.emplace_back(buffer);
  }

  nlohmann::json root;
  nlohmann::json asset;
  asset["generator"] = "VTK";
  asset["version"] = "2.0";
  if (!extensions.empty())
  {
    root["extensions"] = extensions;
    root["extensionsUsed"].push_back("EXT_structural_metadata");
  }
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

  if (this->Binary)
  {
    // header
    std::string rootString = root.dump();
    size_t paddingSizeJSON = GetPaddingAt4Bytes(rootString.size());
    size_t paddingSizeBIN = GetPaddingAt4Bytes(binChunkOffset);
    FileHeader header(static_cast<uint32_t>(
      12 + 8 + rootString.size() + paddingSizeJSON + 8 + binChunkOffset + paddingSizeBIN));
    vtkByteSwap::SwapWrite4LERange(&header, 3, &output);
    // JSON
    ChunkHeader jsonChunkHeader;
    jsonChunkHeader.SetTypeJSON(static_cast<uint32_t>(rootString.size() + paddingSizeJSON));
    vtkByteSwap::SwapWrite4LERange(&jsonChunkHeader, 2, &output);
    output.write(rootString.c_str(), rootString.size());
    std::string paddingJSON = "   "; // max possible padding = 3 space characters
    output.write(paddingJSON.c_str(), paddingSizeJSON);
    // BIN
    ChunkHeader binChunkHeader;
    binChunkHeader.SetTypeBIN(static_cast<uint32_t>(binChunkOffset + paddingSizeBIN));
    vtkByteSwap::SwapWrite4LERange(&binChunkHeader, 2, &output);
    vtksys::ifstream binChunkIn(binChunkPath.c_str(), ios::binary);
    CopyStream(binChunkIn, output);
    char paddingBIN[3] = { 0, 0, 0 };
    output.write(paddingBIN, paddingSizeBIN);
    binChunkIn.close();
    vtksys::SystemTools::RemoveFile(binChunkPath);
  }
  else
  {
    output << std::setw(4) << root;
  }
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
VTK_ABI_NAMESPACE_END
