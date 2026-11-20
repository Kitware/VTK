// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUVolumeRayCastMapperInput.h"

#include "vtkWebGPUVolumeRayCastMapper.h"

#include "vtkAbstractMapper.h"
#include "vtkAlgorithm.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <limits>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
//------------------------------------------------------------------------------
// Copy one brick and its one voxel apron, repeating the edge where the apron would leave the
// volume. The apron lets the shader interpolate inside a brick without reaching into a
// neighbouring slot, so a brick boundary leaves no seam. \a source holds the block of the volume
// starting at \a blockOrigin that is \a blockDimensions in size: the whole volume, or the piece of
// it a streamed brick was read as.
template <typename T, typename D>
void ExtractBrick(const T* source, int components, int channels, const int dimensions[3],
  int brickSize, const int brick[3], int slotSize, D* destination, const int blockOrigin[3],
  const int blockDimensions[3])
{
  const auto pick = [](int value, int last) { return std::clamp(value, 0, last); };
  const int copied = std::min(components, channels);
  for (int z = 0; z < slotSize; ++z)
  {
    const int sz = pick(brick[2] * brickSize + z - 1, dimensions[2] - 1) - blockOrigin[2];
    for (int y = 0; y < slotSize; ++y)
    {
      const int sy = pick(brick[1] * brickSize + y - 1, dimensions[1] - 1) - blockOrigin[1];
      const vtkIdType row =
        (static_cast<vtkIdType>(sz) * blockDimensions[1] + sy) * blockDimensions[0];
      D* out = destination + (static_cast<vtkIdType>(z) * slotSize + y) * slotSize * channels;
      for (int x = 0; x < slotSize; ++x)
      {
        const int sx = pick(brick[0] * brickSize + x - 1, dimensions[0] - 1) - blockOrigin[0];
        const vtkIdType tuple = (row + sx) * components;
        for (int c = 0; c < copied; ++c)
        {
          out[x * channels + c] = static_cast<D>(source[tuple + c]);
        }
        for (int c = copied; c < channels; ++c)
        {
          out[x * channels + c] = D(0);
        }
      }
    }
  }
}

constexpr int WholeOrigin[3] = { 0, 0, 0 };

