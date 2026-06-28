//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//============================================================================

#include <fides/ADIOSDataSource.h>

#if FIDES_USE_MPI
#include <fides/fides_mpi.h>
#endif

#include <algorithm>
#include <iostream>
#include <iterator>
#include <numeric>


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
inline adios2::Mode ToAdiosMode(fides::io::ReadMode mode)
{
  return mode == fides::io::ReadMode::Sync ? adios2::Mode::Sync : adios2::Mode::Deferred;
}

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

ADIOSDataSource::ADIOSDataSource()
{
#if FIDES_USE_MPI
  this->Comm = MPI_COMM_NULL;
#endif
}

#if FIDES_USE_MPI
ADIOSDataSource::ADIOSDataSource(MPI_Comm comm)
{
  this->Comm = comm;
}
#endif

ADIOSDataSource::~ADIOSDataSource() = default;

void ADIOSDataSource::SetEngineType(const std::string engine_type)
{
  if (engine_type == "BPFile")
  {
    this->AdiosEngineType = EngineType::BPFile;
  }
  else if (engine_type == "SST")
  {
    this->AdiosEngineType = EngineType::SST;
  }
  else if (engine_type == "Inline")
  {
    this->AdiosEngineType = EngineType::Inline;
  }
  else
  {
    throw std::runtime_error("parameter engine_type must be BPFile, SST or Inline.");
  }
}

void ADIOSDataSource::SetDataSourceParameters(const DataSourceParams& params)
{
  this->SourceParams = params;
}

void ADIOSDataSource::SetDataSourceIO(void* io)
{
  if (!io)
  {
    return;
  }
  this->AdiosIO = *(reinterpret_cast<adios2::IO*>(io));
  this->SetupEngine();
  this->OpenSource(this->ReaderID);
}

void ADIOSDataSource::SetDataSourceIO(const std::string& ioAddress)
{
  uintptr_t hexAddress = static_cast<uintptr_t>(std::stoull(ioAddress, nullptr, 16));
  // NOLINTNEXTLINE(performance-no-int-to-ptr)
  this->AdiosIO = *(reinterpret_cast<adios2::IO*>(hexAddress));
  this->SetupEngine();
  this->OpenSource(this->ReaderID);
}

