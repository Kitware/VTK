#include <algorithm>

#include "vtkBlockSortHelper.h"
#include "vtkCamera.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkMatrix3x3.h"
#include "vtkMatrix4x4.h"
#include "vtkNew.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkOpenGLState.h"
#include "vtkRectilinearGrid.h"
#include "vtkRenderer.h"
#include "vtkTextureObject.h"
#include "vtkUniformGrid.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVolumeProperty.h"
#include "vtkVolumeTexture.h"
#include "vtk_glew.h"

vtkVolumeTexture::vtkVolumeTexture()
  : HandleLargeDataTypes(false)
  , InterpolationType(vtkTextureObject::Linear)
  , Texture(nullptr)
  , CurrentBlockIdx(0)
  , StreamBlocks(false)
  , Scalars(nullptr)
{
  this->Partitions[0] = this->Partitions[1] = this->Partitions[2] = 1;

  this->ScalarRange[0][0] = this->ScalarRange[0][1] = 0.f;
  this->ScalarRange[1][0] = this->ScalarRange[1][1] = 0.f;
  this->ScalarRange[2][0] = this->ScalarRange[2][1] = 0.f;
  this->ScalarRange[3][0] = this->ScalarRange[3][1] = 0.f;

  this->Scale[0] = 1.0f;
  this->Bias[0] = 0.0f;
  this->Scale[1] = 1.0f;
  this->Bias[1] = 0.0f;
  this->Scale[2] = 1.0f;
  this->Bias[2] = 0.0f;
  this->Scale[3] = 1.0f;
  this->Bias[3] = 0.0f;

  this->CellToPointMatrix->Identity();
  this->AdjustedTexMin[0] = this->AdjustedTexMin[1] = this->AdjustedTexMin[2] = 0.0f;
  this->AdjustedTexMin[3] = 1.0f;
  this->AdjustedTexMax[0] = this->AdjustedTexMax[1] = this->AdjustedTexMax[2] = 1.0f;
  this->AdjustedTexMax[3] = 1.0f;
}

//------------------------------------------------------------------------------
vtkVolumeTexture::~vtkVolumeTexture()
{
  this->ClearBlocks();
}

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkVolumeTexture);

