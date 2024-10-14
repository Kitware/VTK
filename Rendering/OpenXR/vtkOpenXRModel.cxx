// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// VTK_DEPRECATED_IN_9_4_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkOpenXRModel.h"

#include "vtkDataArray.h"
#include "vtkFieldData.h"
#include "vtkFileResourceStream.h"
#include "vtkGLTFReader.h"
#include "vtkGLTFTexture.h"
#include "vtkImageData.h"
#include "vtkIntArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLIndexBufferObject.h"
#include "vtkOpenGLVertexArrayObject.h"
#include "vtkOpenGLVertexBufferObject.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"
#include "vtkTextureObject.h"
#include "vtkTransformTextureCoords.h"
#include "vtkURILoader.h"

#include "vtksys/SystemTools.hxx"

#include <atomic>
#include <thread>
#include <vector>

/*=========================================================================
vtkOpenXRModel
=========================================================================*/
VTK_ABI_NAMESPACE_BEGIN

class vtkOpenXRModel::vtkInternals
{
public:
  vtkInternals() = default;

  //----------------------------------------------------------------------------
  bool TryLoadGLTFModel(const std::string& assetPath)
  {
    if (assetPath.empty())
    {
      vtkDebugWithObjectMacro(nullptr, << "No asset path for model");
      return false;
    }

    vtkNew<vtkURILoader> uriLoader;
    uriLoader->SetBaseDirectory(vtksys::SystemTools::GetFilenamePath(assetPath));
    vtkNew<vtkFileResourceStream> fileStream;

    if (!fileStream->Open(assetPath.c_str()))
    {
      vtkErrorWithObjectMacro(nullptr, << "Unable to open file: " << assetPath.c_str());
      return false;
    }

    vtkNew<vtkGLTFReader> reader;
    reader->SetStream(fileStream);
    reader->SetURILoader(uriLoader);
    reader->Update();

    // GLTF reader produces multiblock dataset, and here we expect the first
    // data object to be a polydata representing the model.
    vtkMultiBlockDataSet* mbds = reader->GetOutput();
    vtkCompositeDataIterator* iter = mbds->NewIterator();
    iter->InitTraversal();
    vtkPolyData* pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
    iter->Delete();

    if (!pd)
    {
      vtkErrorWithObjectMacro(nullptr, "Unable to retrieve polydata from reader");
      return false;
    }

    // Flip the texture coordinates
    vtkNew<vtkTransformTextureCoords> texTransform;
    texTransform->SetInputDataObject(pd);
    texTransform->SetFlipR(true);
    texTransform->Update();

    vtkPolyData* polyData = vtkPolyData::SafeDownCast(texTransform->GetOutput());
    vtkPointData* pointData = polyData->GetPointData();
    vtkDataArray* tCoordsArray = pointData->GetTCoords();

    int numPoints = polyData->GetNumberOfPoints();
    double position[3];
    double tcoord[2];

    // Store the polydata position and tcoords in our vertex attributes buffer
    for (int ptIdx = 0; ptIdx < numPoints; ++ptIdx)
    {
      polyData->GetPoint(ptIdx, position);
      tCoordsArray->GetTuple(ptIdx, tcoord);
      this->ModelVBOData.push_back(static_cast<float>(position[0]));
      this->ModelVBOData.push_back(static_cast<float>(position[1]));
      this->ModelVBOData.push_back(static_cast<float>(position[2]));
      this->ModelVBOData.push_back(static_cast<float>(tcoord[0]));
      this->ModelVBOData.push_back(static_cast<float>(tcoord[1]));
    }

    int numCells = polyData->GetNumberOfCells();
    vtkNew<vtkGenericCell> nextCell;

    // We don't handle non-triangular cells, so if we see any of them, return
    // false to trigger the fallback model.  Otherwise, use the point ids from
    // each cell to build the index buffer object data.
    for (int cellIdx = 0; cellIdx < numCells; ++cellIdx)
    {
      polyData->GetCell(cellIdx, nextCell);

      if (nextCell->GetCellType() != VTK_TRIANGLE)
      {
        this->ModelVBOData.resize(0);
        this->ModelIBOData.resize(0);
        vtkErrorWithObjectMacro(nullptr, << "Cell at index " << cellIdx << " is of type "
                                         << nextCell->GetCellType() << ", but OpenXR controller "
                                         << "models can have only triangular cells.");
        return false;
      }

      vtkIdList* cellPtIds = nextCell->GetPointIds();
      for (vtkIdType ptId = 0; ptId < cellPtIds->GetNumberOfIds(); ++ptId)
      {
        this->ModelIBOData.push_back(cellPtIds->GetId(ptId));
      }
    }

    // GLTF reader produces field data which tells us which texture index
    // corresponds to the base color, which is the only thing we handle
    // here.
    vtkFieldData* fieldData = polyData->GetFieldData();
    vtkIntArray* baseColorIndexArray =
      vtkIntArray::SafeDownCast(fieldData->GetArray("BaseColorTextureIndex"));

    if (!baseColorIndexArray)
    {
      this->ModelVBOData.resize(0);
      this->ModelIBOData.resize(0);
      vtkErrorWithObjectMacro(
        nullptr, << "No BaseColorTextureIndex array, cannot determine base color texture");
      return false;
    }

    int textureIndex = baseColorIndexArray->GetValue(0);
    vtkGLTFTexture* gltfTexture = reader->GetTexture(textureIndex);
    vtkSmartPointer<vtkImageData> baseColorImage = gltfTexture->Image;

    if (!baseColorImage)
    {
      this->ModelVBOData.resize(0);
      this->ModelIBOData.resize(0);
      vtkErrorWithObjectMacro(nullptr, << "No base color image data, texture controller model");
      return false;
    }

    // The last bit is to make sure we can handle the image data containing
    // the base color texture, and then iterate over the image data to
    // populate the color texture buffer used to render the model.
    int extent[6];
    baseColorImage->GetExtent(extent);
    vtkPointData* imgPointData = baseColorImage->GetPointData();
    vtkUnsignedCharArray* imgScalars =
      vtkUnsignedCharArray::SafeDownCast(imgPointData->GetScalars());

    if (!imgScalars)
    {
      this->ModelVBOData.resize(0);
      this->ModelIBOData.resize(0);
      vtkErrorWithObjectMacro(
        nullptr, << "Only unsigned char data type supported for base color image");
      return false;
    }

    int numImgScalarComps = imgScalars->GetNumberOfComponents();

    if (numImgScalarComps != 3)
    {
      this->ModelVBOData.resize(0);
      this->ModelIBOData.resize(0);
      vtkErrorWithObjectMacro(
        nullptr, << "Only 3-component scalars supported for base color image");
      return false;
    }

    int xMin = extent[0];
    int xMax = extent[1];
    int yMin = extent[2];
    int yMax = extent[3];
    vtkIdType scalarIdx;
    unsigned char nextTuple[3];

    for (int y = yMin; y <= yMax; ++y)
    {
      for (int x = xMin; x <= xMax; ++x)
      {
        scalarIdx = baseColorImage->GetScalarIndex(x, y, 0);
        imgScalars->GetTypedTuple(scalarIdx, nextTuple);
        this->TextureData.push_back(nextTuple[0]);
        this->TextureData.push_back(nextTuple[1]);
        this->TextureData.push_back(nextTuple[2]);
        this->TextureData.push_back(255);
      }
    }

    this->TextureDimensions[0] = static_cast<unsigned int>(xMax - xMin + 1);
    this->TextureDimensions[1] = static_cast<unsigned int>(yMax - yMin + 1);

    this->ModelLoaded = true;
    this->ModelLoading = false;

    return true;
  }

