// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "Private/vtkWebGPUVolumeRayCastMapperInternals.h"

#include "Private/vtkWebGPUHelpersPrivate.h"
#include "Private/vtkWebGPUVolumeRayCastCoordinatorInternals.h"

#include "vtkAbstractMapper.h"
#include "vtkColorTransferFunction.h"
#include "vtkDataArray.h"
#include "vtkGPUVolumeRayCastMapper.h"
#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkMatrix4x4.h"
#include "vtkMultiVolume.h"
#include "vtkPiecewiseFunction.h"
#include "vtkPointData.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVolume.h"
#include "vtkVolumeProperty.h"
#include "vtkWebGPUConfiguration.h"
#include "vtkWebGPURenderWindow.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <limits>

VTK_ABI_NAMESPACE_BEGIN

namespace
{
constexpr int TableSize = vtkWebGPUVolumeTransferFunctionSize;

//------------------------------------------------------------------------------
std::uint16_t PackHalf(float value)
{
  std::uint32_t bits;
  std::memcpy(&bits, &value, sizeof(bits));
  const std::uint32_t sign = (bits >> 16) & 0x8000u;
  const std::int32_t exponent = static_cast<std::int32_t>((bits >> 23) & 0xFFu) - 127 + 15;
  const std::uint32_t mantissa = bits & 0x7FFFFFu;
  if (exponent <= 0)
  {
    return static_cast<std::uint16_t>(sign);
  }
  if (exponent >= 31)
  {
    return static_cast<std::uint16_t>(sign | 0x7C00u);
  }
  return static_cast<std::uint16_t>(
    sign | (static_cast<std::uint32_t>(exponent) << 10) | (mantissa >> 13));
}

//------------------------------------------------------------------------------
template <typename T>
void CopyFirstComponent(const T* source, int components, vtkIdType count, float* destination)
{
  for (vtkIdType i = 0; i < count; ++i)
  {
    destination[i] = static_cast<float>(source[i * components]);
  }
}

//------------------------------------------------------------------------------
// Scalar extremes of every cell, including one voxel of overlap on the far side so the range also
// covers what interpolation can produce inside the cell.
template <typename T>
void MeasureCells(const T* texels, int components, int component, const int dimensions[3],
  int cellSize, const int cellDimensions[3], std::vector<float>& minimum,
  std::vector<float>& maximum)
{
  const vtkIdType sliceStride = static_cast<vtkIdType>(dimensions[0]) * dimensions[1];
  const std::size_t count =
    static_cast<std::size_t>(cellDimensions[0]) * cellDimensions[1] * cellDimensions[2];
  minimum.assign(count, std::numeric_limits<float>::max());
  maximum.assign(count, std::numeric_limits<float>::lowest());

  for (int bz = 0; bz < cellDimensions[2]; ++bz)
  {
    const int z0 = bz * cellSize;
    const int z1 = std::min(z0 + cellSize, dimensions[2] - 1);
    for (int by = 0; by < cellDimensions[1]; ++by)
    {
      const int y0 = by * cellSize;
      const int y1 = std::min(y0 + cellSize, dimensions[1] - 1);
      for (int bx = 0; bx < cellDimensions[0]; ++bx)
      {
        const int x0 = bx * cellSize;
        const int x1 = std::min(x0 + cellSize, dimensions[0] - 1);

        float low = std::numeric_limits<float>::max();
        float high = std::numeric_limits<float>::lowest();
        for (int z = z0; z <= z1; ++z)
        {
          for (int y = y0; y <= y1; ++y)
          {
            const vtkIdType row = z * sliceStride + static_cast<vtkIdType>(y) * dimensions[0];
            for (int x = x0; x <= x1; ++x)
            {
              const float value = static_cast<float>(texels[(row + x) * components + component]);
              low = std::min(low, value);
              high = std::max(high, value);
            }
          }
        }
        const std::size_t index =
          static_cast<std::size_t>(bz) * cellDimensions[1] * cellDimensions[0] +
          static_cast<std::size_t>(by) * cellDimensions[0] + bx;
        minimum[index] = low;
        maximum[index] = high;
      }
    }
  }
}

//------------------------------------------------------------------------------
// Copy one brick and its one voxel apron, repeating the edge where the apron would leave the
// volume. The apron lets the shader interpolate inside a brick without reaching into a
// neighbouring slot, so a brick boundary leaves no seam.
template <typename T, typename D>
void ExtractBrick(const T* source, int components, int channels, const int dimensions[3],
  int brickSize, const int brick[3], int slotSize, D* destination)
{
  const auto pick = [](int value, int last) { return std::clamp(value, 0, last); };
  const int copied = std::min(components, channels);
  for (int z = 0; z < slotSize; ++z)
  {
    const int sz = pick(brick[2] * brickSize + z - 1, dimensions[2] - 1);
    for (int y = 0; y < slotSize; ++y)
    {
      const int sy = pick(brick[1] * brickSize + y - 1, dimensions[1] - 1);
      const vtkIdType row = (static_cast<vtkIdType>(sz) * dimensions[1] + sy) * dimensions[0];
      D* out = destination + (static_cast<vtkIdType>(z) * slotSize + y) * slotSize * channels;
      for (int x = 0; x < slotSize; ++x)
      {
        const int sx = pick(brick[0] * brickSize + x - 1, dimensions[0] - 1);
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

//------------------------------------------------------------------------------
// Extremes of the table between two entries in constant time, which turns block classification
// from a per block scan into a lookup.
class RangeTable
{
public:
  explicit RangeTable(const std::array<float, TableSize>& values)
  {
    this->Levels = 1;
    while ((1 << this->Levels) <= TableSize)
    {
      ++this->Levels;
    }
    this->Minimum.resize(this->Levels * TableSize);
    this->Maximum.resize(this->Levels * TableSize);
    for (int i = 0; i < TableSize; ++i)
    {
      this->Minimum[i] = values[i];
      this->Maximum[i] = values[i];
    }
    for (int level = 1; level < this->Levels; ++level)
    {
      const int span = 1 << level;
      const int half = span >> 1;
      for (int i = 0; i + span <= TableSize; ++i)
      {
        this->Minimum[level * TableSize + i] = std::min(this->Minimum[(level - 1) * TableSize + i],
          this->Minimum[(level - 1) * TableSize + i + half]);
        this->Maximum[level * TableSize + i] = std::max(this->Maximum[(level - 1) * TableSize + i],
          this->Maximum[(level - 1) * TableSize + i + half]);
      }
    }
  }

  void Query(int first, int last, float& low, float& high) const
  {
    first = std::clamp(first, 0, TableSize - 1);
    last = std::clamp(last, first, TableSize - 1);
    int level = 0;
    while ((1 << (level + 1)) <= (last - first + 1))
    {
      ++level;
    }
    const int offset = level * TableSize;
    const int second = last - (1 << level) + 1;
    low = std::min(this->Minimum[offset + first], this->Minimum[offset + second]);
    high = std::max(this->Maximum[offset + first], this->Maximum[offset + second]);
  }

private:
  int Levels = 1;
  std::vector<float> Minimum;
  std::vector<float> Maximum;
};
} // anonymous namespace

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::ReleaseGraphicsResources()
{
  for (auto& entry : this->InputCache)
  {
    entry.second.CellSize = 0;
    entry.second.DataTime = 0;
    entry.second.TransferFunctionTime = 0;
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUVolumeRayCastMapper::vtkInternals::CollectInputs(vtkVolume* volume)
{
  this->ActivePorts.clear();

  auto* multiVolume = vtkMultiVolume::SafeDownCast(volume);
  const std::vector<int> ports = this->Self->GetPorts();

  for (int port : ports)
  {
    auto* image = vtkImageData::SafeDownCast(this->Self->GetTransformedInput(port));
    if (image == nullptr)
    {
      continue;
    }
    // Not always the active point scalars: the mapper may have been pointed at a named array, or
    // at the cell data.
    int cellFlag = 0;
    vtkDataArray* scalars = vtkAbstractMapper::GetScalars(image, this->Self->GetScalarMode(),
      this->Self->GetArrayAccessMode(), this->Self->GetArrayId(), this->Self->GetArrayName(),
      cellFlag);
    if (scalars == nullptr || cellFlag == 2)
    {
      continue;
    }

    vtkVolume* portVolume = multiVolume != nullptr ? multiVolume->GetVolume(port) : nullptr;
    const auto override = this->PortProperties.find(port);
    vtkVolumeProperty* property =
      override != this->PortProperties.end() ? override->second.Get() : nullptr;
    if (property == nullptr && portVolume != nullptr)
    {
      property = portVolume->GetProperty();
    }
    if (property == nullptr)
    {
      property = volume->GetProperty();
    }
    if (property == nullptr)
    {
      continue;
    }

    // The second axis of a two dimensional transfer function, when the mapper names an array for
    // it. It travels in the second channel of the same bricks rather than in a volume of its own.
    const bool transfer2D = property->GetTransferFunctionMode() == vtkVolumeProperty::TF_2D;
    vtkDataArray* yAxis = nullptr;
    if (transfer2D && this->Self->GetTransfer2DYAxisArray() != nullptr)
    {
      yAxis = image->GetPointData()->GetArray(this->Self->GetTransfer2DYAxisArray());
      if (yAxis == nullptr)
      {
        vtkWarningWithObjectMacro(
          this->Self, << "No array named " << this->Self->GetTransfer2DYAxisArray()
                      << " to give the second axis of the transfer function.");
      }
    }

    const int available = scalars->GetNumberOfComponents();
    const int components = std::min(available, 4);
    bool independent = property->GetIndependentComponents() != 0;
    if (!independent && components != 2 && components != 4)
    {
      independent = true;
    }
    // The intensity blend modes and the acceleration structure look at one component. Independent
    // components pick the one the property weighs most, which is what the OpenGL mapper does.
    int primary = 0;
    if (independent)
    {
      for (int i = 1; i < components; ++i)
      {
        if (property->GetComponentWeight(i) > property->GetComponentWeight(primary))
        {
          primary = i;
        }
      }
    }

    Input& input = this->InputCache[port];
    input.Port = port;
    input.Data = image;
    vtkImageData* mask = this->Self->GetMaskInput();
    if (input.Mask != mask || input.MaskType != this->Self->GetMaskType() ||
      (mask != nullptr && input.MaskTime < mask->GetMTime()))
    {
      input.CellSize = 0;
      input.DataTime = 0;
    }
    if (input.Scalars != scalars || input.CellData != (cellFlag == 1) ||
      input.NumberOfComponents != components || input.PrimaryComponent != primary ||
      input.YAxisScalars != yAxis || input.Transfer2D != transfer2D)
    {
      // A different array, or a different way of reading it, is a different volume as far as the
      // device is concerned.
      input.CellSize = 0;
      input.DataTime = 0;
      input.ColorTable.clear();
    }
    input.Scalars = scalars;
    input.YAxisScalars = yAxis;
    input.Transfer2D = transfer2D;
    input.CellData = cellFlag == 1;
    input.NumberOfComponents = components;
    input.IndependentComponents = independent;
    input.PrimaryComponent = primary;
    input.Property = property;
    input.WorldMatrix = portVolume != nullptr ? portVolume->GetMatrix() : volume->GetMatrix();
    this->ActivePorts.push_back(port);
  }

  // Let go of the state of ports that are no longer connected.
  for (auto it = this->InputCache.begin(); it != this->InputCache.end();)
  {
    const bool active = std::find(this->ActivePorts.begin(), this->ActivePorts.end(), it->first) !=
      this->ActivePorts.end();
    it = active ? std::next(it) : this->InputCache.erase(it);
  }

  return !this->ActivePorts.empty();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateVolumeData(Input& input, int cellSize)
{
  vtkDataArray* scalars = input.Scalars;
  if (input.CellSize == cellSize && input.DataTime >= input.Data->GetMTime())
  {
    return;
  }

  input.Data->GetDimensions(input.Dimensions);
  input.IndexToPhysical->DeepCopy(input.Data->GetIndexToPhysicalMatrix());
  // Placed before the range queries below so that the dimensions are the cell ones when needed.
  if (input.CellData)
  {
    // A cell array has one fewer sample per axis than the points, but still covers the whole
    // grid, so the index to physical transform stays the points' and the sampling shifts half a
    // cell.
    for (int i = 0; i < 3; ++i)
    {
      input.Dimensions[i] = std::max(1, input.Dimensions[i] - 1);
    }
  }
  const int* dimensions = input.Dimensions;
  const vtkIdType count = static_cast<vtkIdType>(dimensions[0]) * dimensions[1] * dimensions[2];
  if (count <= 0)
  {
    return;
  }

  const int available = scalars->GetNumberOfComponents();
  if (available > 4)
  {
    vtkWarningWithObjectMacro(this->Self, << "Input " << input.Port << " has " << available
                                          << " components, only the first four are rendered.");
  }

  // Eight bit data stays eight bit on the device and is scaled back up in the shader. Everything
  // else becomes single precision float, which is exact for the types that fit.
  input.BytePool = scalars->GetDataType() == VTK_UNSIGNED_CHAR &&
    (input.YAxisScalars == nullptr || input.YAxisScalars->GetDataType() == VTK_UNSIGNED_CHAR);
  input.ValueScale = input.BytePool ? 255.0f : 1.0f;
  input.ValueBias = 0.0f;
  // A second axis array takes the channel a second component would have taken.
  input.PoolChannels = (input.NumberOfComponents > 1 || input.YAxisScalars != nullptr) ? 4 : 1;
  for (int component = 0; component < 4; ++component)
  {
    scalars->GetRange(input.DataRange[component], std::min(component, available - 1));
  }
  if (input.YAxisScalars != nullptr)
  {
    input.YAxisScalars->GetRange(input.DataRange[1], 0);
  }

  input.CellSize = cellSize;
  for (int i = 0; i < 3; ++i)
  {
    input.CellDimensions[i] = std::max(1, (dimensions[i] - 1) / cellSize + 1);
  }

  void* source = scalars->GetVoidPointer(0);
  // Every component is measured, because a region that one component maps to nothing may be
  // exactly where another one carries what there is to see.
  for (int component = 0; component < input.NumberOfComponents; ++component)
  {
    const int read = std::min(component, available - 1);
    switch (scalars->GetDataType())
    {
      vtkTemplateMacro(
        MeasureCells(static_cast<const VTK_TT*>(source), available, read, dimensions, cellSize,
          input.CellDimensions, input.CellMinimum[component], input.CellMaximum[component]));
      default:
        vtkErrorWithObjectMacro(
          this->Self, << "Unsupported scalar type on input " << input.Port << ".");
        input.CellSize = 0;
        return;
    }
  }

  this->UpdateHiddenVoxels(input);

  input.Mask = this->Self->GetMaskInput();
  input.MaskType = this->Self->GetMaskType();
  input.MaskTime = input.Mask != nullptr ? input.Mask->GetMTime() : 0;
  input.DataTime = input.Data->GetMTime();
  // The cells were measured again, so their classification is stale.
  input.TransferFunctionTime = 0;
  input.ContentTime.Modified();
  input.BrickTime.Modified();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateHiddenVoxels(Input& input)
{
  input.Hidden.clear();
  input.MaskHidesNeighbours = false;
  input.MaximumLabel = 0;

  const vtkIdType count =
    static_cast<vtkIdType>(input.Dimensions[0]) * input.Dimensions[1] * input.Dimensions[2];
  if (count <= 0)
  {
    return;
  }

  // A binary mask cuts exactly where it says and does not take its neighbours with it. A label map
  // instead names, per voxel, which transfer functions to draw it with, so it hides nothing.
  vtkImageData* mask = this->Self->GetMaskInput();
  if (mask != nullptr)
  {
    const bool binary = this->Self->GetMaskType() == vtkGPUVolumeRayCastMapper::BinaryMaskType;
    vtkDataArray* values = mask->GetPointData()->GetScalars();
    int maskDimensions[3];
    mask->GetDimensions(maskDimensions);
    if (values != nullptr && maskDimensions[0] == input.Dimensions[0] &&
      maskDimensions[1] == input.Dimensions[1] && maskDimensions[2] == input.Dimensions[2])
    {
      input.Hidden.assign(static_cast<std::size_t>(count) * 2, 0);
      for (vtkIdType i = 0; i < count; ++i)
      {
        const double value = values->GetComponent(i, 0);
        if (binary)
        {
          input.Hidden[i * 2] = value == 0.0 ? 255 : 0;
        }
        else
        {
          // Labels ride in a byte, and one that does not fit is one no transfer function was
          // ever going to be found for.
          const int label = static_cast<int>(value);
          if (label > 0 && label < 256)
          {
            input.Hidden[i * 2 + 1] = static_cast<unsigned char>(label);
            input.MaximumLabel = std::max(input.MaximumLabel, label);
          }
        }
      }
    }
    else if (values != nullptr)
    {
      vtkWarningWithObjectMacro(this->Self,
        << "The mask has to have the same dimensions as the volume it masks. Ignoring it.");
    }
  }

  // Ghosted and blanked voxels are not data at all, so anything that interpolates through one is
  // wrong and has to go with it.
  vtkUnsignedCharArray* pointGhosts = input.Data->GetPointGhostArray();
  vtkUnsignedCharArray* cellGhosts = input.Data->GetCellGhostArray();
  if (pointGhosts == nullptr && cellGhosts == nullptr)
  {
    return;
  }
  // A ghosted cell is the sample itself when the array lives on the cells, so it takes nothing
  // else with it; a ghosted point is one corner of the cells around it, and those go too.
  input.MaskHidesNeighbours = !input.CellData;
  if (input.Hidden.empty())
  {
    input.Hidden.assign(static_cast<std::size_t>(count) * 2, 0);
  }

  if (input.CellData)
  {
    // The samples are the cells, so only the cell ghosts have anything to say about them.
    if (cellGhosts != nullptr && cellGhosts->GetNumberOfTuples() >= count)
    {
      for (vtkIdType i = 0; i < count; ++i)
      {
        input.Hidden[i * 2] |= cellGhosts->GetValue(i) != 0 ? 255 : 0;
      }
    }
    return;
  }

  if (pointGhosts != nullptr && pointGhosts->GetNumberOfTuples() >= count)
  {
    for (vtkIdType i = 0; i < count; ++i)
    {
      input.Hidden[i * 2] |= pointGhosts->GetValue(i) != 0 ? 255 : 0;
    }
  }
  if (cellGhosts != nullptr)
  {
    // A point belongs to up to eight cells, and a ghosted cell takes every point of it with it.
    const int* dims = input.Dimensions;
    const int cells[3] = { std::max(dims[0] - 1, 1), std::max(dims[1] - 1, 1),
      std::max(dims[2] - 1, 1) };
    for (int z = 0; z < dims[2]; ++z)
    {
      for (int y = 0; y < dims[1]; ++y)
      {
        for (int x = 0; x < dims[0]; ++x)
        {
          bool hidden = false;
          for (int dz = -1; dz <= 0 && !hidden; ++dz)
          {
            for (int dy = -1; dy <= 0 && !hidden; ++dy)
            {
              for (int dx = -1; dx <= 0 && !hidden; ++dx)
              {
                const int cx = x + dx;
                const int cy = y + dy;
                const int cz = z + dz;
                if (cx < 0 || cy < 0 || cz < 0 || cx >= cells[0] || cy >= cells[1] ||
                  cz >= cells[2])
                {
                  continue;
                }
                const vtkIdType cell = (static_cast<vtkIdType>(cz) * cells[1] + cy) * cells[0] + cx;
                hidden = cell < cellGhosts->GetNumberOfTuples() && cellGhosts->GetValue(cell) != 0;
              }
            }
          }
          if (hidden)
          {
            input.Hidden[((static_cast<vtkIdType>(z) * dims[1] + y) * dims[0] + x) * 2] = 255;
          }
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
bool vtkWebGPUVolumeRayCastMapper::vtkInternals::Input::FetchMask(
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
bool vtkWebGPUVolumeRayCastMapper::vtkInternals::Input::FetchBrick(
  const int brick[3], int slotSize, void* voxels)
{
  vtkDataArray* scalars = this->Scalars;
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
  void* source = scalars->GetVoidPointer(0);

  switch (scalars->GetDataType())
  {
    vtkTemplateMacro(
      if (this->BytePool) {
        ExtractBrick(static_cast<const VTK_TT*>(source), components, channels, this->Dimensions,
          brickSize, brick, slotSize, static_cast<unsigned char*>(voxels));
      } else {
        ExtractBrick(static_cast<const VTK_TT*>(source), components, channels, this->Dimensions,
          brickSize, brick, slotSize, static_cast<float*>(voxels));
      });
    default:
      return false;
  }

  if (this->YAxisScalars != nullptr && channels > 1)
  {
    // Lay the second axis into the channel next to the scalars, one brick at a time like them.
    std::vector<float> axis(static_cast<std::size_t>(slotSize) * slotSize * slotSize);
    void* yaxis = this->YAxisScalars->GetVoidPointer(0);
    const int yComponents = this->YAxisScalars->GetNumberOfComponents();
    switch (this->YAxisScalars->GetDataType())
    {
      vtkTemplateMacro(ExtractBrick(static_cast<const VTK_TT*>(yaxis), yComponents, 1,
        this->Dimensions, brickSize, brick, slotSize, axis.data()));
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

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateTransferFunctions(Input& input)
{
  vtkVolumeProperty* property = input.Property;
  if (!input.ColorTable.empty() && input.TransferFunctionTime >= property->GetMTime())
  {
    return;
  }

  const int components = input.NumberOfComponents;
  // Only independent components have transfer functions of their own. Dependent ones describe
  // one colour between them, and only the first set is consulted.
  const int tables = input.IndependentComponents ? components : 1;

  input.ColorTable.assign(static_cast<std::size_t>(TableSize) * 4 * components, 0);
  input.OpacityTable.assign(static_cast<std::size_t>(TableSize) * 4 * components, 0);
  input.GradientTable.assign(static_cast<std::size_t>(TableSize) * 4 * components, 0);

  for (int component = 0; component < components; ++component)
  {
    const int table = std::min(component, tables - 1);
    const std::size_t base = static_cast<std::size_t>(component) * TableSize * 4;

    vtkColorTransferFunction* rgb =
      property->GetColorChannels(table) == 1 ? nullptr : property->GetRGBTransferFunction(table);
    vtkPiecewiseFunction* gray =
      property->GetColorChannels(table) == 1 ? property->GetGrayTransferFunction(table) : nullptr;
    vtkPiecewiseFunction* scalarOpacity = property->GetScalarOpacity(table);
    vtkPiecewiseFunction* gradientOpacity = property->GetDisableGradientOpacity(table) != 0
      ? nullptr
      : property->GetGradientOpacity(table);

    // Every table is indexed by where the sample sits in the data range, so that is what they are
    // tabulated over. SCALAR, the default, reads the function at the data value; NATIVE reads it at
    // the same fraction of the function's own range.
    double data[2] = { input.DataRange[component][0], input.DataRange[component][1] };
    if (data[1] <= data[0])
    {
      data[1] = data[0] + 1.0;
    }
    input.ScalarRange[component][0] = data[0];
    input.ScalarRange[component][1] = data[1];

    const auto domain = [&data](
                          vtkObject* function, const double* native, int rangeType, double out[2])
    {
      if (function == nullptr || rangeType == vtkGPUVolumeRayCastMapper::SCALAR ||
        native[1] <= native[0])
      {
        out[0] = data[0];
        out[1] = data[1];
      }
      else
      {
        out[0] = native[0];
        out[1] = native[1];
      }
    };

    double colorDomain[2];
    double opacityDomain[2];
    double gradientDomain[2];
    const double* colorNative =
      rgb != nullptr ? rgb->GetRange() : (gray != nullptr ? gray->GetRange() : data);
    domain(rgb != nullptr ? static_cast<vtkObject*>(rgb) : static_cast<vtkObject*>(gray),
      colorNative, this->Self->GetColorRangeType(), colorDomain);
    domain(scalarOpacity, scalarOpacity != nullptr ? scalarOpacity->GetRange() : data,
      this->Self->GetScalarOpacityRangeType(), opacityDomain);
    domain(gradientOpacity, gradientOpacity != nullptr ? gradientOpacity->GetRange() : data,
      this->Self->GetGradientOpacityRangeType(), gradientDomain);

    for (int i = 0; i < TableSize; ++i)
    {
      const double fraction = static_cast<double>(i) / (TableSize - 1);

      double rgbValue[3] = { 1.0, 1.0, 1.0 };
      const double colorAt = colorDomain[0] + fraction * (colorDomain[1] - colorDomain[0]);
      if (rgb != nullptr)
      {
        rgb->GetColor(colorAt, rgbValue);
      }
      else if (gray != nullptr)
      {
        const double level = gray->GetValue(colorAt);
        rgbValue[0] = rgbValue[1] = rgbValue[2] = level;
      }
      input.ColorTable[base + i * 4 + 0] = PackHalf(static_cast<float>(rgbValue[0]));
      input.ColorTable[base + i * 4 + 1] = PackHalf(static_cast<float>(rgbValue[1]));
      input.ColorTable[base + i * 4 + 2] = PackHalf(static_cast<float>(rgbValue[2]));
      input.ColorTable[base + i * 4 + 3] = PackHalf(1.0f);

      const double opacityAt = opacityDomain[0] + fraction * (opacityDomain[1] - opacityDomain[0]);
      const float alpha =
        scalarOpacity != nullptr ? static_cast<float>(scalarOpacity->GetValue(opacityAt)) : 1.0f;
      input.OpacityCurve[component][i] = std::clamp(alpha, 0.0f, 1.0f);
      input.OpacityTable[base + i * 4 + 0] = PackHalf(alpha);
      input.OpacityTable[base + i * 4 + 3] = PackHalf(1.0f);
    }

    // The gradient magnitude is measured against a quarter of the data range, which is the scale
    // vtkVolumeShaderComposer settles on for want of knowing what gradients the data can produce.
    input.GradientRange[component][0] = 0.0;
    input.GradientRange[component][1] = 0.25 * (data[1] - data[0]);

    // The table follows the same scale: it starts at no gradient at all and runs to that quarter.
    double gradientRange[2] = { 0.0, 0.25 * (gradientDomain[1] - gradientDomain[0]) };
    const double gradientStep = (gradientRange[1] - gradientRange[0]) / (TableSize - 1);
    for (int i = 0; i < TableSize; ++i)
    {
      const float value = gradientOpacity != nullptr
        ? static_cast<float>(gradientOpacity->GetValue(gradientRange[0] + i * gradientStep))
        : 1.0f;
      input.GradientTable[base + i * 4 + 0] = PackHalf(value);
      input.GradientTable[base + i * 4 + 3] = PackHalf(1.0f);
    }
  }

  // The second axis array is not a component and so has no tables of its own, but it is still
  // normalized by its own range before it indexes the function.
  if (input.YAxisScalars != nullptr)
  {
    input.ScalarRange[1][0] = input.DataRange[1][0];
    input.ScalarRange[1][1] = input.DataRange[1][1];
    if (input.ScalarRange[1][1] <= input.ScalarRange[1][0])
    {
      input.ScalarRange[1][1] = input.ScalarRange[1][0] + 1.0;
    }
  }

  // A two dimensional transfer function replaces the colour and opacity tables above, indexed by
  // the data range along one axis and by gradient magnitude, or a second array, along the other.
  input.Transfer2DTable.clear();
  if (input.Transfer2D)
  {
    constexpr int size = vtkWebGPUVolumeTransferFunction2DSize;
    input.Transfer2DTable.assign(static_cast<std::size_t>(size) * size * 4 * components, 0);
    for (int component = 0; component < components; ++component)
    {
      vtkImageData* function =
        property->GetTransferFunction2D(input.IndependentComponents ? component : 0);
      if (function == nullptr)
      {
        continue;
      }
      // The function is drawn into an image that does not always name its colours as the active
      // scalars, so fall back to whatever array it does have.
      vtkDataArray* texels = function->GetPointData()->GetScalars();
      if (texels == nullptr && function->GetPointData()->GetNumberOfArrays() > 0)
      {
        texels = function->GetPointData()->GetArray(0);
      }
      if (texels == nullptr)
      {
        vtkWarningWithObjectMacro(this->Self, << "The two dimensional transfer function of input "
                                              << input.Port << " has no values in it.");
        continue;
      }
      int dimensions[3];
      function->GetDimensions(dimensions);
      if (dimensions[0] < 1 || dimensions[1] < 1)
      {
        continue;
      }
      const std::size_t base = static_cast<std::size_t>(component) * size * size * 4;
      const int channels = texels->GetNumberOfComponents();
      // A two dimensional function leaves the one dimensional opacity function unused, so the
      // envelope of the largest opacity over the second axis stands in for it.
      std::vector<float> envelope(static_cast<std::size_t>(size), 0.0f);
      for (int y = 0; y < size; ++y)
      {
        // Nearest resampling onto a fixed grid, which is what the function is drawn on anyway.
        const int sy =
          std::clamp(static_cast<int>(std::lround(y * (dimensions[1] - 1.0) / (size - 1.0))), 0,
            dimensions[1] - 1);
        for (int x = 0; x < size; ++x)
        {
          const int sx =
            std::clamp(static_cast<int>(std::lround(x * (dimensions[0] - 1.0) / (size - 1.0))), 0,
              dimensions[0] - 1);
          const vtkIdType tuple = static_cast<vtkIdType>(sy) * dimensions[0] + sx;
          const std::size_t out = base + (static_cast<std::size_t>(y) * size + x) * 4;
          for (int c = 0; c < 4; ++c)
          {
            const double value = c < channels ? texels->GetComponent(tuple, c) : 1.0;
            input.Transfer2DTable[out + c] = PackHalf(static_cast<float>(value));
          }
          const float alpha =
            channels > 3 ? static_cast<float>(texels->GetComponent(tuple, 3)) : 1.0f;
          envelope[x] = std::max(envelope[x], std::clamp(alpha, 0.0f, 1.0f));
        }
      }
      // The envelope is measured on the function's own grid, so every entry of the table it
      // stands in for has to be given the value nearest it rather than only every fourth one.
      for (int i = 0; i < TableSize; ++i)
      {
        const int x = std::clamp(
          static_cast<int>(std::lround(i * (size - 1.0) / (TableSize - 1.0))), 0, size - 1);
        input.OpacityCurve[component][i] = envelope[x];
      }
    }
  }

  // A label map names transfer functions per label, and a label naming none is drawn with the
  // volume's. One row per label, indexed by the label itself, so labels may be sparse.
  input.LabelTable.clear();
  if (input.MaximumLabel > 0)
  {
    const int rows = input.MaximumLabel + 1;
    input.LabelTable.assign(static_cast<std::size_t>(TableSize) * rows * 4, 0);
    vtkColorTransferFunction* baseRgb =
      property->GetColorChannels(0) == 1 ? nullptr : property->GetRGBTransferFunction(0);
    vtkPiecewiseFunction* baseGray =
      property->GetColorChannels(0) == 1 ? property->GetGrayTransferFunction(0) : nullptr;
    vtkPiecewiseFunction* baseOpacity = property->GetScalarOpacity(0);
    const double* range = input.ScalarRange[0];

    for (int label = 1; label < rows; ++label)
    {
      vtkColorTransferFunction* rgb = property->GetLabelColor(label);
      vtkPiecewiseFunction* opacity = property->GetLabelScalarOpacity(label);
      std::vector<float> colors(static_cast<std::size_t>(TableSize) * 3, 0.0f);
      std::vector<float> alphas(static_cast<std::size_t>(TableSize), 0.0f);
      if (rgb != nullptr)
      {
        rgb->GetTable(range[0], range[1], TableSize, colors.data());
      }
      else if (baseRgb != nullptr)
      {
        baseRgb->GetTable(range[0], range[1], TableSize, colors.data());
      }
      else if (baseGray != nullptr)
      {
        std::vector<float> gray(static_cast<std::size_t>(TableSize), 0.0f);
        baseGray->GetTable(range[0], range[1], TableSize, gray.data());
        for (int i = 0; i < TableSize; ++i)
        {
          colors[i * 3] = colors[i * 3 + 1] = colors[i * 3 + 2] = gray[i];
        }
      }
      (opacity != nullptr ? opacity : baseOpacity)
        ->GetTable(range[0], range[1], TableSize, alphas.data());

      const std::size_t base = static_cast<std::size_t>(label) * TableSize * 4;
      for (int i = 0; i < TableSize; ++i)
      {
        input.LabelTable[base + i * 4 + 0] = PackHalf(colors[i * 3]);
        input.LabelTable[base + i * 4 + 1] = PackHalf(colors[i * 3 + 1]);
        input.LabelTable[base + i * 4 + 2] = PackHalf(colors[i * 3 + 2]);
        input.LabelTable[base + i * 4 + 3] = PackHalf(alphas[i]);
      }
    }
  }

  input.TransferFunctionTime = property->GetMTime();
  input.CellClassification.clear();
  input.ContentTime.Modified();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::UpdateCellClassification(Input& input)
{
  if (!input.CellClassification.empty() || input.CellMinimum[0].empty())
  {
    return;
  }

  const std::size_t cells = input.CellMinimum[0].size();
  input.CellClassification.assign(cells * 2, 0.0f);

  // A cell is only empty when every component says so, and it is as detailed as the component
  // that varies most across it, so each one is asked in turn and the answers are combined.
  const int components = input.NumberOfComponents;
  for (int component = 0; component < components; ++component)
  {
    // Dependent components describe one colour between them: the last carries the opacity, and
    // it is looked up in the first set of tables.
    const int measured = input.IndependentComponents ? component : components - 1;
    const int table = input.IndependentComponents ? component : 0;
    if (!input.IndependentComponents && component > 0)
    {
      break;
    }

    const RangeTable range(input.OpacityCurve[table]);
    const double origin = input.ScalarRange[table][0];
    const double scale = 1.0 / (input.ScalarRange[table][1] - origin);

    for (std::size_t cell = 0; cell < cells; ++cell)
    {
      const double low = std::clamp((input.CellMinimum[measured][cell] - origin) * scale, 0.0, 1.0);
      const double high =
        std::clamp((input.CellMaximum[measured][cell] - origin) * scale, 0.0, 1.0);

      float opacityLow = 0.0f;
      float opacityHigh = 0.0f;
      range.Query(static_cast<int>(std::floor(low * (TableSize - 1))),
        static_cast<int>(std::ceil(high * (TableSize - 1))), opacityLow, opacityHigh);

      // How much the cell varies. Amplified so a cell only counts as featureless when it is
      // almost flat, because coarse steps through a slowly changing region are what bands.
      constexpr float sensitivity = 4.0f;
      const float detail =
        std::clamp(sensitivity * std::max(static_cast<float>(high - low), opacityHigh - opacityLow),
          0.0f, 1.0f);

      input.CellClassification[cell * 2] =
        std::max(input.CellClassification[cell * 2], opacityHigh);
      input.CellClassification[cell * 2 + 1] =
        std::max(input.CellClassification[cell * 2 + 1], detail);
    }
  }

  input.ContentTime.Modified();
}

//------------------------------------------------------------------------------
void vtkWebGPUVolumeRayCastMapper::vtkInternals::SyncDeviceResources(
  vtkWebGPURenderWindow* vtkNotUsed(window), vtkWebGPUVolumeRayCastCoordinator* coordinator,
  vtkVolume* volume)
{
  if (coordinator == nullptr || !this->CollectInputs(volume))
  {
    return;
  }

  for (int port : this->ActivePorts)
  {
    Input& input = this->InputCache[port];
    this->UpdateVolumeData(input, this->Self->GetBrickSize());
    if (input.CellSize <= 0)
    {
      continue;
    }
    this->UpdateTransferFunctions(input);
    this->UpdateCellClassification(input);

    vtkWebGPUVolumeRayCastContribution contribution;
    contribution.Mapper = this->Self;
    contribution.Volume = volume;
    contribution.Port = port;
    contribution.Data = input.Data;
    contribution.IndexToPhysical = input.IndexToPhysical;
    contribution.Property = input.Property;
    contribution.WorldMatrix = input.WorldMatrix;
    contribution.ClippingPlanes = this->Self->GetClippingPlanes();
    contribution.BrickSource = &input;
    contribution.BytePool = input.BytePool;
    contribution.PoolChannels = input.PoolChannels;
    contribution.CellData = input.CellData;
    contribution.HasMask = !input.Hidden.empty();
    contribution.MaskHidesNeighbours = input.MaskHidesNeighbours;
    contribution.MaximumLabel = input.LabelTable.empty() ? 0 : input.MaximumLabel;
    contribution.MaskBlendFactor = this->Self->GetMaskBlendFactor();
    contribution.LabelTable = &input.LabelTable;
    std::copy_n(input.Dimensions, 3, contribution.Dimensions);
    contribution.ValueScale = input.ValueScale;
    contribution.ValueBias = input.ValueBias;
    contribution.NumberOfComponents = input.NumberOfComponents;
    contribution.IndependentComponents = input.IndependentComponents;
    contribution.PrimaryComponent = input.PrimaryComponent;
    for (int component = 0; component < 4; ++component)
    {
      std::copy_n(input.ScalarRange[component], 2, contribution.ScalarRange[component]);
      std::copy_n(input.GradientRange[component], 2, contribution.GradientRange[component]);
    }
    contribution.CellSize = input.CellSize;
    std::copy_n(input.CellDimensions, 3, contribution.CellDimensions);
    contribution.CellClassification = &input.CellClassification;
    contribution.Transfer2D = input.Transfer2D && !input.Transfer2DTable.empty();
    contribution.Transfer2DHasYAxisArray = input.YAxisScalars != nullptr;
    contribution.Transfer2DTable = &input.Transfer2DTable;
    contribution.ColorTable = &input.ColorTable;
    contribution.OpacityTable = &input.OpacityTable;
    contribution.GradientTable = &input.GradientTable;
    contribution.ContentTime = input.ContentTime.GetMTime();
    contribution.BrickTime = input.BrickTime.GetMTime();
    contribution.CellsTime = input.CellsTime.GetMTime();

    if (!coordinator->GetInternals()->Offer(contribution))
    {
      vtkWarningWithObjectMacro(
        this->Self, << "Only " << vtkWebGPUVolumeRayCastCoordinator::MaximumNumberOfVolumes
                    << " volumes can be ray cast together, the rest are not drawn.");
      break;
    }
  }
}

VTK_ABI_NAMESPACE_END
