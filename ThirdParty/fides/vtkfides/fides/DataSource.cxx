//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/DataSource.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/Storage.h>

#include <algorithm>
#include <numeric>

#include <iostream>

#ifdef FIDES_USE_MPI
#include <vtk_mpi.h>
#endif

template <typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& v)
{
  if (!v.empty())
  {
    out << "[";
    std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
    out << "]";
  }
  return out;
}

namespace
{
std::vector<std::string> SplitString(const std::string& input, char sep)
{
  std::vector<std::string> result;
  if (input.empty())
  {
    return result;
  }
  std::string::size_type pos1 = 0;
  std::string::size_type pos2 = input.find(sep, pos1);
  while (pos2 != std::string::npos)
  {
    result.push_back(input.substr(pos1, pos2 - pos1));
    pos1 = pos2 + 1;
    pos2 = input.find(sep, pos1 + 1);
  }
  result.push_back(input.substr(pos1, pos2 - pos1));

  return result;
}

std::string JoinString(const std::vector<std::string>& words, char sep)
{
  std::string result;
  std::size_t i = 0;
  for (; i < words.size() - 1; ++i)
  {
    result += words[i];
    result += sep;
  }
  result += words[i];
  return result;
}
}

namespace fides
{
namespace io
{

void DataSource::SetDataSourceParameters(const DataSourceParams& params)
{
  this->SourceParams = params;
}

void DataSource::SetDataSourceIO(void* io)
{
  if (!io)
  {
    return;
  }
  this->AdiosIO = *(reinterpret_cast<adios2::IO*>(io));
  this->SetupEngine();
  this->OpenSource(this->ReaderID);
}

void DataSource::SetDataSourceIO(const std::string& ioAddress)
{
  long long hexAddress = std::stoll(ioAddress, nullptr, 16);
  this->AdiosIO = *(reinterpret_cast<adios2::IO*>(hexAddress));
  this->SetupEngine();
  this->OpenSource(this->ReaderID);
}

void DataSource::SetupEngine()
{
  auto it = this->SourceParams.find("engine_type");
  std::string engine = "BPFile";
  if (it != this->SourceParams.end())
  {
    engine = it->second;
  }

  if (engine == "BPFile")
  {
    this->AdiosEngineType = EngineType::BPFile;
    this->AdiosIO.SetEngine("BPFile");
  }
  else if (engine == "SST")
  {
    this->AdiosEngineType = EngineType::SST;
    this->AdiosIO.SetEngine("SST");
  }
  else if (engine == "Inline")
  {
    this->AdiosEngineType = EngineType::Inline;
    if (!this->AdiosIO)
    {
      throw std::runtime_error("Inline engine requires passing (to DataSetReader) "
                               "a valid pointer to an adios2::IO object.");
    }
    this->AdiosIO.SetEngine("Inline");
  }
  else
  {
    throw std::runtime_error("parameter engine_type must be BPFile, SST or Inline.");
  }

  it = this->SourceParams.find("verbose");
  if (it != this->SourceParams.end())
  {
    this->AdiosIO.SetParameter("verbose", it->second);
  }
}

std::map<std::string, adios2::Params>::iterator DataSource::FindVariable(
  const std::string& name,
  const fides::metadata::MetaData& selections)
{
  std::string fullVarName = name;
  if (selections.Has(fides::keys::GROUP_SELECTION()))
  {
    const auto& group =
      selections.Get<fides::metadata::String>(fides::keys::GROUP_SELECTION()).Data;
    if (!group.empty())
    {
      fullVarName = group + "/" + name;
    }
  }
  return this->AvailVars.find(fullVarName);
}

std::map<std::string, adios2::Params>::iterator DataSource::FindAttribute(
  const std::string& name,
  const fides::metadata::MetaData& selections)
{
  std::string fullAttrName = name;
  if (selections.Has(fides::keys::GROUP_SELECTION()))
  {
    const auto& group =
      selections.Get<fides::metadata::String>(fides::keys::GROUP_SELECTION()).Data;
    if (!group.empty())
    {
      fullAttrName = group + "/" + name;
    }
  }
  return this->AvailAtts.find(fullAttrName);
}

void DataSource::OpenSource(const std::string& fname, bool useMPI /* = true */)
{
  //if the reader (ADIOS engine) is already been set, do nothing
  if (this->Reader)
  {
    return;
  }

  if (!this->AdiosIO)
  {
    if (!this->Adios)
    {
      //if the factory pointer and the specific IO is empty
      //reset the implementation
#ifdef FIDES_USE_MPI
      if (useMPI)
      {
        this->Adios.reset(new adios2::ADIOS(MPI_COMM_WORLD));
      }
      else
      {
        this->Adios.reset(new adios2::ADIOS);
      }
#else
      // The ADIOS2 docs say "do not use () for the empty constructor"
      // See here: https://adios2.readthedocs.io/en/latest/components/components.html#components-overview
      this->Adios.reset(new adios2::ADIOS);
#endif
    }
    //if the factory is not empty, generate the io used by fides internally
    this->AdiosIO = this->Adios->DeclareIO("adios-io-read");
    this->SetupEngine();
  }
  auto mode = adios2::Mode::Read;
  if (!this->StreamingMode)
  {
#ifdef FIDES_ADIOS_HAS_RANDOM_ACCESS
    mode = adios2::Mode::ReadRandomAccess;
#else
    mode = adios2::Mode::Read;
#endif
  }

  this->Reader = this->AdiosIO.Open(fname, mode);
  this->Refresh();
}

void DataSource::Refresh()
{
  this->AvailVars = this->AdiosIO.AvailableVariables();
  this->AvailAtts = this->AdiosIO.AvailableAttributes();
  // refresh available groups
  this->AvailGroups.clear();
  for (const auto& availItems : { this->AvailAtts, this->AvailVars })
  {
    for (const auto& item : availItems)
    {
      const auto& fullName = item.first;
      auto pieces = SplitString(fullName, '/');
      if (pieces.size() < 2)
      {
        continue;
      }
      const auto lastPiece = pieces.back();
      pieces.pop_back();
      this->AvailGroups[lastPiece].insert(JoinString(pieces, '/'));
    }
  }
}

template <typename VariableType, typename VecType>
vtkm::cont::UnknownArrayHandle AllocateArrayHandle(vtkm::Id bufSize, VariableType*& buffer)
{
  vtkm::cont::ArrayHandleBasic<VecType> arrayHandle;
  arrayHandle.Allocate(bufSize);
  buffer = reinterpret_cast<VariableType*>(arrayHandle.GetWritePointer());
  return arrayHandle;
}

template <typename VariableType, vtkm::IdComponent Dim>
vtkm::cont::UnknownArrayHandle AllocateArrayHandle(const VariableType* vecData, vtkm::Id bufSize)
{
  vtkm::cont::ArrayHandle<VariableType> arrayHandle =
    vtkm::cont::make_ArrayHandle(vecData, bufSize, vtkm::CopyFlag::Off);
  return vtkm::cont::make_ArrayHandleGroupVec<Dim>(arrayHandle);
}

struct FidesArrayMemoryRequirements
{
  /// Total number of elements.
  vtkm::Id Size;
  /// Location of the first element - local to the block.
  adios2::Dims Start;
  /// Number of elements in each dimension - local to the block.
  adios2::Dims Count;
  /// Does this memory describe shared points as well?
  /// This is used to decide whether `SetSelection` of `SetBlockSelection` is called for global
  /// arrays distributed across blocks.
  bool HasSharedPoints;
};

inline std::ostream& operator<<(std::ostream& os, const FidesArrayMemoryRequirements& memReqs)
{
  os << "FidesArrayMemoryRequirements: \n";
  os << "\tSize: " << memReqs.Size << "\n";
  for (size_t dim = 0; dim < memReqs.Start.size(); ++dim)
  {
    os << "\tStart[" << dim << "]: " << memReqs.Start[dim] << "\n";
    os << "\tCount[" << dim << "]: " << memReqs.Count[dim] << "\n";
  }
  os << "\tHasSharedPoints: " << memReqs.HasSharedPoints;
  return os;
}

size_t GetBufferSize(const adios2::Dims& shape)
{
  size_t size = 1;
  for (const auto& n : shape)
  {
    size *= n;
  }
  return size;
}

// Set 1 to enable debug prints of array memory requirements.
#define FidesArrayMemoryRequirements_DEBUG 0

template <typename VariableType,
          typename BlocksInfoType = typename adios2::Variable<VariableType>::Info>
FidesArrayMemoryRequirements GetVariableMemoryRequirements(
  std::vector<BlocksInfoType>& blocksInfo,
  adios2::Variable<VariableType>& varADIOS2,
  size_t blockId,
  bool createSharedPoints = false)
{
  if (blockId >= blocksInfo.size())
  {
    std::stringstream ss;
    ss << __FILE__ << ":" << __LINE__;
    ss << " Cannot read block " << blockId << " for variable " << varADIOS2.Name()
       << "; there are only " << blocksInfo.size() << " blocks.";
    throw std::invalid_argument(ss.str());
  }
  const auto& blockInfo = blocksInfo[blockId];

#if FidesArrayMemoryRequirements_DEBUG
  std::cout << "GetVariableMemoryRequirements for " << varADIOS2.Name() << " in block " << blockId
            << std::endl;
#endif
  FidesArrayMemoryRequirements memoryRequirements = {};
  // sane defaults correspond to whatever the variable and block info have.
  memoryRequirements.Start = blockInfo.Start;
  memoryRequirements.Count = blockInfo.Count;
  memoryRequirements.Size = GetBufferSize(blockInfo.Count);
  memoryRequirements.HasSharedPoints = false;
#if FidesArrayMemoryRequirements_DEBUG
  std::cout << "Default " << memoryRequirements << std::endl;
#endif
  // start and count will be adjusted depending on the value of Variable::ShapeID().
  switch (varADIOS2.ShapeID())
  {
    case adios2::ShapeID::GlobalArray:
    {
      if (createSharedPoints)
      {
        const auto& nDims = blockInfo.Start.size();
        for (size_t dim = 0; dim < nDims; ++dim)
        {
          // grow by one index for all blocks whose start > 0
          const bool extendDimensions = blockInfo.Start[dim] > 0;
          const int indexDelta = static_cast<int>(extendDimensions);
          memoryRequirements.Start[dim] -= indexDelta;
          memoryRequirements.Count[dim] += indexDelta;
          memoryRequirements.HasSharedPoints |= extendDimensions;
        }
        // size will include shared points
        memoryRequirements.Size = GetBufferSize(memoryRequirements.Count);
      }
      break;
    }
    default:
      // all other types return un-modified shape and buffer size.
      break;
  }
  if (memoryRequirements.Size <= 0)
  {
    // ADIOS Dims are size_t, but vtk-m uses signed integers (32- or 64-bit
    // depending on build) for allocating storage for the arrayhandle (num values,
    // not bytes). I think it's unlikely that we'd actually get overflow, but just
    // in case...
    if (sizeof(vtkm::Id) == 4)
    {
      throw std::runtime_error("Overflow in number of values being read detected."
                               "Building VTK-m with VTKm_USE_64BIT_IDS should fix this.");
    }
    throw std::runtime_error("Overflow in number of values being read detected.");
  }
#if FidesArrayMemoryRequirements_DEBUG
  std::cout << "New " << memoryRequirements << std::endl;
#endif
  return memoryRequirements;
}

/// This method makes an appropriate selection on the variable.
///  - applies an extended selection using `SetSelection` if memory requirements deem that's necessary.
///  - otherwise, applies a block selection using `SetBlockSelection`
template <typename VariableType>
void PrepareVariableSelection(adios2::Variable<VariableType>& varADIOS2,
                              const FidesArrayMemoryRequirements& memoryRequirements,
                              const std::size_t blockId)
{
  if (memoryRequirements.HasSharedPoints)
  {
    varADIOS2.SetSelection({ memoryRequirements.Start, memoryRequirements.Count });
  }
  else
  {
    // ADIOS2 calls `SetSelection`
    varADIOS2.SetBlockSelection(blockId);
  }
}

template <typename VariableType>
vtkm::cont::UnknownArrayHandle ReadVariableInternal(adios2::Engine& reader,
                                                    adios2::Variable<VariableType>& varADIOS2,
                                                    size_t blockId,
                                                    EngineType engineType,
                                                    size_t step,
                                                    IsVector isit = IsVector::Auto,
                                                    bool createSharedPoints = false)
{
  auto blocksInfo = reader.BlocksInfo(varADIOS2, step);
  auto memoryRequirements =
    GetVariableMemoryRequirements(blocksInfo, varADIOS2, blockId, createSharedPoints);
  const auto& shape = memoryRequirements.Count;
  auto& bufSize = memoryRequirements.Size;

  vtkm::cont::UnknownArrayHandle retVal;
  VariableType* buffer = nullptr;

  PrepareVariableSelection(varADIOS2, memoryRequirements, blockId);

  if (engineType == EngineType::Inline)
  {
    // For the inline engine we can grab the pointer to the data
    // instead of data being copied into a buffer.
    // And this can be handled the same way whether it's a
    // vector or not
    reader.Get(varADIOS2, blocksInfo[blockId]);
    reader.PerformGets();
  }

  // This logic is used to determine if a variable is a
  // vector (in which case we need to read it as 2D) or
  // not (in which case we need to read it as 1D even when
  // it is a multi-dimensional variable because VTK-m expects
  // it as such)
  bool isVector;
  if (isit == IsVector::Auto)
  {
    // If we are in auto mode, assume all 2D variables are
    // vectors. This is the default.
    isVector = shape.size() == 2;
  }
  else
  {
    // Otherwise, use what the data model says.
    isVector = isit == IsVector::Yes;
  }

  if (!isVector)
  {
    if (engineType == EngineType::Inline)
    {
      const VariableType* vecData = blocksInfo[blockId].Data();
      vtkm::cont::ArrayHandle<VariableType> arrayHandle =
        vtkm::cont::make_ArrayHandle(vecData, bufSize, vtkm::CopyFlag::Off);
      retVal = arrayHandle;
    }
    else
    {
      vtkm::cont::ArrayHandleBasic<VariableType> arrayHandle;
      arrayHandle.Allocate(bufSize);
      buffer = arrayHandle.GetWritePointer();
      retVal = arrayHandle;
      reader.Get(varADIOS2, buffer);
    }
  }
  else
  {
    // Vector: the last dimension is assumed to be the vector
    // components. Previous dimensions are collapsed together.
    size_t nDims = shape.size();
    if (nDims < 2)
    {
      throw std::runtime_error("1D array cannot be a vector");
    }

    vtkm::Id bufSize2 = 1;
    for (size_t i = 0; i < nDims - 1; i++)
    {
      bufSize2 *= shape[i];
    }
    if (engineType == EngineType::Inline)
    {
      const VariableType* vecData = blocksInfo[blockId].Data();
      switch (shape[nDims - 1])
      {
        case 1:
          retVal = AllocateArrayHandle<VariableType, 1>(vecData, bufSize);
          break;
        case 2:
          retVal = AllocateArrayHandle<VariableType, 2>(vecData, bufSize2);
          break;
        case 3:
          retVal = AllocateArrayHandle<VariableType, 3>(vecData, bufSize2);
          break;
        default:
          break;
      }
    }
    else
    {
      switch (shape[nDims - 1])
      {
        case 1:
          retVal = AllocateArrayHandle<VariableType, VariableType>(bufSize, buffer);
          break;
        case 2:
          retVal = AllocateArrayHandle<VariableType, vtkm::Vec<VariableType, 2>>(bufSize2, buffer);
          break;
        case 3:
          retVal = AllocateArrayHandle<VariableType, vtkm::Vec<VariableType, 3>>(bufSize2, buffer);
          break;
        default:
          break;
      }
      reader.Get(varADIOS2, buffer);
    }
  }

  return retVal;
}

// Inline engine is not supported for multiblock read into a contiguous array
template <typename VariableType>
vtkm::cont::UnknownArrayHandle ReadMultiBlockVariableInternal(
  adios2::Engine& reader,
  adios2::Variable<VariableType>& varADIOS2,
  std::vector<size_t> blocks,
  size_t step)
{
  auto blocksInfo = reader.BlocksInfo(varADIOS2, step);
  vtkm::Id bufSize = 0;
  for (const auto& blockId : blocks)
  {
    const auto memoryRequirements = GetVariableMemoryRequirements(blocksInfo, varADIOS2, blockId);
    bufSize += memoryRequirements.Size;
  }

  vtkm::cont::UnknownArrayHandle retVal;
  vtkm::cont::ArrayHandleBasic<VariableType> arrayHandle;
  arrayHandle.Allocate(bufSize);
  VariableType* buffer = arrayHandle.GetWritePointer();
  retVal = arrayHandle;
  for (size_t i = 0; i < blocks.size(); ++i)
  {
    auto blockId = blocks[i];
    if (i > 0)
    {
      const auto& shape = blocksInfo[blockId].Count;
      vtkm::Id size = 1;
      for (auto n : shape)
      {
        size *= n;
      }
      buffer += size;
    }
    varADIOS2.SetBlockSelection(blockId);
    reader.Get(varADIOS2, buffer);
  }

  return retVal;
}

template <typename VariableType>
size_t GetNumberOfBlocksInternal(adios2::IO& adiosIO,
                                 adios2::Engine& reader,
                                 const std::string& varName)
{
  auto varADIOS2 = adiosIO.InquireVariable<VariableType>(varName);
  auto blocksInfo = reader.BlocksInfo(varADIOS2, reader.CurrentStep());
  return blocksInfo.size();
}

template <typename VariableType>
std::vector<size_t> GetVariableShapeInternal(adios2::IO& adiosIO,
                                             adios2::Engine& fidesNotUsed(reader),
                                             const std::string& varName)
{
  auto varADIOS2 = adiosIO.InquireVariable<VariableType>(varName);
  return varADIOS2.Shape();
}

template <typename VariableType>
std::vector<vtkm::cont::UnknownArrayHandle> ReadVariableBlocksInternal(
  adios2::IO& adiosIO,
  adios2::Engine& reader,
  const std::string& varName,
  const fides::metadata::MetaData& selections,
  EngineType engineType,
  IsVector isit = IsVector::Auto,
  bool isMultiBlock = false,
  bool createSharedPoints = false)
{
  std::vector<vtkm::cont::UnknownArrayHandle> arrays;
  if (selections.Has(fides::keys::BLOCK_SELECTION()) &&
      selections.Get<fides::metadata::Vector<size_t>>(fides::keys::BLOCK_SELECTION()).Data.empty())
  {
    return arrays;
  }
  auto varADIOS2 = adiosIO.InquireVariable<VariableType>(varName);

  if (!varADIOS2)
  {
    throw std::runtime_error("adiosIO.InquireVariable() failed on variable " + varName);
  }

  auto step = reader.CurrentStep();
  if (selections.Has(fides::keys::STEP_SELECTION()) && varADIOS2.Steps() > 1)
  {
    step = selections.Get<fides::metadata::Index>(fides::keys::STEP_SELECTION()).Data;
    varADIOS2.SetStepSelection({ step, 1 });
  }

  auto blocksInfo = reader.BlocksInfo(varADIOS2, step);
  if (blocksInfo.empty())
  {
    return arrays;
    //throw std::runtime_error("reader.BlocksInfo() did not return any block for variable " +
    //                         varName + " for step " + std::to_string(step));
  }
  std::vector<size_t> blocksToReallyRead;
  if (!selections.Has(fides::keys::BLOCK_SELECTION()))
  {
    size_t nBlocks = blocksInfo.size();
    blocksToReallyRead.resize(nBlocks);
    std::iota(blocksToReallyRead.begin(), blocksToReallyRead.end(), 0);
  }
  else
  {
    const std::vector<size_t>& blocksToRead =
      selections.Get<fides::metadata::Vector<size_t>>(fides::keys::BLOCK_SELECTION()).Data;
    blocksToReallyRead = blocksToRead;
  }

  if (isMultiBlock)
  {
    if (engineType == EngineType::Inline)
    {
      throw std::runtime_error(
        "Inline engine is not supported when reading multiple blocks into a single "
        "contiguous array");
    }
    arrays.reserve(1);
    arrays.push_back(
      ReadMultiBlockVariableInternal<VariableType>(reader, varADIOS2, blocksToReallyRead, step));
  }
  else
  {
    arrays.reserve(blocksToReallyRead.size());
    for (auto blockId : blocksToReallyRead)
    {
      arrays.push_back(ReadVariableInternal<VariableType>(
        reader, varADIOS2, blockId, engineType, step, isit, createSharedPoints));
    }
  }

  return arrays;
}

template <typename VariableType>
std::vector<vtkm::cont::UnknownArrayHandle> GetDimensionsInternal(
  adios2::IO& adiosIO,
  adios2::Engine& reader,
  const std::string& varName,
  const fides::metadata::MetaData& selections,
  bool createSharedPoints = false)
{
  auto varADIOS2 = adiosIO.InquireVariable<VariableType>(varName);
  size_t step = reader.CurrentStep();
  if (selections.Has(fides::keys::STEP_SELECTION()) && varADIOS2.Steps() > 1)
  {
    step = selections.Get<fides::metadata::Index>(fides::keys::STEP_SELECTION()).Data;
  }

  auto blocksInfo = reader.BlocksInfo(varADIOS2, step);
  if (blocksInfo.empty())
  {
    throw std::runtime_error("blocksInfo is 0 for variable: " + varName);
  }

  std::vector<size_t> blocksToReallyRead;
  if (!selections.Has(fides::keys::BLOCK_SELECTION()) ||
      selections.Get<fides::metadata::Vector<size_t>>(fides::keys::BLOCK_SELECTION()).Data.empty())
  {
    size_t nBlocks = blocksInfo.size();
    blocksToReallyRead.resize(nBlocks);
    std::iota(blocksToReallyRead.begin(), blocksToReallyRead.end(), 0);
  }
  else
  {
    const std::vector<size_t>& blocksToRead =
      selections.Get<fides::metadata::Vector<size_t>>(fides::keys::BLOCK_SELECTION()).Data;
    blocksToReallyRead = blocksToRead;
  }

  std::vector<vtkm::cont::UnknownArrayHandle> arrays;
  arrays.reserve(blocksToReallyRead.size());

  for (auto blockId : blocksToReallyRead)
  {
    const auto memoryRequirements =
      GetVariableMemoryRequirements(blocksInfo, varADIOS2, blockId, createSharedPoints);
    std::vector<size_t> shape = memoryRequirements.Count;
    std::reverse(shape.begin(), shape.end());
    std::vector<size_t> start = memoryRequirements.Start;
    std::reverse(start.begin(), start.end());
    shape.insert(shape.end(), start.begin(), start.end());
    arrays.push_back(vtkm::cont::make_ArrayHandle(shape, vtkm::CopyFlag::On));
  }

  return arrays;
}

// Since this is grabbing a scalar variable, ADIOS should always be
// able to return the actual value immediately
template <typename VariableType>
std::vector<vtkm::cont::UnknownArrayHandle> GetScalarVariableInternal(
  adios2::IO& adiosIO,
  adios2::Engine& reader,
  const std::string& varName,
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  auto varADIOS2 = adiosIO.InquireVariable<VariableType>(varName);

  std::vector<vtkm::cont::UnknownArrayHandle> retVal;
  vtkm::cont::UnknownArrayHandle valueAH;
  vtkm::cont::ArrayHandleBasic<VariableType> arrayHandle;
  arrayHandle.Allocate(1);
  VariableType* buffer = arrayHandle.GetWritePointer();
  valueAH = arrayHandle;
  // because we're getting a single value, we can use sync mode here
  // I think for most engines, it doesn't actually matter to specify this for
  // a single value (it will still be performed in sync),
  // but for Inline engine, you'll get an error if you don't specify sync mode
  reader.Get(varADIOS2, buffer, adios2::Mode::Sync);
  retVal.push_back(valueAH);

  return retVal;
}

template <typename VariableType>
std::vector<vtkm::cont::UnknownArrayHandle> GetTimeArrayInternal(
  adios2::IO& adiosIO,
  adios2::Engine& reader,
  const std::string& varName,
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  auto varADIOS2 = adiosIO.InquireVariable<VariableType>(varName);
  auto numSteps = varADIOS2.Steps();
  varADIOS2.SetStepSelection({ varADIOS2.StepsStart(), numSteps });
  std::vector<vtkm::cont::UnknownArrayHandle> retVal;
  vtkm::cont::UnknownArrayHandle valueAH;
  vtkm::cont::ArrayHandleBasic<VariableType> arrayHandle;
  arrayHandle.Allocate(numSteps);
  VariableType* buffer = arrayHandle.GetWritePointer();
  valueAH = arrayHandle;
  reader.Get(varADIOS2, buffer, adios2::Mode::Sync);
  retVal.push_back(valueAH);

  return retVal;
}

#define fidesTemplateMacro(call)                 \
  switch (type[0])                               \
  {                                              \
    case 'c':                                    \
    {                                            \
      using fides_TT = char;                     \
      return call;                               \
      break;                                     \
    }                                            \
    case 'f':                                    \
    {                                            \
      using fides_TT = float;                    \
      return call;                               \
      break;                                     \
    }                                            \
    case 'd':                                    \
    {                                            \
      using fides_TT = double;                   \
      return call;                               \
      break;                                     \
    }                                            \
    case 'i':                                    \
      if (type == "int")                         \
      {                                          \
        using fides_TT = int;                    \
        return call;                             \
      }                                          \
      else if (type == "int8_t")                 \
      {                                          \
        using fides_TT = int8_t;                 \
        return call;                             \
      }                                          \
      else if (type == "int16_t")                \
      {                                          \
        using fides_TT = int16_t;                \
        return call;                             \
      }                                          \
      else if (type == "int32_t")                \
      {                                          \
        using fides_TT = int32_t;                \
        return call;                             \
      }                                          \
      else if (type == "int64_t")                \
      {                                          \
        using fides_TT = int64_t;                \
        return call;                             \
      }                                          \
      break;                                     \
    case 'l':                                    \
      if (type == "long long int")               \
      {                                          \
        using fides_TT = long long int;          \
        return call;                             \
      }                                          \
      else if (type == "long int")               \
      {                                          \
        using fides_TT = long int;               \
        return call;                             \
      }                                          \
      break;                                     \
    case 's':                                    \
      if (type == "short")                       \
      {                                          \
        using fides_TT = short;                  \
        return call;                             \
      }                                          \
      else if (type == "signed char")            \
      {                                          \
        using fides_TT = signed char;            \
        return call;                             \
      }                                          \
      break;                                     \
    case 'u':                                    \
      if (type == "unsigned char")               \
      {                                          \
        using fides_TT = unsigned char;          \
        return call;                             \
      }                                          \
      else if (type == "unsigned int")           \
      {                                          \
        using fides_TT = unsigned int;           \
        return call;                             \
      }                                          \
      else if (type == "unsigned long int")      \
      {                                          \
        using fides_TT = unsigned long int;      \
        return call;                             \
      }                                          \
      else if (type == "unsigned long long int") \
      {                                          \
        using fides_TT = unsigned long long int; \
        return call;                             \
      }                                          \
      else if (type == "uint8_t")                \
      {                                          \
        using fides_TT = uint8_t;                \
        return call;                             \
      }                                          \
      else if (type == "uint16_t")               \
      {                                          \
        using fides_TT = uint16_t;               \
        return call;                             \
      }                                          \
      else if (type == "uint32_t")               \
      {                                          \
        using fides_TT = uint32_t;               \
        return call;                             \
      }                                          \
      else if (type == "uint64_t")               \
      {                                          \
        using fides_TT = uint64_t;               \
        return call;                             \
      }                                          \
      break;                                     \
  }

std::vector<vtkm::cont::UnknownArrayHandle> DataSource::GetVariableDimensions(
  const std::string& varName,
  const fides::metadata::MetaData& selections)
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variable without setting the adios engine.");
  }
  auto itr = this->FindVariable(varName, selections);
  if (itr == this->AvailVars.end())
  {
    // previously we were throwing an error if the variable could not be found,
    // but it's possible that a variable may just not be available on a certain timestep.
    return std::vector<vtkm::cont::UnknownArrayHandle>();
  }