void ADIOSDataSource::SetupEngine()
{
  auto it = this->SourceParams.find("engine_type");
  if (it != this->SourceParams.end())
  {
    std::string engine = it->second;
    this->SetEngineType(engine);
  }

  if (this->AdiosEngineType == EngineType::BPFile)
  {
    this->AdiosIO.SetEngine("BPFile");
  }
  else if (this->AdiosEngineType == EngineType::SST)
  {
    this->AdiosIO.SetEngine("SST");
  }
  else if (this->AdiosEngineType == EngineType::Inline)
  {
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

std::map<std::string, adios2::Params>::iterator ADIOSDataSource::FindVariable(
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

std::map<std::string, adios2::Params>::iterator ADIOSDataSource::FindAttribute(
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

void ADIOSDataSource::OpenSource(const std::unordered_map<std::string, std::string>& paths,
                                 const std::string& dataSourceName,
                                 bool useMPI /* = true */)
{
  if (this->Reader)
  {
    return;
  }

  auto itr = paths.find(dataSourceName);
  std::string path;
  if (itr != paths.end())
  {
    path = itr->second;
  }
  else
  {
    if (!this->RelativePath.empty())
    {
      path = this->RelativePath;
    }
    else
    {
      throw std::runtime_error("Could not find data_source with name " + dataSourceName +
                               " among the input paths.");
    }
  }

  path += this->FileName;
  this->OpenSource(path, useMPI);
}

void ADIOSDataSource::OpenSource(const std::string& fname, bool useMPI /* = true */)
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
#if FIDES_USE_MPI
      if (useMPI)
      {
        this->Adios.reset(new adios2::ADIOS(this->Comm));
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

  auto engineValue = this->AdiosIO.Open(fname, mode);
  this->Reader = std::make_shared<adios2::Engine>(std::move(engineValue));
  this->Refresh();
}

void ADIOSDataSource::Close()
{
  if (this->Reader)
  {
    this->Reader->Close();
  }
}

void ADIOSDataSource::Refresh()
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

struct FidesArrayMemoryRequirements
{
  /// Total number of elements.
  size_t Size;
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
          const size_t indexDelta = static_cast<size_t>(extendDimensions);
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
  if (memoryRequirements.Size == 0)
  {
    throw std::runtime_error("Zero-sized read detected for variable " + varADIOS2.Name());
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
fides::RawArray ReadVariableInternal(std::shared_ptr<adios2::Engine> reader,
                                     adios2::Variable<VariableType>& varADIOS2,
                                     size_t blockId,
                                     EngineType engineType,
                                     size_t step,
                                     adios2::Mode mode,
                                     IsVector isit = IsVector::Auto,
                                     bool createSharedPoints = false)
{
  auto blocksInfo = reader->BlocksInfo(varADIOS2, step);
  auto memoryRequirements =
    GetVariableMemoryRequirements(blocksInfo, varADIOS2, blockId, createSharedPoints);
  const auto& shape = memoryRequirements.Count;
  auto& bufSize = memoryRequirements.Size;

  PrepareVariableSelection(varADIOS2, memoryRequirements, blockId);

  // This logic is used to determine if a variable is a
  // vector (in which case we need to read it as 2D) or
  // not (in which case we need to read it as 1D even when
  // it is a multi-dimensional variable because Viskores expects
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

  int numComponents = 1;
  size_t numValues = bufSize;
  if (isVector)
  {
    const size_t nDims = shape.size();
    numComponents = static_cast<int>(shape[nDims - 1]);
    numValues = bufSize / static_cast<size_t>(numComponents);
  }

  if (engineType == EngineType::Inline)
  {
    reader->Get(varADIOS2, blocksInfo[blockId]);
    reader->PerformGets();

    // Get pointer to the data buffer we want to share access to
    const VariableType* vecData = blocksInfo[blockId].Data();

    // For inline engine, wrap the ADIOS2-owned pointer with a no-op deleter,
    // and also copy a shared handle to the engine so we keep it alive at
    // least until the RawArray we're handing back goes out of scope.
    std::shared_ptr<void> buf(
      const_cast<VariableType*>(vecData), [engineHandle = reader, blocksInfo](void*) {
        // Even though we don't actually need to do anything in the custom
        // deleter, we need it so that, via its captured context, we keep the
        // buffer and the engine around until the last shared_ptr goes away.
      });
    return fides::RawArray(
      std::move(buf), numValues, numComponents, fides::GetDataType<VariableType>());
  }
  else
  {
    // Allocate our own buffer; mode is set by the caller (Deferred by
    // default, Sync only when the caller needs the data immediately).
    auto raw = fides::AllocateRawArray<VariableType>(numValues, numComponents);
    VariableType* buffer = raw.template GetWritePointer<VariableType>();
    reader->Get(varADIOS2, buffer, mode);
    return raw;
  }
}

// Inline engine is not supported for multiblock read into a contiguous array
template <typename VariableType>
fides::RawArray ReadMultiBlockVariableInternal(std::shared_ptr<adios2::Engine> reader,
                                               adios2::Variable<VariableType>& varADIOS2,
                                               std::vector<size_t> blocks,
                                               size_t step,
                                               adios2::Mode mode)
{
  auto blocksInfo = reader->BlocksInfo(varADIOS2, step);
  size_t bufSize = 0;
  for (const auto& blockId : blocks)
  {
    const auto memoryRequirements = GetVariableMemoryRequirements(blocksInfo, varADIOS2, blockId);
    bufSize += memoryRequirements.Size;
  }

  auto raw = fides::AllocateRawArray<VariableType>(bufSize);
  VariableType* buffer = raw.template GetWritePointer<VariableType>();

  for (size_t i = 0; i < blocks.size(); ++i)
  {
    auto blockId = blocks[i];
    if (i > 0)
    {
      const auto& blockShape = blocksInfo[blockId].Count;
      size_t size = 1;
      for (auto n : blockShape)
      {
        size *= n;
      }
      buffer += size;
    }
    varADIOS2.SetBlockSelection(blockId);
    reader->Get(varADIOS2, buffer, mode);
  }

  return raw;
}

template <typename VariableType>
size_t GetNumberOfBlocksInternal(adios2::IO& adiosIO,
                                 std::shared_ptr<adios2::Engine> reader,
                                 const std::string& varName)
{
  auto varADIOS2 = adiosIO.InquireVariable<VariableType>(varName);
  auto blocksInfo = reader->BlocksInfo(varADIOS2, reader->CurrentStep());
  return blocksInfo.size();
}

template <typename VariableType>
std::vector<size_t> GetVariableShapeInternal(adios2::IO& adiosIO,
                                             std::shared_ptr<adios2::Engine> fidesNotUsed(reader),
                                             const std::string& varName)
{
  auto varADIOS2 = adiosIO.InquireVariable<VariableType>(varName);
  return varADIOS2.Shape();
}

template <typename VariableType>
std::vector<fides::RawArray> ReadVariableBlocksInternal(adios2::IO& adiosIO,
                                                        std::shared_ptr<adios2::Engine> reader,
                                                        const std::string& varName,
                                                        const fides::metadata::MetaData& selections,
                                                        EngineType engineType,
                                                        adios2::Mode mode,
                                                        IsVector isit = IsVector::Auto,
                                                        bool isMultiBlock = false,
                                                        bool createSharedPoints = false)
{
  std::vector<fides::RawArray> arrays;
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

  auto step = reader->CurrentStep();
  if (selections.Has(fides::keys::STEP_SELECTION()) && varADIOS2.Steps() > 1)
  {
    step = selections.Get<fides::metadata::Index>(fides::keys::STEP_SELECTION()).Data;
    varADIOS2.SetStepSelection({ step, 1 });
  }

  auto blocksInfo = reader->BlocksInfo(varADIOS2, step);
  if (blocksInfo.empty())
  {
    return arrays;
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
    arrays.push_back(ReadMultiBlockVariableInternal<VariableType>(
      reader, varADIOS2, blocksToReallyRead, step, mode));
  }
  else
  {
    arrays.reserve(blocksToReallyRead.size());
    for (auto blockId : blocksToReallyRead)
    {
      arrays.push_back(ReadVariableInternal<VariableType>(
        reader, varADIOS2, blockId, engineType, step, mode, isit, createSharedPoints));
    }
  }

  return arrays;
}

template <typename VariableType>
std::vector<fides::RawArray> GetDimensionsInternal(adios2::IO& adiosIO,
                                                   std::shared_ptr<adios2::Engine> reader,
                                                   const std::string& varName,
                                                   const fides::metadata::MetaData& selections,
                                                   bool createSharedPoints = false)
{
  auto varADIOS2 = adiosIO.InquireVariable<VariableType>(varName);
  size_t step = reader->CurrentStep();
  if (selections.Has(fides::keys::STEP_SELECTION()) && varADIOS2.Steps() > 1)
  {
    step = selections.Get<fides::metadata::Index>(fides::keys::STEP_SELECTION()).Data;
  }

  auto blocksInfo = reader->BlocksInfo(varADIOS2, step);
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

  std::vector<fides::RawArray> arrays;
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

    // Store the shape/start data as a RawArray of size_t
    auto raw = fides::AllocateRawArray<size_t>(shape.size());
    std::memcpy(raw.GetWritePointer<size_t>(), shape.data(), shape.size() * sizeof(size_t));
    arrays.push_back(std::move(raw));
  }

  return arrays;
}

// Since this is grabbing a scalar variable, ADIOS should always be
// able to return the actual value immediately
template <typename VariableType>
std::vector<fides::RawArray> GetScalarVariableInternal(
  adios2::IO& adiosIO,
  std::shared_ptr<adios2::Engine> reader,
  const std::string& varName,
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  auto varADIOS2 = adiosIO.InquireVariable<VariableType>(varName);

  auto raw = fides::AllocateRawArray<VariableType>(1);
  VariableType* buffer = raw.template GetWritePointer<VariableType>();
  // because we're getting a single value, we can use sync mode here
  // I think for most engines, it doesn't actually matter to specify this for
  // a single value (it will still be performed in sync),
  // but for Inline engine, you'll get an error if you don't specify sync mode
  reader->Get(varADIOS2, buffer, adios2::Mode::Sync);

  std::vector<fides::RawArray> retVal;
  retVal.push_back(std::move(raw));
  return retVal;
}

template <typename VariableType>
std::vector<fides::RawArray> GetTimeArrayInternal(
  adios2::IO& adiosIO,
  std::shared_ptr<adios2::Engine> reader,
  const std::string& varName,
  const fides::metadata::MetaData& fidesNotUsed(selections))
{
  auto varADIOS2 = adiosIO.InquireVariable<VariableType>(varName);
  auto numSteps = varADIOS2.Steps();
  varADIOS2.SetStepSelection({ varADIOS2.StepsStart(), numSteps });

  auto raw = fides::AllocateRawArray<VariableType>(numSteps);
  VariableType* buffer = raw.template GetWritePointer<VariableType>();
  reader->Get(varADIOS2, buffer, adios2::Mode::Sync);

  std::vector<fides::RawArray> retVal;
  retVal.push_back(std::move(raw));
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

std::vector<fides::RawArray> ADIOSDataSource::GetVariableDimensions(
  const std::string& varName,
  const fides::metadata::MetaData& selections,
  fides::FieldAssociation association)
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
    return std::vector<fides::RawArray>();
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

  const bool createSharedPoints =
    this->CreateSharedPoints && association != fides::FieldAssociation::Cells;

  fidesTemplateMacro(GetDimensionsInternal<fides_TT>(
    this->AdiosIO, this->Reader, itr->first, selections, createSharedPoints));

  throw std::runtime_error("Unsupported variable type " + type);
}

std::vector<fides::RawArray> ADIOSDataSource::GetScalarVariable(
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
    return std::vector<fides::RawArray>();
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

std::vector<fides::RawArray> ADIOSDataSource::GetTimeArray(
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
    return std::vector<fides::RawArray>();
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

std::vector<fides::RawArray> ADIOSDataSource::ReadVariable(
  const std::string& varName,
  const fides::metadata::MetaData& selections,
  IsVector isit,
  ReadMode mode)
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
    return std::vector<fides::RawArray>();
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

  const adios2::Mode adiosMode = ToAdiosMode(mode);

  fidesTemplateMacro(ReadVariableBlocksInternal<fides_TT>(this->AdiosIO,
                                                          this->Reader,
                                                          itr->first,
                                                          selections,
                                                          this->AdiosEngineType,
                                                          adiosMode,
                                                          isit,
                                                          false,
                                                          this->CreateSharedPoints));

  throw std::runtime_error("Unsupported variable type " + type);
}

std::vector<fides::RawArray> ADIOSDataSource::ReadMultiBlockVariable(
  const std::string& varName,
  const fides::metadata::MetaData& selections,
  ReadMode mode)
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
    return std::vector<fides::RawArray>();
  }
  const std::string& type = itr->second["Type"];
  if (type.empty())
  {
    throw std::runtime_error("Variable type unavailable.");
  }

  const adios2::Mode adiosMode = ToAdiosMode(mode);

  fidesTemplateMacro(ReadVariableBlocksInternal<fides_TT>(this->AdiosIO,
                                                          this->Reader,
                                                          itr->first,
                                                          selections,
                                                          this->AdiosEngineType,
                                                          adiosMode,
                                                          IsVector::No,
                                                          true));

  throw std::runtime_error("Unsupported variable type " + type);
}

size_t ADIOSDataSource::GetNumberOfBlocks(const std::string& varName)
{
  return this->GetNumberOfBlocks(varName, "");
}

size_t ADIOSDataSource::GetNumberOfBlocks(const std::string& varName, const std::string& group)
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

std::set<std::string> ADIOSDataSource::GetGroupNames(const std::string& name) const
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

size_t ADIOSDataSource::GetNumberOfSteps()
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variable without setting the adios engine.");
  }
  size_t nSteps;
  try
  {
    nSteps = this->Reader->Steps();
  }
  // This is here to handle cases that do not support Steps()
  // such as the SST engine.
  catch (std::invalid_argument&)
  {
    nSteps = 0;
  }
  return nSteps;
}

void ADIOSDataSource::DoAllReads()
{
  // It's possible for a data source to exist, but not have the adios reader
  // be opened, so don't throw an error here.
  if (this->Reader)
  {
    this->Reader->PerformGets();
  }
}

StepStatus ADIOSDataSource::BeginStep()
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variables without setting the adios engine.");
  }

  if (this->MostRecentStepStatus != StepStatus::EndOfStream)
  {
    if (this->DoNotCallNextBeginStep)
    {
      // this is necessary so we can appropriately handle BP5/SST when streaming and
      // the json info is in the adios file. if streaming, we have to call BeginStep
      // before we can read the json info. We'll handle this internally to try not to
      // complicate things for the user, so we need to make sure that when the user
      // calls PrepareNextStep that we don't call it on that source again or we'll
      // get an error about BeginStep being called twice.
      this->DoNotCallNextBeginStep = false;
      return this->MostRecentStepStatus;
    }

    auto retVal = this->Reader->BeginStep();
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
        this->MostRecentStepStatus = StepStatus::OtherError;
        break;
      default:
        throw std::runtime_error("DataSource::BeginStep received unknown StepStatus from ADIOS");
    }
  }
  return this->MostRecentStepStatus;
}