  //----------------------------------------------------------------------------
  void AsyncLoad(const std::string& assetPath)
  {
    if (this->TryLoadGLTFModel(assetPath))
    {
      return;
    }

    // for now use a pyramid for a controller
    for (int k = 0; k < 2; k++)
    {
      for (int j = 0; j < 2; j++)
      {
        for (int i = 0; i < 2; i++)
        {
          // 5 cm x 10 cm controller
          this->ModelVBOData.push_back(i * 0.05);
          this->ModelVBOData.push_back(j * 0.05);
          this->ModelVBOData.push_back(k * 0.1);
          // tcoords
          this->ModelVBOData.push_back(0.0);
          this->ModelVBOData.push_back(0.0);
        }
      }
    }

    this->ModelIBOData.push_back(0);
    this->ModelIBOData.push_back(4);
    this->ModelIBOData.push_back(5);

    this->ModelIBOData.push_back(0);
    this->ModelIBOData.push_back(4);
    this->ModelIBOData.push_back(6);

    this->ModelIBOData.push_back(4);
    this->ModelIBOData.push_back(5);
    this->ModelIBOData.push_back(6);

    for (int i = 0; i < 16 * 16; i++)
    {
      this->TextureData.push_back(128);
      this->TextureData.push_back(255);
      this->TextureData.push_back(128);
      this->TextureData.push_back(255);
    }

    this->ModelLoaded = true;
    this->ModelLoading = false;
  }