  const std::string& type = itr->second["Type"];
  if (type.empty())
  {
    throw std::runtime_error("Variable type unavailable.");
  }

  if (this->AdiosEngineType == EngineType::Inline)
  {
    // in the inline case, Fides can't read from other blocks,
    // so we'll just set to false so we don't end up with junk
    // data.
    this->CreateSharedPoints = false;
  }

  fidesTemplateMacro(GetDimensionsInternal<fides_TT>(
    this->AdiosIO, this->Reader, itr->first, selections, this->CreateSharedPoints));

  throw std::runtime_error("Unsupported variable type " + type);
}

std::vector<vtkm::cont::UnknownArrayHandle> DataSource::GetScalarVariable(
  const std::string& varName,
  const fides::metadata::MetaData& selections)
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variable without setting the adios engine.");
  }
  auto itr = this->FindVariable(varName, selections);
  if (itr == this->AvailVars.end())
  {
    // previously we were throwing an error if the variable could not be found,
    // but it's possible that a variable may just not be available on a certain timestep.
    return std::vector<vtkm::cont::UnknownArrayHandle>();
  }

  const std::string& type = itr->second["Type"];
  if (type.empty())
  {
    throw std::runtime_error("Variable type unavailable.");
  }

  fidesTemplateMacro(
    GetScalarVariableInternal<fides_TT>(this->AdiosIO, this->Reader, itr->first, selections));

  throw std::runtime_error("Unsupported variable type " + type);
}