size_t ADIOSDataSource::CurrentStep()
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot get step without setting the adios engine.");
  }

  return this->Reader->CurrentStep();
}

void ADIOSDataSource::EndStep()
{
  if (!this->Reader)
  {
    throw std::runtime_error("Cannot read variables without setting the adios engine.");
  }

  if (this->MostRecentStepStatus == StepStatus::OK)
  {
    this->Reader->EndStep();
  }
}

std::vector<size_t> ADIOSDataSource::GetVariableShape(std::string& varName)
{
  return this->GetVariableShape(varName, "");
}

std::vector<size_t> ADIOSDataSource::GetVariableShape(std::string& varName,
                                                      const std::string& group)
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

std::string ADIOSDataSource::GetAttributeType(const std::string& attrName)
{
  return this->GetAttributeType(attrName, "");
}

std::string ADIOSDataSource::GetAttributeType(const std::string& attrName, const std::string& group)
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

std::set<std::string> ADIOSDataSource::GetAttributeNames(const std::string& prefix)
{
  std::set<std::string> result;
  // Match "<prefix>/" so a prefix of "shape/vtkDGHex" does not also pick
  // up a sibling like "shape/vtkDGHexFoo/...". An empty prefix lists the
  // top-level components of every attribute name.
  const std::string match = prefix.empty() ? std::string() : prefix + "/";
  for (const auto& entry : this->AvailAtts)
  {
    const std::string& name = entry.first;
    if (name.size() <= match.size() || name.compare(0, match.size(), match) != 0)
    {
      continue;
    }
    const std::string rest = name.substr(match.size());
    const auto slash = rest.find('/');
    result.insert(slash == std::string::npos ? rest : rest.substr(0, slash));
  }
  return result;
}

}
}