//------------------------------------------------------------------------------
// Scalar extremes of the cells that lie wholly inside the voxels [\a low, \a high], read from a
// streamed block. A cell reaching outside is left unmeasured rather than half measured: it could
// otherwise look empty and keep the brick that holds the rest of it from ever being read.
template <typename T>
void MeasureBlockCells(const T* texels, int components, int component, const int blockOrigin[3],
  const int blockDimensions[3], const int dimensions[3], int cellSize, const int cellDimensions[3],
  const int low[3], const int high[3], std::vector<float>& minimum, std::vector<float>& maximum,
  std::vector<std::size_t>* measured)
{
  int first[3];
  int last[3];
  for (int i = 0; i < 3; ++i)
  {
    first[i] = low[i] / cellSize;
    last[i] = std::min(high[i] / cellSize, cellDimensions[i] - 1);
  }
  for (int cz = first[2]; cz <= last[2]; ++cz)
  {
    const int z0 = cz * cellSize;
    const int z1 = std::min(z0 + cellSize, dimensions[2] - 1);
    if (z0 < low[2] || z1 > high[2])
    {
      continue;
    }
    for (int cy = first[1]; cy <= last[1]; ++cy)
    {
      const int y0 = cy * cellSize;
      const int y1 = std::min(y0 + cellSize, dimensions[1] - 1);
      if (y0 < low[1] || y1 > high[1])
      {
        continue;
      }
      for (int cx = first[0]; cx <= last[0]; ++cx)
      {
        const int x0 = cx * cellSize;
        const int x1 = std::min(x0 + cellSize, dimensions[0] - 1);
        if (x0 < low[0] || x1 > high[0])
        {
          continue;
        }
        float lowest = std::numeric_limits<float>::max();
        float highest = std::numeric_limits<float>::lowest();
        for (int z = z0; z <= z1; ++z)
        {
          for (int y = y0; y <= y1; ++y)
          {
            const vtkIdType row = (static_cast<vtkIdType>(z - blockOrigin[2]) * blockDimensions[1] +
                                    (y - blockOrigin[1])) *
              blockDimensions[0];
            for (int x = x0; x <= x1; ++x)
            {
              const float value =
                static_cast<float>(texels[(row + x - blockOrigin[0]) * components + component]);
              lowest = std::min(lowest, value);
              highest = std::max(highest, value);
            }
          }
        }
        const std::size_t index =
          (static_cast<std::size_t>(cz) * cellDimensions[1] + cy) * cellDimensions[0] + cx;
        minimum[index] = lowest;
        maximum[index] = highest;
        if (measured != nullptr)
        {
          measured->push_back(index);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
// Hidden flag and label of every voxel of the same brick, taken from a host side array laid out
// like the volume.
void ExtractMask(const unsigned char* source, const int dimensions[3], int brickSize,
  const int brick[3], int slotSize, unsigned char* destination)
{
  const auto pick = [](int value, int last) { return std::clamp(value, 0, last); };
  for (int z = 0; z < slotSize; ++z)
  {
    const int sz = pick(brick[2] * brickSize + z - 1, dimensions[2] - 1);
    for (int y = 0; y < slotSize; ++y)
    {
      const int sy = pick(brick[1] * brickSize + y - 1, dimensions[1] - 1);
      const vtkIdType row = (static_cast<vtkIdType>(sz) * dimensions[1] + sy) * dimensions[0];
      unsigned char* out = destination + (static_cast<vtkIdType>(z) * slotSize + y) * slotSize * 2;
      for (int x = 0; x < slotSize; ++x)
      {
        const vtkIdType at = (row + pick(brick[0] * brickSize + x - 1, dimensions[0] - 1)) * 2;
        out[x * 2] = source[at];
        out[x * 2 + 1] = source[at + 1];
      }
    }
  }
}
} // anonymous namespace

//------------------------------------------------------------------------------
vtkImageData* vtkWebGPUVolumeRayCastMapperInput::ReadExtent(
  vtkAlgorithm* source, int sourcePort, const int extent[6])
{
  vtkNew<vtkInformation> request;
  request->Set(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent, 6);
  vtkNew<vtkInformationVector> requests;
  requests->SetInformationObject(sourcePort, request);
  if (!source->Update(sourcePort, requests))
  {
    return nullptr;
  }
  auto* block = vtkImageData::SafeDownCast(source->GetOutputDataObject(sourcePort));
  if (block == nullptr)
  {
    return nullptr;
  }
  // A source may hand back more than was asked for, never less.
  const int* got = block->GetExtent();
  for (int i = 0; i < 3; ++i)
  {
    if (got[2 * i] > extent[2 * i] || got[2 * i + 1] < extent[2 * i + 1])
    {
      return nullptr;
    }
  }
  return block;
}

//------------------------------------------------------------------------------
bool vtkWebGPUVolumeRayCastMapperInput::FetchMask(
  const int brick[3], int slotSize, unsigned char* voxels)
{
  if (this->Hidden.empty() || slotSize <= 2)
  {
    return false;
  }
  ExtractMask(this->Hidden.data(), this->Dimensions, slotSize - 2, brick, slotSize, voxels);
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUVolumeRayCastMapperInput::FetchStreamedBrick(
  const int brick[3], int slotSize, void* voxels)
{
  if (this->Source == nullptr || this->Mapper == nullptr || this->Scalars == nullptr ||
    slotSize <= 2 || this->CellSize <= 0)
  {
    return false;
  }
  const int brickSize = slotSize - 2;

  // The voxels the slot needs, its apron included, in the volume's own index space and then in
  // the source's.
  int low[3];
  int high[3];
  int extent[6];
  for (int i = 0; i < 3; ++i)
  {
    low[i] = std::clamp(brick[i] * brickSize - 1, 0, this->Dimensions[i] - 1);
    high[i] = std::clamp(brick[i] * brickSize + brickSize, 0, this->Dimensions[i] - 1);
    extent[2 * i] = this->WholeExtent[2 * i] + low[i];
    extent[2 * i + 1] = this->WholeExtent[2 * i] + high[i];
  }
  vtkImageData* block = ReadExtent(this->Source, this->SourcePort, extent);
  if (block == nullptr)
  {
    return false;
  }
  int cellFlag = 0;
  vtkDataArray* scalars = vtkAbstractMapper::GetScalars(block, this->Mapper->GetScalarMode(),
    this->Mapper->GetArrayAccessMode(), this->Mapper->GetArrayId(), this->Mapper->GetArrayName(),
    cellFlag);
  // The source changed what it produces under the mapper; the next frame will notice and start
  // over, so this brick is simply left out.
  if (scalars == nullptr || cellFlag != 0 ||
    scalars->GetDataType() != this->Scalars->GetDataType() ||
    scalars->GetNumberOfComponents() != this->Scalars->GetNumberOfComponents())
  {
    return false;
  }

  const int* got = block->GetExtent();
  const int blockOrigin[3] = { got[0] - this->WholeExtent[0], got[2] - this->WholeExtent[2],
    got[4] - this->WholeExtent[4] };
  const int blockDimensions[3] = { got[1] - got[0] + 1, got[3] - got[2] + 1, got[5] - got[4] + 1 };
  const int components = scalars->GetNumberOfComponents();
  const int channels = this->PoolChannels;
  auto aos = scalars->ToAOSDataArray();
  const void* source = aos->GetVoidPointer(0); // NOLINT(bugprone-unsafe-functions)

  switch (scalars->GetDataType())
  {
    vtkTemplateMacro(
      if (this->BytePool) {
        ExtractBrick(static_cast<const VTK_TT*>(source), components, channels, this->Dimensions,
          brickSize, brick, slotSize, static_cast<unsigned char*>(voxels), blockOrigin,
          blockDimensions);
      } else {
        ExtractBrick(static_cast<const VTK_TT*>(source), components, channels, this->Dimensions,
          brickSize, brick, slotSize, static_cast<float*>(voxels), blockOrigin, blockDimensions);
      });
    default:
      return false;
  }

  // The brick is also the first look anyone has had at its cells.
  for (int component = 0; component < this->NumberOfComponents; ++component)
  {
    const int read = std::min(component, components - 1);
    switch (scalars->GetDataType())
    {
      vtkTemplateMacro(MeasureBlockCells(static_cast<const VTK_TT*>(source), components, read,
        blockOrigin, blockDimensions, this->Dimensions, this->CellSize, this->CellDimensions, low,
        high, this->CellMinimum[component], this->CellMaximum[component],
        component == 0 ? &this->PendingCells : nullptr));
    }
  }
  return true;
}

//------------------------------------------------------------------------------
bool vtkWebGPUVolumeRayCastMapperInput::FetchBrick(const int brick[3], int slotSize, void* voxels)
{
  if (this->Streaming)
  {
    return this->FetchStreamedBrick(brick, slotSize, voxels);
  }
  vtkDataArray* scalars = this->AOSScalars;
  if (scalars == nullptr || slotSize <= 2)
  {
    return false;
  }
  // A slot is the brick plus a one voxel apron on each side.
  const int brickSize = slotSize - 2;
  const int components = scalars->GetNumberOfComponents();
  // One channel for a single component volume with nothing riding along, four for anything more,
  // padded where there is less to put in them.
  const int channels = this->PoolChannels;
  void* source = scalars->GetVoidPointer(0); // NOLINT(bugprone-unsafe-functions)

  switch (scalars->GetDataType())
  {
    vtkTemplateMacro(
      if (this->BytePool) {
        ExtractBrick(static_cast<const VTK_TT*>(source), components, channels, this->Dimensions,
          brickSize, brick, slotSize, static_cast<unsigned char*>(voxels), WholeOrigin,
          this->Dimensions);
      } else {
        ExtractBrick(static_cast<const VTK_TT*>(source), components, channels, this->Dimensions,
          brickSize, brick, slotSize, static_cast<float*>(voxels), WholeOrigin, this->Dimensions);
      });
    default:
      return false;
  }

  if (this->AOSYAxisScalars != nullptr && channels > 1)
  {
    // Lay the second axis into the channel next to the scalars, one brick at a time like them.
    std::vector<float> axis(static_cast<std::size_t>(slotSize) * slotSize * slotSize);
    void* yaxis = this->AOSYAxisScalars->GetVoidPointer(0); // NOLINT(bugprone-unsafe-functions)
    const int yComponents = this->AOSYAxisScalars->GetNumberOfComponents();
    switch (this->AOSYAxisScalars->GetDataType())
    {
      vtkTemplateMacro(ExtractBrick(static_cast<const VTK_TT*>(yaxis), yComponents, 1,
        this->Dimensions, brickSize, brick, slotSize, axis.data(), WholeOrigin, this->Dimensions));
      default:
        return false;
    }
    const std::size_t count = axis.size();
    if (this->BytePool)
    {
      auto* out = static_cast<unsigned char*>(voxels);
      for (std::size_t i = 0; i < count; ++i)
      {
        out[i * channels + 1] = static_cast<unsigned char>(axis[i]);
      }
    }
    else
    {
      auto* out = static_cast<float*>(voxels);
      for (std::size_t i = 0; i < count; ++i)
      {
        out[i * channels + 1] = axis[i];
      }
    }
  }
  return true;
}

VTK_ABI_NAMESPACE_END