std::vector<vtkm::cont::UnknownArrayHandle> DataSource::GetTimeArray(
  const std::string& varName,
  const fides::metadata::MetaData& selections)
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variable without setting the adios engine.");
  }

  if (this->AdiosEngineType != EngineType::BPFile)
  {
    throw std::runtime_error("A full time array can only be read when using BP files");
  }

  auto itr = this->FindVariable(varName, selections);
  if (itr == this->AvailVars.end())
  {
    // previously we were throwing an error if the variable could not be found,
    // but it's possible that a variable may just not be available on a certain timestep.
    return std::vector<vtkm::cont::UnknownArrayHandle>();
  }

  const std::string& type = itr->second["Type"];
  if (type.empty())
  {
    throw std::runtime_error("Variable type unavailable.");
  }

  fidesTemplateMacro(
    GetTimeArrayInternal<fides_TT>(this->AdiosIO, this->Reader, itr->first, selections));

  throw std::runtime_error("Unsupported variable type " + type);
}

std::vector<vtkm::cont::UnknownArrayHandle> DataSource::ReadVariable(
  const std::string& varName,
  const fides::metadata::MetaData& selections,
  IsVector isit)
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variable without setting the adios engine.");
  }
  auto itr = this->FindVariable(varName, selections);
  if (itr == this->AvailVars.end())
  {
    // previously we were throwing an error if the variable could not be found,
    // but it's possible that a variable may just not be available on a certain timestep.
    return std::vector<vtkm::cont::UnknownArrayHandle>();
  }
  const std::string& type = itr->second["Type"];
  if (type.empty())
  {
    throw std::runtime_error("Variable type unavailable.");
  }

  if (this->AdiosEngineType == EngineType::Inline)
  {
    // in the inline case, Fides can't read from other blocks,
    // so we'll just set to false so we don't end up with junk
    // data.
    this->CreateSharedPoints = false;
  }

  fidesTemplateMacro(ReadVariableBlocksInternal<fides_TT>(this->AdiosIO,
                                                          this->Reader,
                                                          itr->first,
                                                          selections,
                                                          this->AdiosEngineType,
                                                          isit,
                                                          false,
                                                          this->CreateSharedPoints));

  throw std::runtime_error("Unsupported variable type " + type);
}