  std::atomic<bool> ModelLoading{ false };
  std::atomic<bool> ModelLoaded{ false };
  std::vector<float> ModelVBOData;
  std::vector<uint16_t> ModelIBOData;
  std::vector<uint8_t> TextureData;
  unsigned int TextureDimensions[2] = { 16, 16 };
};

vtkStandardNewMacro(vtkOpenXRModel);

//------------------------------------------------------------------------------
vtkOpenXRModel::vtkOpenXRModel()
  : Internal(new vtkOpenXRModel::vtkInternals())
{
  this->Visibility = true;
}

//------------------------------------------------------------------------------
vtkOpenXRModel::~vtkOpenXRModel() = default;

//------------------------------------------------------------------------------
void vtkOpenXRModel::FillModelHelper()
{
  this->ModelVBO->Upload(this->Internal->ModelVBOData, vtkOpenGLBufferObject::ArrayBuffer);
  this->ModelHelper.IBO->Upload(
    this->Internal->ModelIBOData, vtkOpenGLBufferObject::ElementArrayBuffer);
  this->ModelHelper.IBO->IndexCount = this->Internal->ModelIBOData.size();
}

//------------------------------------------------------------------------------
void vtkOpenXRModel::SetPositionAndTCoords()
{
  this->ModelHelper.VAO->Bind();
  vtkShaderProgram* program = this->ModelHelper.Program;
  if (!this->ModelHelper.VAO->AddAttributeArray(
        program, this->ModelVBO, "position", 0, 5 * sizeof(float), VTK_FLOAT, 3, false))
  {
    vtkErrorMacro(<< "Error setting position in shader VAO.");
  }
  if (!this->ModelHelper.VAO->AddAttributeArray(program, this->ModelVBO, "v2TexCoordsIn",
        3 * sizeof(float), 5 * sizeof(float), VTK_FLOAT, 2, false))
  {
    vtkErrorMacro(<< "Error setting tcoords in shader VAO.");
  }
}

//------------------------------------------------------------------------------
void vtkOpenXRModel::CreateTextureObject(vtkOpenGLRenderWindow* win)
{
  this->TextureObject->SetContext(win);
  this->TextureObject->Create2DFromRaw(this->Internal->TextureDimensions[0],
    this->Internal->TextureDimensions[1], 4, VTK_UNSIGNED_CHAR,
    const_cast<void*>(static_cast<const void*>(this->Internal->TextureData.data())));
  this->TextureObject->SetWrapS(vtkTextureObject::ClampToEdge);
  this->TextureObject->SetWrapT(vtkTextureObject::ClampToEdge);

  this->TextureObject->SetMinificationFilter(vtkTextureObject::LinearMipmapLinear);
  this->TextureObject->SetGenerateMipmap(true);
}

//------------------------------------------------------------------------------
void vtkOpenXRModel::LoadModelAndTexture(vtkOpenGLRenderWindow* win)
{
  // if we do not have the model loaded and haven't initiated loading
  if (!this->Internal->ModelLoaded && !this->Internal->ModelLoading)
  {
    this->Internal->ModelLoading = true;
    // loading the models can be slow so do it in a separate thread
    std::thread loader([this] { this->Internal->AsyncLoad(this->GetAssetPath()); });
    loader.detach();
  }

  if (this->Internal->ModelLoaded && !this->Loaded)
  {
    if (!this->Build(win))
    {
      vtkErrorMacro("Unable to create GL model from render model " << this->GetName());
    }
    this->Loaded = true;
  }
}
VTK_ABI_NAMESPACE_END