//------------------------------------------------------------------------------
bool vtkVolumeTexture::LoadVolume(vtkRenderer* ren, vtkDataSet* data, vtkDataArray* scalars,
  int const isCell, int const interpolation)
{
  this->ClearBlocks();
  this->Scalars = scalars;
  this->IsCellData = isCell;
  this->InterpolationType = interpolation;
  vtkImageData* imData = vtkImageData::SafeDownCast(data);
  vtkRectilinearGrid* rGrid = vtkRectilinearGrid::SafeDownCast(data);
  if (imData)
  {
    imData->GetExtent(this->FullExtent.GetData());
  }
  else if (rGrid)
  {
    rGrid->GetExtent(this->FullExtent.GetData());
  }

  // Setup partition blocks
  if (this->Partitions[0] > 1 || this->Partitions[1] > 1 || this->Partitions[2] > 1)
  {
    // TODO: Partitions are only supported for image data input for now.
    if (!imData)
    {

      vtkErrorMacro(<< "Partitioning only supported for vtkImageData input right now!");
      return false;
    }
    this->SplitVolume(imData, this->Partitions);
  }
  else // Single block
  {
    if (this->IsCellData == 1)
    {
      this->AdjustExtentForCell(this->FullExtent);
    }
    if (imData)
    {
      vtkImageData* singleBlock = nullptr;
      if (vtkUniformGrid* ugData = vtkUniformGrid::SafeDownCast(data))
      {
        singleBlock = vtkUniformGrid::New();
        singleBlock->ShallowCopy(ugData);
      }
      else
      {
        singleBlock = vtkImageData::New();
        singleBlock->ShallowCopy(imData);
      }
      singleBlock->SetExtent(this->FullExtent.GetData());
      this->ImageDataBlocks.push_back(singleBlock);
    }
    else if (rGrid)
    {
      vtkRectilinearGrid* singleBlock = vtkRectilinearGrid::New();
      singleBlock->ShallowCopy(rGrid);
      singleBlock->SetExtent(this->FullExtent.GetData());
      this->ImageDataBlocks.push_back(singleBlock);
    }
  }

  // Get default formats from vtkTextureObject
  if (!this->Texture)
  {
    this->Texture = vtkSmartPointer<vtkTextureObject>::New();
    this->Texture->SetContext(vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
  }
  if (rGrid)
  {
    if (!this->CoordsTex)
    {
      this->CoordsTex = vtkSmartPointer<vtkTextureObject>::New();
      this->CoordsTex->SetContext(vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
    }
  }
  if (data->GetPointGhostArray() || data->GetCellGhostArray())
  {
    this->BlankingTex = vtkSmartPointer<vtkTextureObject>::New();
    this->BlankingTex->SetContext(vtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));
  }

  int scalarType = this->Scalars->GetDataType();
  int noOfComponents = this->Scalars->GetNumberOfComponents();

  unsigned int format = this->Texture->GetDefaultFormat(scalarType, noOfComponents, false);
  unsigned int internalFormat =
    this->Texture->GetDefaultInternalFormat(scalarType, noOfComponents, false);
  int type = this->Texture->GetDefaultDataType(scalarType);

  // Resolve the appropriate texture format from the array properties
  this->SelectTextureFormat(format, internalFormat, type, scalarType, noOfComponents);
  this->CreateBlocks(format, internalFormat, type);

  // If there is a single block, load it right away since GetNextBlock() does not
  // load if streaming is disabled.
  if (this->ImageDataBlocks.size() == 1)
  {
    VolumeBlock* onlyBlock = this->SortedVolumeBlocks.at(0);
    return this->LoadTexture(this->InterpolationType, onlyBlock);
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::SetInterpolation(int const interpolation)
{
  this->InterpolationType = interpolation;

  if (!this->StreamBlocks)
  {
    this->Texture->Activate();
    this->Texture->SetMagnificationFilter(interpolation);
    this->Texture->SetMinificationFilter(interpolation);
  }
}

//------------------------------------------------------------------------------
vtkVolumeTexture::VolumeBlock* vtkVolumeTexture::GetNextBlock()
{
  this->CurrentBlockIdx++;
  // All blocks were already rendered
  if (this->SortedVolumeBlocks.size() <= this->CurrentBlockIdx)
  {
    this->CurrentBlockIdx = 0;
    return nullptr;
  }
  VolumeBlock* block = this->SortedVolumeBlocks[this->CurrentBlockIdx];

  // Load current block
  if (this->StreamBlocks)
  {
    this->LoadTexture(this->InterpolationType, block);
  }

  return block;
}

//------------------------------------------------------------------------------
vtkVolumeTexture::VolumeBlock* vtkVolumeTexture::GetCurrentBlock()
{
  return this->SortedVolumeBlocks[this->CurrentBlockIdx];
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::CreateBlocks(
  unsigned int const format, unsigned int const internalFormat, int const type)
{
  // Pre compute array size
  this->FullSize[0] = this->FullExtent[1] - this->FullExtent[0] + 1;
  this->FullSize[1] = this->FullExtent[3] - this->FullExtent[2] + 1;
  this->FullSize[2] = this->FullExtent[5] - this->FullExtent[4] + 1;

  size_t const numBlocks = this->ImageDataBlocks.size();
  for (size_t i = 0; i < numBlocks; i++)
  {
    vtkDataSet* dataset = this->ImageDataBlocks.at(i);
    vtkImageData* imData = vtkImageData::SafeDownCast(dataset);
    vtkRectilinearGrid* rGrid = vtkRectilinearGrid::SafeDownCast(dataset);
    int* ext = nullptr;
    if (imData)
    {
      ext = imData->GetExtent();
    }
    else if (rGrid)
    {
      ext = rGrid->GetExtent();
    }
    Size3 const texSize = this->ComputeBlockSize(ext);
    VolumeBlock* block = new VolumeBlock(dataset, this->Texture, texSize);

    // Compute tuple index (array aligned in x -> Y -> Z)
    // index = z0 * Dx * Dy + y0 * Dx + x0
    block->TupleIndex =
      ext[4] * this->FullSize[0] * this->FullSize[1] + ext[2] * this->FullSize[0] + ext[0];

    this->ImageDataBlockMap[dataset] = block;
    this->ComputeBounds(block);
    this->UpdateTextureToDataMatrix(block);
  }
  this->ComputeCellToPointMatrix(this->FullExtent.GetData());

  // Format texture
  this->Texture->SetFormat(format);
  this->Texture->SetInternalFormat(internalFormat);
  this->Texture->SetDataType(type);

  // Sorting is skipped when handling a single block, so here the block vector
  // is initialized
  if (this->ImageDataBlocks.size() == 1)
  {
    this->SortedVolumeBlocks.push_back(this->ImageDataBlockMap[this->ImageDataBlocks.at(0)]);
  }
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::AdjustExtentForCell(Size6& extent)
{
  int i = 1;
  while (i < 6)
  {
    extent[i]--;
    i += 2;
  }
}

//------------------------------------------------------------------------------
vtkVolumeTexture::Size3 vtkVolumeTexture::ComputeBlockSize(int* extent)
{
  int i = 0;
  Size3 texSize;
  while (i < 3)
  {
    texSize[i] = extent[2 * i + 1] - extent[2 * i] + 1;
    ++i;
  }
  return texSize;
}

//------------------------------------------------------------------------------
bool vtkVolumeTexture::LoadTexture(int const interpolation, VolumeBlock* volBlock)
{
  int const noOfComponents = this->Scalars->GetNumberOfComponents();
  int scalarType = this->Scalars->GetDataType();

  auto dataSet = volBlock->DataSet;
  auto imBlock = vtkImageData::SafeDownCast(dataSet);
  auto rgBlock = vtkRectilinearGrid::SafeDownCast(dataSet);
  int blockExt[6];
  if (imBlock)
  {
    imBlock->GetExtent(blockExt);
  }
  else if (rgBlock)
  {
    rgBlock->GetExtent(blockExt);
  }
  Size3 const& blockSize = volBlock->TextureSize;
  vtkTextureObject* texture = volBlock->TextureObject;
  vtkIdType const& tupleIdx = volBlock->TupleIndex;

  auto ostate = texture->GetContext()->GetState();

  bool success = true;
  if (!this->HandleLargeDataTypes)
  {
    // Adjust strides used by OpenGL to load the data (X and Y strides in case the
    // texture had to be split on those axis).
    bool const useXStride = blockSize[0] != this->FullSize[0];
    if (useXStride)
    {
      ostate->vtkglPixelStorei(GL_UNPACK_ROW_LENGTH, this->FullSize[0]);
    }

    bool const useYStride = blockSize[1] != this->FullSize[1];
    if (useYStride)
    {
      ostate->vtkglPixelStorei(GL_UNPACK_IMAGE_HEIGHT, this->FullSize[1]);
    }

    // Account for component offset
    // index = ( z0 * Dx * Dy + y0 * Dx + x0 ) * numComp
    vtkIdType const dataIdx = tupleIdx * noOfComponents;
    void* dataPtr = this->Scalars->GetVoidPointer(dataIdx);

    if (this->StreamBlocks)
    {
      success = texture->Create3DFromRaw(
        blockSize[0], blockSize[1], blockSize[2], noOfComponents, scalarType, dataPtr);
    }
    else
    {
      success = SafeLoadTexture(
        texture, blockSize[0], blockSize[1], blockSize[2], noOfComponents, scalarType, dataPtr);
    }
    texture->Activate();
    texture->SetWrapS(vtkTextureObject::ClampToEdge);
    texture->SetWrapT(vtkTextureObject::ClampToEdge);
    texture->SetWrapR(vtkTextureObject::ClampToEdge);
    texture->SetMagnificationFilter(interpolation);
    texture->SetMinificationFilter(interpolation);
    texture->SetBorderColor(0.0f, 0.0f, 0.0f, 0.0f);

    if (useXStride)
    {
      ostate->vtkglPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    }

    if (useYStride)
    {
      ostate->vtkglPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);
    }
  }
  else // Handle 64-bit types
  {
    // 64-bit types are cast to float and then streamed slice by slice into
    // GPU memory. Assumes GL_ARB_texture_non_power_of_two is available.

    scalarType = VTK_FLOAT;
    if (this->StreamBlocks)
    {
      success = texture->Create3DFromRaw(
        blockSize[0], blockSize[1], blockSize[2], noOfComponents, scalarType, nullptr);
    }
    else
    {
      success = SafeLoadTexture(
        texture, blockSize[0], blockSize[1], blockSize[2], noOfComponents, scalarType, nullptr);
    }
    texture->Activate();
    texture->SetWrapS(vtkTextureObject::ClampToEdge);
    texture->SetWrapT(vtkTextureObject::ClampToEdge);
    texture->SetWrapR(vtkTextureObject::ClampToEdge);
    texture->SetMagnificationFilter(interpolation);
    texture->SetMinificationFilter(interpolation);
    texture->SetBorderColor(0.0f, 0.0f, 0.0f, 0.0f);

    vtkFloatArray* sliceArray = vtkFloatArray::New();
    sliceArray->SetNumberOfComponents(noOfComponents);
    sliceArray->SetNumberOfTuples(blockSize[0] * blockSize[1]);

    int k = 0;
    vtkIdType const kInc = this->FullSize[0] * this->FullSize[1];
    vtkIdType kOffset = tupleIdx;

    float* tupPtr = new float[noOfComponents];
    while (k < blockSize[2])
    {
      int j = 0;
      vtkIdType jOffset = 0;
      vtkIdType jDestOffset = 0;
      while (j < blockSize[1])
      {
        int i = 0;
        while (i < blockSize[0])
        {
          // Set components
          double* scalarPtr = this->Scalars->GetTuple(kOffset + jOffset + i);
          for (int n = 0; n < noOfComponents; ++n)
          {
            tupPtr[n] = scalarPtr[n] * this->Scale[n] + this->Bias[n];
          }

          sliceArray->SetTuple(jDestOffset + i, tupPtr);
          ++i;
        }

        ++j;
        jOffset += this->FullSize[0];
        jDestOffset += blockSize[0];
      }

      void* slicePtr = static_cast<void*>(sliceArray->GetPointer(0));
      GLint format = texture->GetFormat(scalarType, noOfComponents, false);
      GLenum type = texture->GetDataType(scalarType);
      glTexSubImage3D(
        GL_TEXTURE_3D, 0, 0, 0, k, blockSize[0], blockSize[1], 1, format, type, slicePtr);

      ++k;
      kOffset += kInc;
    }

    delete[] tupPtr;
    sliceArray->Delete();
  }

  if (rgBlock)
  {
    vtkDataArray* xCoords = rgBlock->GetXCoordinates();
    this->CoordsTexSizes[0] = xCoords->GetNumberOfTuples();
    float fRange[2];
    double* r = xCoords->GetFiniteRange(0);
    for (int i = 0; i < 2; ++i)
    {
      fRange[i] = static_cast<float>(r[i]);
    }
    vtkVolumeTexture::GetScaleAndBias(VTK_FLOAT, fRange, this->CoordsScale[0], this->CoordsBias[0]);
    vtkDataArray* yCoords = rgBlock->GetYCoordinates();
    this->CoordsTexSizes[1] = yCoords->GetNumberOfTuples();
    r = yCoords->GetFiniteRange(0);
    for (int i = 0; i < 2; ++i)
    {
      fRange[i] = static_cast<float>(r[i]);
    }
    vtkVolumeTexture::GetScaleAndBias(VTK_FLOAT, fRange, this->CoordsScale[1], this->CoordsBias[1]);
    vtkDataArray* zCoords = rgBlock->GetZCoordinates();
    this->CoordsTexSizes[2] = zCoords->GetNumberOfTuples();
    r = zCoords->GetFiniteRange(0);
    for (int i = 0; i < 2; ++i)
    {
      fRange[i] = static_cast<float>(r[i]);
    }
    vtkVolumeTexture::GetScaleAndBias(VTK_FLOAT, fRange, this->CoordsScale[2], this->CoordsBias[2]);

    vtkNew<vtkFloatArray> coordsArray;
    coordsArray->SetNumberOfComponents(3);
    int numTuples = std::max(this->CoordsTexSizes[0], this->CoordsTexSizes[1]);
    numTuples = std::max(numTuples, this->CoordsTexSizes[2]);
    coordsArray->SetNumberOfTuples(numTuples);
    for (int i = 0; i < this->CoordsTexSizes[0]; ++i)
    {
      coordsArray->SetTypedComponent(i, 0,
        static_cast<float>(xCoords->GetTuple1(i) * this->CoordsScale[0] + this->CoordsBias[0]));
    }
    for (int i = 0; i < this->CoordsTexSizes[1]; ++i)
    {
      coordsArray->SetTypedComponent(i, 1,
        static_cast<float>(yCoords->GetTuple1(i) * this->CoordsScale[1] + this->CoordsBias[1]));
    }
    for (int i = 0; i < this->CoordsTexSizes[2]; ++i)
    {
      coordsArray->SetTypedComponent(i, 2,
        static_cast<float>(zCoords->GetTuple1(i) * this->CoordsScale[2] + this->CoordsBias[2]));
    }

    void* coordsPtr = static_cast<void*>(coordsArray->GetPointer(0));
    this->CoordsTex->Create1DFromRaw(numTuples, 3, VTK_FLOAT, coordsPtr);
    this->CoordsTex->SetWrapR(vtkTextureObject::ClampToEdge);
    this->CoordsTex->SetWrapS(vtkTextureObject::ClampToEdge);
    this->CoordsTex->SetWrapT(vtkTextureObject::ClampToEdge);
    this->CoordsTex->SetMagnificationFilter(vtkTextureObject::Nearest);
    this->CoordsTex->SetMinificationFilter(vtkTextureObject::Nearest);
    this->CoordsTex->SetBorderColor(0.0f, 0.0f, 0.0f, 0.0f);
  }

  vtkSmartPointer<vtkUnsignedCharArray> ugCellBlankArray = dataSet->GetCellGhostArray();
  vtkSmartPointer<vtkUnsignedCharArray> ugPointBlankArray = dataSet->GetPointGhostArray();
  // Not relying on HasAnyBlankCells because it also does the additional step of checking point
  // ghost array to determine if any cells are blanked.
  bool blankCells = (ugCellBlankArray != nullptr);
  bool blankPoints = (ugPointBlankArray != nullptr);
  if (blankCells || blankPoints)
  {
    vtkNew<vtkUnsignedCharArray> blankingArray;
    auto numComps = (blankCells && blankPoints) ? 2 : 1;
    blankingArray->SetNumberOfComponents(numComps);
    auto numPts = dataSet->GetNumberOfPoints();
    blankingArray->SetNumberOfTuples(numPts);
    blankingArray->FillValue(0);

    auto blankingArrayRange = vtk::DataArrayTupleRange(blankingArray);
    if (blankPoints)
    {
      const auto blankPointsRange = vtk::DataArrayValueRange<1>(ugPointBlankArray);
      int d0 = (blockSize[0] - this->IsCellData) * (blockSize[1] - this->IsCellData);
      int ptId, cellId;
      for (int k = 0; k < blockSize[2]; ++k)
      {
        for (int j = 0; j < blockSize[1]; ++j)
        {
          for (int i = 0; i < blockSize[0]; ++i)
          {
            cellId = k * d0 + j * (blockSize[0] - this->IsCellData) + i;
            ptId = k * (blockSize[0]) * (blockSize[1]) + j * (blockSize[0]) + i;
            blankingArrayRange[cellId][0] = blankPointsRange[ptId];
          }
        }
      }
    }

    if (blankCells)
    {
      int isPointData = this->IsCellData ? 0 : 1;
      int comp = blankPoints ? 1 : 0;
      int d0 = (blockSize[0] - isPointData) * (blockSize[1] - isPointData);
      int d01 = (blockSize[0]) * (blockSize[1]);
      const auto blankCellsRange = vtk::DataArrayValueRange<1>(ugCellBlankArray);
      int ptId, cellId;
      for (int k = 0; k < blockSize[2] - isPointData; ++k)
      {
        for (int j = 0; j < blockSize[1] - isPointData; ++j)
        {
          for (int i = 0; i < blockSize[0] - isPointData; ++i)
          {
            ptId = k * d01 + j * (blockSize[0]) + i;
            cellId = k * d0 + j * (blockSize[0] - isPointData) + i;
            if (isPointData)
            {
              auto kc = (k >= (blockSize[2] - 1) ? blockSize[2] - 2 : k);
              auto jc = (j >= (blockSize[1] - 1) ? blockSize[1] - 2 : j);
              auto ic = (i >= (blockSize[0] - 1) ? blockSize[0] - 2 : i);
              cellId = kc * d0 + jc * (blockSize[0] - 1) + ic;
            }
            blankingArrayRange[ptId][comp] = blankCellsRange[cellId];
          }
        }
      }
    }

    // Since this is a pseudo-bit array i.e. values either 0 or 255, skip scale and bias
    // computation
    this->BlankingTex->Create3DFromRaw(blockSize[0], blockSize[1], blockSize[2], numComps,
      VTK_UNSIGNED_CHAR, &blankingArrayRange[0][0]);
    this->BlankingTex->SetWrapR(vtkTextureObject::ClampToEdge);
    this->BlankingTex->SetWrapS(vtkTextureObject::ClampToEdge);
    this->BlankingTex->SetWrapT(vtkTextureObject::ClampToEdge);
    this->BlankingTex->SetMagnificationFilter(vtkTextureObject::Nearest);
    this->BlankingTex->SetMinificationFilter(vtkTextureObject::Nearest);
    this->BlankingTex->SetBorderColor(0.0f, 0.0f, 0.0f, 0.0f);
  }

  texture->Deactivate();
  this->UploadTime.Modified();

  return success;
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::ReleaseGraphicsResources(vtkWindow* win)
{
  if (this->Texture)
  {
    this->Texture->ReleaseGraphicsResources(win);
    this->Texture = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::ClearBlocks()
{
  if (this->ImageDataBlocks.empty())
  {
    return;
  }

  size_t const numBlocks = this->ImageDataBlocks.size();
  for (size_t i = 0; i < numBlocks; i++)
  {
    this->ImageDataBlocks.at(i)->Delete();
    delete this->SortedVolumeBlocks.at(i);
  }

  this->CurrentBlockIdx = 0;
  this->ImageDataBlocks.clear();
  this->SortedVolumeBlocks.clear();
  this->ImageDataBlockMap.clear();
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::SplitVolume(vtkImageData* imageData, Size3 const& part)
{
  Size6& fullExt = this->FullExtent;
  double const numBlocks_x = part[0];
  double const numBlocks_y = part[1];
  double const numBlocks_z = part[2];
  double const deltaX = (fullExt[1] - fullExt[0]) / numBlocks_x;
  double const deltaY = (fullExt[3] - fullExt[2]) / numBlocks_y;
  double const deltaZ = (fullExt[5] - fullExt[4]) / numBlocks_z;
  unsigned int const numBlocks = static_cast<unsigned int>(numBlocks_x * numBlocks_y * numBlocks_z);

  this->ImageDataBlocks = std::vector<vtkDataSet*>();
  this->ImageDataBlocks.reserve(numBlocks);
  this->SortedVolumeBlocks.reserve(numBlocks);

  for (int k = 0; k < static_cast<int>(numBlocks_z); k++)
  {
    for (int j = 0; j < static_cast<int>(numBlocks_y); j++)
    {
      for (int i = 0; i < static_cast<int>(numBlocks_x); i++)
      {
        Size6 ext;
        ext[0] = fullExt[0] + i * deltaX;
        ext[1] = fullExt[0] + (i + 1) * deltaX;
        ext[2] = fullExt[2] + j * deltaY;
        ext[3] = fullExt[2] + (j + 1) * deltaY;
        ext[4] = fullExt[4] + k * deltaZ;
        ext[5] = fullExt[4] + (k + 1) * deltaZ;

        // Adjust extents depending on the data representation (cell or point) and
        // compute texture size.
        if (this->IsCellData == 1)
        {
          this->AdjustExtentForCell(ext);
        }

        // Create a proxy vtkImageData object for each block
        vtkImageData* block = vtkImageData::New();
        block->ShallowCopy(imageData);
        block->SetExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
        this->ImageDataBlocks.push_back(block);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::GetScaleAndBias(
  const int scalarType, float* scalarRange, float& scale, float& bias)
{
  scale = 1.0f;
  bias = 0.0f;
  double glScale = 1.0;
  double glBias = 0.0;

  switch (scalarType)
  {
    case VTK_UNSIGNED_CHAR:
      glScale = 1.0 / (VTK_UNSIGNED_CHAR_MAX + 1);
      glBias = 0.0;
      break;
    case VTK_SIGNED_CHAR:
      glScale = 2.0 / (VTK_UNSIGNED_CHAR_MAX + 1);
      glBias = -1.0 - VTK_SIGNED_CHAR_MIN * glScale;
      break;
    case VTK_SHORT:
      glScale = 2.0 / (VTK_UNSIGNED_SHORT_MAX + 1);
      glBias = -1.0 - VTK_SHORT_MIN * glScale;
      break;
    case VTK_UNSIGNED_SHORT:
      glScale = 1.0 / (VTK_UNSIGNED_SHORT_MAX + 1);
      glBias = 0.0;
      break;
    case VTK_CHAR:
    case VTK_BIT:
    case VTK_ID_TYPE:
    case VTK_STRING:
      // not supported
      assert("check: impossible case" && 0);
      break;
  }

  double glRange[2];
  for (int i = 0; i < 2; ++i)
  {
    glRange[i] = scalarRange[i] * glScale + glBias;
  }
  glRange[1] = (glRange[1] == glRange[0] ? glRange[0] + 1e-6 : glRange[1]);
  scale = static_cast<float>(1.0 / (glRange[1] - glRange[0]));
  bias = static_cast<float>(0.0 - glRange[0] * scale);
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::SelectTextureFormat(unsigned int& format, unsigned int& internalFormat,
  int& type, int const scalarType, int const noOfComponents)
{
  bool supportsFloat = true;
  this->HandleLargeDataTypes = false;

  switch (scalarType)
  {
    case VTK_FLOAT:
      if (supportsFloat)
      {
        switch (noOfComponents)
        {
          case 1:
            internalFormat = GL_R32F;
            format = GL_RED;
            break;
          case 2:
            internalFormat = GL_RG32F;
            format = GL_RG;
            break;
          case 3:
            internalFormat = GL_RGB32F;
            format = GL_RGB;
            break;
          case 4:
            internalFormat = GL_RGBA32F;
            format = GL_RGBA;
            break;
        }
      }
      else
      {
        switch (noOfComponents)
        {
          case 1:
            internalFormat = GL_RED;
            format = GL_RED;
            break;
          case 2:
            internalFormat = GL_RG;
            format = GL_RG;
            break;
          case 3:
            internalFormat = GL_RGB;
            format = GL_RGB;
            break;
          case 4:
            internalFormat = GL_RGBA;
            format = GL_RGBA;
            break;
        }
      }
      break;
    case VTK_UNSIGNED_CHAR:
    case VTK_SIGNED_CHAR:
    case VTK_SHORT:
    case VTK_UNSIGNED_SHORT:
      // Nothing to be done
      break;
    case VTK_INT:
    case VTK_DOUBLE:
    case VTK_LONG:
    case VTK_LONG_LONG:
    case VTK_UNSIGNED_INT:
    case VTK_UNSIGNED_LONG:
    case VTK_UNSIGNED_LONG_LONG:
      this->HandleLargeDataTypes = true;
      type = GL_FLOAT;
      switch (noOfComponents)
      {
        case 1:
          if (supportsFloat)
          {
            internalFormat = GL_R32F;
          }
          else
          {
            internalFormat = GL_RED;
          }
          format = GL_RED;
          break;
        case 2:
          internalFormat = GL_RG;
          format = GL_RG;
          break;
        case 3:
          internalFormat = GL_RGB;
          format = GL_RGB;
          break;
        case 4:
          internalFormat = GL_RGBA;
          format = GL_RGBA;
          break;
      }
      break;
    case VTK_CHAR:
    case VTK_BIT:
    case VTK_ID_TYPE:
    case VTK_STRING:
    default:
      assert("check: impossible case" && 0);
      break;
  }

  // Cache the array's scalar range
  for (int n = 0; n < noOfComponents; ++n)
  {
    double* range = this->Scalars->GetFiniteRange(n);
    for (int i = 0; i < 2; ++i)
    {
      this->ScalarRange[n][i] = static_cast<float>(range[i]);
    }
  }

  // Pixel Transfer NI to LUT Tex.Coord. [0, 1]
  //
  // NP = P * scale + bias
  // Given two point matches a,b to c,d the formulas are:
  // scale = (d - c) / (b - a)
  // bias = c - a * scale
  // For unsigned/float types c is zero.
  int const components = vtkMath::Min(noOfComponents, 4);
  for (int n = 0; n < components; n++)
  {
    this->GetScaleAndBias(scalarType, this->ScalarRange[n], this->Scale[n], this->Bias[n]);
  }
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::UpdateVolume(vtkVolumeProperty* property)
{
  if (property->GetMTime() > this->UpdateTime.GetMTime())
  {
    int const newInterp = property->GetInterpolationType();
    this->UpdateInterpolationType(newInterp);
  }

  this->UpdateTime.Modified();
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::UpdateInterpolationType(int const interpolation)
{
  if (interpolation == VTK_LINEAR_INTERPOLATION &&
    this->InterpolationType != vtkTextureObject::Linear)
  {
    this->SetInterpolation(vtkTextureObject::Linear);
  }
  else if (interpolation == VTK_NEAREST_INTERPOLATION &&
    this->InterpolationType != vtkTextureObject::Nearest)
  {
    this->SetInterpolation(vtkTextureObject::Nearest);
  }
  else if (interpolation != VTK_LINEAR_INTERPOLATION && interpolation != VTK_NEAREST_INTERPOLATION)
  {
    std::cerr << "Interpolation type not supported in this mapper." << std::endl;
  }
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::SortBlocksBackToFront(vtkRenderer* ren, vtkMatrix4x4* volumeMat)
{
  if (this->ImageDataBlocks.size() > 1)
  {
    vtkBlockSortHelper::BackToFront<vtkImageData> sortBlocks(ren, volumeMat);
    vtkBlockSortHelper::Sort(
      this->ImageDataBlocks.begin(), this->ImageDataBlocks.end(), sortBlocks);

    size_t const numBlocks = this->ImageDataBlocks.size();
    this->SortedVolumeBlocks.clear();
    this->SortedVolumeBlocks.reserve(numBlocks);
    for (size_t i = 0; i < numBlocks; i++)
    {
      this->SortedVolumeBlocks.push_back(this->ImageDataBlockMap[this->ImageDataBlocks[i]]);
    }

    // Load the first block
    auto firstBlock = this->SortedVolumeBlocks.at(0);
    this->LoadTexture(this->InterpolationType, firstBlock);
  }
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::SetPartitions(int const x, int const y, int const z)
{
  if (x > 0 && y > 0 && z > 0)
  {
    if (x > 1 || y > 1 || z > 1)
      this->StreamBlocks = true;

    this->Partitions[0] = x;
    this->Partitions[1] = y;
    this->Partitions[2] = z;
  }
  else
  {
    this->StreamBlocks = false;
    this->Partitions[0] = this->Partitions[1] = this->Partitions[2] = 1;
  }

  this->Modified();
}

//------------------------------------------------------------------------------
const vtkVolumeTexture::Size3& vtkVolumeTexture::GetPartitions()
{
  return this->Partitions;
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "HandleLargeDataTypes: " << this->HandleLargeDataTypes << '\n';
  os << indent << "GL Scale: " << this->Scale[0] << ", " << this->Scale[1] << ", " << this->Scale[2]
     << ", " << this->Scale[3] << '\n';
  os << indent << "GL Bias: " << this->Bias[0] << ", " << this->Bias[1] << ", " << this->Bias[2]
     << ", " << this->Bias[3] << '\n';
  os << indent << "InterpolationType: " << this->InterpolationType << '\n';
  os << indent << "UploadTime: " << this->UploadTime << '\n';
  os << indent << "CurrentBlockIdx: " << this->CurrentBlockIdx << '\n';
  os << indent << "StreamBlocks: " << this->StreamBlocks << '\n';
}

//------------------------------------------------------------------------------
bool vtkVolumeTexture::AreDimensionsValid(
  vtkTextureObject* texture, int const width, int const height, int const depth)
{
  int const maxSize = texture->GetMaximumTextureSize3D();
  if (width > maxSize || height > maxSize || depth > maxSize)
  {
    std::cout << "ERROR: OpenGL MAX_3D_TEXTURE_SIZE is " << maxSize << "\n";
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
bool vtkVolumeTexture::SafeLoadTexture(vtkTextureObject* texture, int const width, int const height,
  int const depth, int numComps, int dataType, void* dataPtr)
{
  if (!AreDimensionsValid(texture, width, height, depth))
  {
    vtkErrorMacro(<< "Invalid texture dimensions [" << width << ", " << height << ", " << depth
                  << "]");
    return false;
  }

  if (!texture->AllocateProxyTexture3D(width, height, depth, numComps, dataType))
  {
    vtkErrorMacro(<< "Capabilities check via proxy texture 3D allocation "
                     "failed!");
    return false;
  }

  if (!texture->Create3DFromRaw(width, height, depth, numComps, dataType, dataPtr))
  {
    vtkErrorMacro(<< "Texture 3D allocation failed! \n");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::ComputeBounds(VolumeBlock* block)
{
  vtkImageData* imData = vtkImageData::SafeDownCast(block->DataSet);
  vtkRectilinearGrid* rGrid = vtkRectilinearGrid::SafeDownCast(block->DataSet);
  double spacing[3];
  double origin[3];
  double* direction = nullptr;
  if (imData)
  {
    imData->GetSpacing(spacing); /// TODO could be causing inf issue on streaming
    imData->GetExtent(block->Extents);
    imData->GetOrigin(origin);
    direction = imData->GetDirectionMatrix()->GetData();
  }
  else if (rGrid)
  {
    double bounds[6];
    int dims[3];
    rGrid->GetBounds(bounds);
    rGrid->GetDimensions(dims);
    for (int cc = 0; cc < 3; ++cc)
    {
      spacing[cc] = (bounds[2 * cc + 1] - bounds[2 * cc]) / dims[cc];
      origin[cc] = bounds[2 * cc];
    }
    rGrid->GetExtent(block->Extents);
    if (this->IsCellData)
    {
      block->Extents[1]--;
      block->Extents[3]--;
      block->Extents[5]--;
    }
  }

  int swapBounds[3];
  swapBounds[0] = (spacing[0] < 0);
  swapBounds[1] = (spacing[1] < 0);
  swapBounds[2] = (spacing[2] < 0);

  // push corners through matrix to get bounding box
  int iMin, iMax, jMin, jMax, kMin, kMax;
  int* extent = block->Extents;
  iMin = extent[0];
  iMax = extent[1] + this->IsCellData;
  jMin = extent[2];
  jMax = extent[3] + this->IsCellData;
  kMin = extent[4];
  kMax = extent[5] + this->IsCellData;
  int ijkCorners[8][3] = { { iMin, jMin, kMin }, { iMax, jMin, kMin }, { iMin, jMax, kMin },
    { iMax, jMax, kMin }, { iMin, jMin, kMax }, { iMax, jMin, kMax }, { iMin, jMax, kMax },
    { iMax, jMax, kMax } };
  double xMin, xMax, yMin, yMax, zMin, zMax;
  xMin = yMin = zMin = VTK_DOUBLE_MAX;
  xMax = yMax = zMax = VTK_DOUBLE_MIN;
  for (int i = 0; i < 8; ++i)
  {
    int* ijkCorner = ijkCorners[i];
    double* xyz = block->VolumeGeometry + i * 3;
    if (imData)
    {
      vtkImageData::TransformContinuousIndexToPhysicalPoint(
        ijkCorner[0], ijkCorner[1], ijkCorner[2], origin, spacing, direction, xyz);
    }
    else if (rGrid)
    {
      rGrid->GetPoint(ijkCorner[0], ijkCorner[1], ijkCorner[2], xyz);
    }
    if (xyz[0] < xMin)
      xMin = xyz[0];
    if (xyz[0] > xMax)
      xMax = xyz[0];
    if (xyz[1] < yMin)
      yMin = xyz[1];
    if (xyz[1] > yMax)
      yMax = xyz[1];
    if (xyz[2] < zMin)
      zMin = xyz[2];
    if (xyz[2] > zMax)
      zMax = xyz[2];
  }
  block->LoadedBoundsAA[0] = xMin;
  block->LoadedBoundsAA[1] = xMax;
  block->LoadedBoundsAA[2] = yMin;
  block->LoadedBoundsAA[3] = yMax;
  block->LoadedBoundsAA[4] = zMin;
  block->LoadedBoundsAA[5] = zMax;

  // Loaded data represents points
  if (!this->IsCellData)
  {
    if (imData)
    {
      // If spacing is negative, we may have to rethink the equation
      // between real point and texture coordinate...
      block->LoadedBounds[0] =
        origin[0] + static_cast<double>(block->Extents[0 + swapBounds[0]]) * spacing[0];
      block->LoadedBounds[2] =
        origin[1] + static_cast<double>(block->Extents[2 + swapBounds[1]]) * spacing[1];
      block->LoadedBounds[4] =
        origin[2] + static_cast<double>(block->Extents[4 + swapBounds[2]]) * spacing[2];
      block->LoadedBounds[1] =
        origin[0] + static_cast<double>(block->Extents[1 - swapBounds[0]]) * spacing[0];
      block->LoadedBounds[3] =
        origin[1] + static_cast<double>(block->Extents[3 - swapBounds[1]]) * spacing[1];
      block->LoadedBounds[5] =
        origin[2] + static_cast<double>(block->Extents[5 - swapBounds[2]]) * spacing[2];
    }
    else if (rGrid)
    {
      double xyzMin[3], xyzMax[3];
      rGrid->GetPoint(block->Extents[0], block->Extents[2], block->Extents[4], xyzMin);
      rGrid->GetPoint(block->Extents[1], block->Extents[3], block->Extents[5], xyzMax);
      for (int i = 0; i < 3; ++i)
      {
        block->LoadedBounds[2 * i] = xyzMin[i];
        block->LoadedBounds[2 * i + 1] = xyzMax[i];
      }
    }
  }
  // Loaded extents represent cells
  else
  {
    if (imData)
    {
      for (int i = 0; i < 3; ++i)
      {
        block->LoadedBounds[2 * i + swapBounds[i]] =
          origin[i] + (static_cast<double>(block->Extents[2 * i])) * spacing[i];

        block->LoadedBounds[2 * i + 1 - swapBounds[i]] =
          origin[i] + (static_cast<double>(block->Extents[2 * i + 1]) + 1.0) * spacing[i];
      }
    }
    else if (rGrid)
    {
      double xyzMin[3], xyzMax[3];
      rGrid->GetPoint(block->Extents[0], block->Extents[2], block->Extents[4], xyzMin);
      rGrid->GetPoint(block->Extents[1] + 1, block->Extents[3] + 1, block->Extents[5] + 1, xyzMax);
      for (int i = 0; i < 3; ++i)
      {
        block->LoadedBounds[2 * i] = xyzMin[i];
        block->LoadedBounds[2 * i + 1] = xyzMax[i];
      }
    }
  }

  // Update sampling distance
  block->DatasetStepSize[0] = 1.0 / (block->LoadedBounds[1] - block->LoadedBounds[0]);
  block->DatasetStepSize[1] = 1.0 / (block->LoadedBounds[3] - block->LoadedBounds[2]);
  block->DatasetStepSize[2] = 1.0 / (block->LoadedBounds[5] - block->LoadedBounds[4]);

  // Cell step/scale are adjusted per block.
  // Step should be dependent on the bounds and not on the texture size
  // since we can have a non-uniform voxel size / spacing / aspect ratio.
  block->CellStep[0] = (1.f / static_cast<float>(block->Extents[1] - block->Extents[0]));
  block->CellStep[1] = (1.f / static_cast<float>(block->Extents[3] - block->Extents[2]));
  block->CellStep[2] = (1.f / static_cast<float>(block->Extents[5] - block->Extents[4]));

  this->CellSpacing[0] = static_cast<float>(spacing[0]);
  this->CellSpacing[1] = static_cast<float>(spacing[1]);
  this->CellSpacing[2] = static_cast<float>(spacing[2]);
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::UpdateTextureToDataMatrix(VolumeBlock* block)
{
  // take the 0.0 to 1.0 texture coordinates and map them into
  // physical/dataset coordinates.
  vtkImageData* imData = vtkImageData::SafeDownCast(block->DataSet);
  vtkRectilinearGrid* rGrid = vtkRectilinearGrid::SafeDownCast(block->DataSet);

  double origin[3];
  double spacing[3];
  vtkMatrix3x3* directionMat = vtkMatrix3x3::New();
  directionMat->Identity();
  if (imData)
  {
    directionMat->DeepCopy(imData->GetDirectionMatrix()->GetData());
    imData->GetOrigin(origin);
    imData->GetSpacing(spacing);
  }

  auto stepsize = block->DatasetStepSize;
  vtkMatrix4x4* matrix = block->TextureToDataset;
  matrix->Identity();
  double* result = matrix->GetData();

  // Scale diag (1.0 -> world coord width)
  double* direction = directionMat->GetData();
  for (int i = 0; i < 3; ++i)
  {
    result[i * 4] = direction[i * 3] / stepsize[0];
    result[i * 4 + 1] = direction[i * 3 + 1] / stepsize[1];
    result[i * 4 + 2] = direction[i * 3 + 2] / stepsize[2];
  }

  double blockOrigin[3];
  if (imData)
  {
    vtkImageData::TransformContinuousIndexToPhysicalPoint(block->Extents[0], block->Extents[2],
      block->Extents[4], origin, spacing, direction, blockOrigin);
  }
  else if (rGrid)
  {
    rGrid->GetPoint(block->Extents[0], block->Extents[2], block->Extents[4], blockOrigin);
  }

  // Translation vec
  result[3] = blockOrigin[0];
  result[7] = blockOrigin[1];
  result[11] = blockOrigin[2];

  auto matrixInv = block->TextureToDatasetInv.GetPointer();
  matrixInv->DeepCopy(matrix);
  matrixInv->Invert();

  directionMat->Delete();
}

//------------------------------------------------------------------------------
void vtkVolumeTexture::ComputeCellToPointMatrix(int extents[6])
{
  this->CellToPointMatrix->Identity();
  this->AdjustedTexMin[0] = this->AdjustedTexMin[1] = this->AdjustedTexMin[2] = 0.0f;
  this->AdjustedTexMin[3] = 1.0f;
  this->AdjustedTexMax[0] = this->AdjustedTexMax[1] = this->AdjustedTexMax[2] = 1.0f;
  this->AdjustedTexMax[3] = 1.0f;

  if (!this->IsCellData) // point data
  {
    // Extents are one minus the number of elements
    // so we have to add 1 to it to account for
    // number of elements in any cell or point image
    // data.
    float delta[3];
    delta[0] = extents[1] - extents[0] + 1;
    delta[1] = extents[3] - extents[2] + 1;
    delta[2] = extents[5] - extents[4] + 1;

    float min[3];
    min[0] = delta[0] > 0.0 ? 0.5f / delta[0] : 0.5f;
    min[1] = delta[1] > 0.0 ? 0.5f / delta[1] : 0.5f;
    min[2] = delta[2] > 0.0 ? 0.5f / delta[2] : 0.5f;

    float range[3]; // max - min
    range[0] = (delta[0] - 0.5f) / delta[0] - min[0];
    range[1] = (delta[1] - 0.5f) / delta[1] - min[1];
    range[2] = (delta[2] - 0.5f) / delta[2] - min[2];

    this->CellToPointMatrix->SetElement(0, 0, range[0]); // Scale diag
    this->CellToPointMatrix->SetElement(1, 1, range[1]);
    this->CellToPointMatrix->SetElement(2, 2, range[2]);
    this->CellToPointMatrix->SetElement(0, 3, min[0]); // t vector
    this->CellToPointMatrix->SetElement(1, 3, min[1]);
    this->CellToPointMatrix->SetElement(2, 3, min[2]);

    // Adjust limit coordinates for texture access.
    float const zeros[4] = { 0.0f, 0.0f, 0.0f, 1.0f }; // GL tex min
    float const ones[4] = { 1.0f, 1.0f, 1.0f, 1.0f };  // GL tex max
    this->CellToPointMatrix->MultiplyPoint(zeros, this->AdjustedTexMin);
    this->CellToPointMatrix->MultiplyPoint(ones, this->AdjustedTexMax);
  }
}

//------------------------------------------------------------------------------
vtkDataArray* vtkVolumeTexture::GetLoadedScalars()
{
  return this->Scalars;
}