std::vector<vtkm::cont::UnknownArrayHandle> DataSource::ReadMultiBlockVariable(
  const std::string& varName,
  const fides::metadata::MetaData& selections)
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variable without setting the adios engine.");
  }
  auto itr = this->FindVariable(varName, selections);
  if (itr == this->AvailVars.end())
  {
    // previously we were throwing an error if the variable could not be found,
    // but it's possible that a variable may just not be available on a certain timestep.
    return std::vector<vtkm::cont::UnknownArrayHandle>();
  }
  const std::string& type = itr->second["Type"];
  if (type.empty())
  {
    throw std::runtime_error("Variable type unavailable.");
  }

  fidesTemplateMacro(ReadVariableBlocksInternal<fides_TT>(this->AdiosIO,
                                                          this->Reader,
                                                          itr->first,
                                                          selections,
                                                          this->AdiosEngineType,
                                                          IsVector::No,
                                                          true));

  throw std::runtime_error("Unsupported variable type " + type);
}

size_t DataSource::GetNumberOfBlocks(const std::string& varName)
{
  return this->GetNumberOfBlocks(varName, "");
}

size_t DataSource::GetNumberOfBlocks(const std::string& varName, const std::string& group)
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variable without setting the adios engine.");
  }
  fides::metadata::MetaData selections;
  selections.Set<fides::metadata::String>(fides::keys::GROUP_SELECTION(), group);
  auto itr = this->FindVariable(varName, selections);
  if (itr == this->AvailVars.end())
  {
    return 0;
  }
  const std::string& type = itr->second["Type"];

  fidesTemplateMacro(GetNumberOfBlocksInternal<fides_TT>(this->AdiosIO, this->Reader, itr->first));

  throw std::runtime_error("Unsupported variable type " + type);
}

std::set<std::string> DataSource::GetGroupNames(const std::string& name) const
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot retrieve groups without setting the adios engine.");
  }
  auto itr = this->AvailGroups.find(name);
  if (itr != this->AvailGroups.end())
  {
    return itr->second;
  }
  else
  {
    return {};
  }
}

size_t DataSource::GetNumberOfSteps()
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variable without setting the adios engine.");
  }
  size_t nSteps;
  try
  {
    nSteps = this->Reader.Steps();
  }
  // This is here to handle cases that do not support Steps()
  // such as the SST engine.
  catch (std::invalid_argument&)
  {
    nSteps = 0;
  }
  return nSteps;
}

void DataSource::DoAllReads()
{
  // It's possible for a data source to exist, but not have the adios reader
  // be opened, so don't throw an error here.
  if (this->Reader)
  {
    this->Reader.PerformGets();
  }
}

StepStatus DataSource::BeginStep()
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variables without setting the adios engine.");
  }

  if (this->MostRecentStepStatus != StepStatus::EndOfStream)
  {
    auto retVal = this->Reader.BeginStep();
    switch (retVal)
    {
      case adios2::StepStatus::OK:
        this->Refresh();
        this->MostRecentStepStatus = StepStatus::OK;
        break;
      case adios2::StepStatus::NotReady:
        this->MostRecentStepStatus = StepStatus::NotReady;
        break;
      case adios2::StepStatus::EndOfStream:
        this->MostRecentStepStatus = StepStatus::EndOfStream;
        break;
      case adios2::StepStatus::OtherError:
        this->MostRecentStepStatus = StepStatus::NotReady;
        break;
      default:
        throw std::runtime_error("DataSource::BeginStep received unknown StepStatus from ADIOS");
    }
  }
  return this->MostRecentStepStatus;
}

size_t DataSource::CurrentStep()
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot get step without setting the adios engine.");
  }

  return this->Reader.CurrentStep();
}

void DataSource::EndStep()
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variables without setting the adios engine.");
  }

  if (this->MostRecentStepStatus == StepStatus::OK)
  {
    this->Reader.EndStep();
  }
}

std::vector<size_t> DataSource::GetVariableShape(std::string& varName)
{
  return this->GetVariableShape(varName, "");
}

std::vector<size_t> DataSource::GetVariableShape(std::string& varName, const std::string& group)
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot get variable size without setting the adios engine.");
  }
  fides::metadata::MetaData selections;
  selections.Set<fides::metadata::String>(fides::keys::GROUP_SELECTION(), group);
  auto itr = this->FindVariable(varName, selections);
  if (itr == this->AvailVars.end())
  {
    throw std::runtime_error("Variable " + varName + " was not found.");
  }
  const std::string& type = itr->second["Type"];

  fidesTemplateMacro(GetVariableShapeInternal<fides_TT>(this->AdiosIO, this->Reader, itr->first));

  throw std::runtime_error("Unsupported variable type " + type);
}

std::string DataSource::GetAttributeType(const std::string& attrName)
{
  return this->GetAttributeType(attrName, "");
}

std::string DataSource::GetAttributeType(const std::string& attrName, const std::string& group)
{
  fides::metadata::MetaData selections;
  selections.Set<fides::metadata::String>(fides::keys::GROUP_SELECTION(), group);
  auto itr = this->FindAttribute(attrName, selections);
  if (itr == this->AvailAtts.end())
  {
    // Attributes can be optional so just return empty string if it isn't found
    return "";
  }
  return itr->second["Type"];
}

}
}
