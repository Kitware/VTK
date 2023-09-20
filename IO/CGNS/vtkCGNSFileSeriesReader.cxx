// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCGNSFileSeriesReader.h"

#include "vtkCGNSReader.h"
#include "vtkCommand.h"
#include "vtkCommunicator.h"
#include "vtkDataSet.h"
#include "vtkFileSeriesHelper.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMultiProcessStream.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtksys/RegularExpression.hxx"
#include "vtksys/SystemTools.hxx"

#include <cassert>
#include <map>
#include <set>
#include <sstream>
#include <string>

namespace
{
template <class T>
class SCOPED_SET
{
  T& Var;
  T Prev;

public:
  SCOPED_SET(T& var, const T& val)
    : Var(var)
    , Prev(var)
  {
    this->Var = val;
  }
  ~SCOPED_SET() { this->Var = this->Prev; }

private:
  SCOPED_SET(const SCOPED_SET&) = delete;
  void operator=(const SCOPED_SET&) = delete;
};
}
VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCGNSFileSeriesReader);
vtkCxxSetObjectMacro(vtkCGNSFileSeriesReader, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkCGNSFileSeriesReader::vtkCGNSFileSeriesReader()
  : Reader(nullptr)
  , IgnoreReaderTime(false)
  , Controller(nullptr)
  , ReaderObserverId(0)
  , InProcessRequest(false)
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->SetController(vtkMultiProcessController::GetGlobalController());
}

//----------------------------------------------------------------------------
vtkCGNSFileSeriesReader::~vtkCGNSFileSeriesReader()
{
  this->SetReader(nullptr);
  this->SetController(nullptr);
}

//----------------------------------------------------------------------------
int vtkCGNSFileSeriesReader::CanReadFile(const char* filename)
{
  return this->Reader ? this->Reader->CanReadFile(filename) : 0;
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Reader: " << this->Reader << endl;
  os << indent << "IgnoreReaderTime: " << this->IgnoreReaderTime << endl;
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::AddFileName(const char* fname)
{
  this->FileSeriesHelper->AddFileName(fname);
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::RemoveAllFileNames()
{
  this->FileSeriesHelper->RemoveAllFileNames();
}

//----------------------------------------------------------------------------
const char* vtkCGNSFileSeriesReader::GetCurrentFileName() const
{
  return this->Reader ? this->Reader->GetFileName() : nullptr;
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::SetReader(vtkCGNSReader* reader)
{
  if (this->Reader == reader)
  {
    return;
  }

  if (this->Reader)
  {
    this->RemoveObserver(this->ReaderObserverId);
  }
  vtkSetObjectBodyMacro(Reader, vtkCGNSReader, reader);
  if (this->Reader)
  {
    this->ReaderObserverId = this->Reader->AddObserver(
      vtkCommand::ModifiedEvent, this, &vtkCGNSFileSeriesReader::OnReaderModifiedEvent);
  }
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::OnReaderModifiedEvent()
{
  if (!this->InProcessRequest)
  {
    this->Modified();
  }
}

//----------------------------------------------------------------------------
vtkTypeBool vtkCGNSFileSeriesReader::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (!this->Reader)
  {
    vtkErrorMacro("`Reader` cannot be NULL.");
    return 0;
  }

  int requestFromPort = request->Has(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    ? request->Get(vtkStreamingDemandDrivenPipeline::FROM_OUTPUT_PORT())
    : 0;
  assert(requestFromPort < this->GetNumberOfOutputPorts());
  vtkInformation* outInfo = outputVector->GetInformationObject(requestFromPort);

  assert(this->InProcessRequest == false);
  SCOPED_SET<bool> markInProgress(this->InProcessRequest, true);

  // Since we are dealing with potentially temporal or partitioned fileseries, a
  // single rank may have to read more than 1 file. Before processing any
  // pipeline pass, let's make sure we have built up the set of active files.
  if (!this->UpdateActiveFileSet(outInfo))
  {
    return 0;
  }

  // Before we continue processing the request, let's decide what mode should
  // the internal vtkCGNSReader work i.e. should it handle parallel processing
  // by splitting blocks across ranks, or are we letting
  // vtkCGNSReaderFileSeriesReader split files among ranks.
  if (this->FileSeriesHelper->GetPartitionedFiles())
  {
    this->Reader->SetController(nullptr);
    this->Reader->SetDistributeBlocks(false);
  }
  else
  {
    this->Reader->SetController(this->Controller);
    this->Reader->SetDistributeBlocks(true);
  }

  if (this->FileSeriesHelper->GetPartitionedFiles() &&
    request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_DATA()))
  {
    // For REQUEST_DATA(), we need to iterate over all files in the active
    // set.
    if (!this->RequestData(request, inputVector, outputVector))
    {
      return 0;
    }
  }
  else
  {
    // For most pipeline passes, it's sufficient to choose the first file in the
    // active set, if any, and then pass the request to the internal reader.
    if (!this->ActiveFiles.empty())
    {
      this->ChooseActiveFile(0);
      if (!this->Reader->ProcessRequest(request, inputVector, outputVector))
      {
        return 0;
      }
    }
  }

  // restore time information.
  this->FileSeriesHelper->FillTimeInformation(outInfo);
  return 1;
}
VTK_ABI_NAMESPACE_END

namespace vtkCGNSFileSeriesReaderNS
{
VTK_ABI_NAMESPACE_BEGIN
static bool SetFileNameCallback(vtkAlgorithm* reader, const std::string& fname)
{
  if (vtkCGNSReader* cgnsReader = vtkCGNSReader::SafeDownCast(reader))
  {
    cgnsReader->SetFileName(fname.c_str());
    return true;
  }
  return false;
}
VTK_ABI_NAMESPACE_END
}

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------
bool vtkCGNSFileSeriesReader::UpdateActiveFileSet(vtkInformation* outInfo)
{
  // Pass ivars to vtkFileSeriesHelper.
  this->FileSeriesHelper->SetIgnoreReaderTime(this->IgnoreReaderTime);

  // We pass a new instance of the reader to vtkFileSeriesHelper to avoid
  // mucking with this->Reader to just gather the file's time meta-data.
  vtkSmartPointer<vtkCGNSReader> reader =
    vtkSmartPointer<vtkCGNSReader>::Take(this->Reader->NewInstance());
  reader->SetController(nullptr);
  reader->SetDistributeBlocks(false);

  // Update vtkFileSeriesHelper. Make it process all the filenames provided and
  // collect useful metadata from it. This is a no-op if the vtkFileSeriesHelper
  // wasn't modified.
  if (!this->FileSeriesHelper->UpdateInformation(
        reader.Get(), vtkCGNSFileSeriesReaderNS::SetFileNameCallback))
  {
    return false;
  }

  // For current time/local partition, we need to determine which files to
  // read. Let's determine that.
  this->ActiveFiles = this->FileSeriesHelper->GetActiveFiles(outInfo);

  // For temporal file series, the active set should only have 1 file. If more than 1
  // file matches the timestep, it means that we may have invalid time information
  // in the file series. Warn about it.
  if (!this->FileSeriesHelper->GetPartitionedFiles() && this->ActiveFiles.size() > 1)
  {
    vtkWarningMacro(
      "The CGNS file series may have incorrect (or duplicate) "
      "time values for a temporal file series. You may want to turn on 'IgnoreReaderTime'.");
  }
  return true;
}

//----------------------------------------------------------------------------
void vtkCGNSFileSeriesReader::ChooseActiveFile(int index)
{
  std::string fname =
    (index < static_cast<int>(this->ActiveFiles.size())) ? this->ActiveFiles[index] : std::string();
  if (this->Reader->GetFileName() == nullptr || this->Reader->GetFileName() != fname)
  {
    this->Reader->SetFileName(fname.c_str());
    this->Reader->UpdateInformation();
  }
}

//----------------------------------------------------------------------------
#if defined(_MSC_VER)
// VS generates `warning C4503 : decorated name length exceeded, name was truncated`
// warning for the `BasesMap` data structure defined below. Since it's a private
// data structure, we ignore this warning.
#pragma warning(disable : 4503)
#endif
VTK_ABI_NAMESPACE_END

namespace
{

/**
 * This helps me sync up the multiblock structure across ranks.
 * This is a little hard-coded to the output of CGNS reader. It may be worthwhile to
 * generalize this to a filter and then simply use that.
 */
struct ANode
{
  std::map<std::string, ANode*> Children;
  std::vector<vtkSmartPointer<vtkDataSet>> Datasets;
  ANode() = default;
  ~ANode()
  {
    for (auto iter = this->Children.begin(); iter != this->Children.end(); ++iter)
    {
      delete iter->second;
    }
  }

  void Add(vtkMultiBlockDataSet* mb)
  {
    vtksys::RegularExpression nameRe("^(.*)_proc-([0-9]+)$");
    for (unsigned int cc = 0, max = mb->GetNumberOfBlocks(); cc < max; ++cc)
    {
      std::string name = mb->GetMetaData(cc)->Get(vtkCompositeDataSet::NAME());
      if (nameRe.find(name))
      {
        name = nameRe.match(1);
      }

      auto citer = this->Children.find(name);
      if (citer == this->Children.end())
      {
        ANode* child = new ANode();
        this->Children[name] = child;
        child->Add(mb->GetBlock(cc));
      }
      else
      {
        citer->second->Add(mb->GetBlock(cc));
      }
    }
  }

  vtkSmartPointer<vtkDataObject> Get() const
  {
    if (this->Children.empty())
    {
      if (this->Datasets.size() == 1)
      {
        return this->Datasets.front();
      }
      else if (!this->Datasets.empty())
      {
        vtkNew<vtkMultiPieceDataSet> mp;
        mp->SetNumberOfPieces(static_cast<unsigned int>(this->Datasets.size()));
        for (unsigned int cc = 0; cc < mp->GetNumberOfPieces(); ++cc)
        {
          mp->SetPiece(cc, this->Datasets[cc]);
        }
        return mp.Get();
      }
      return nullptr;
    }
    else
    {
      vtkNew<vtkMultiBlockDataSet> mb;
      mb->SetNumberOfBlocks(static_cast<unsigned int>(this->Children.size()));
      unsigned int blockNo = 0;
      for (auto iter = this->Children.begin(); iter != this->Children.end(); ++iter, ++blockNo)
      {
        mb->SetBlock(blockNo, iter->second->Get());
        mb->GetMetaData(blockNo)->Set(vtkCompositeDataSet::NAME(), iter->first.c_str());
      }
      return mb.Get();
    }
  }

  bool SyncMetadata(vtkMultiProcessController* controller)
  {
    // note: this is not optimized to deep trees.
    const unsigned int child_count = static_cast<unsigned int>(this->Children.size());
    unsigned int max_child_count;
    controller->AllReduce(&child_count, &max_child_count, 1, vtkCommunicator::MAX_OP);

    unsigned int ds_count = static_cast<unsigned int>(this->Datasets.size());
    unsigned int total_ds_count;
    controller->AllReduce(&ds_count, &total_ds_count, 1, vtkCommunicator::SUM_OP);

    assert(max_child_count == 0 || total_ds_count == 0); // one of two must be 0.
    if (max_child_count > 0)
    {
      std::set<std::string> cnames;
      for (auto citer = this->Children.begin(); citer != this->Children.end(); ++citer)
      {
        cnames.insert(citer->first);
      }
      this->AllReduce(cnames, controller);
      for (auto cniter = cnames.begin(); cniter != cnames.end(); ++cniter)
      {
        const std::string& name = (*cniter);
        if (this->Children.find(name) == this->Children.end())
        {
          this->Children[name] = new ANode();
        }
      }
      // Sync all children.
      for (auto citer = this->Children.begin(); citer != this->Children.end(); ++citer)
      {
        citer->second->SyncMetadata(controller);
      }
    }
    else if (total_ds_count > 0)
    {
      this->Datasets.resize(total_ds_count);
    }
    return true;
  }

private:
  void Add(vtkDataObject* dobj)
  {
    if (vtkMultiBlockDataSet* mb = vtkMultiBlockDataSet::SafeDownCast(dobj))
    {
      this->Add(mb);
    }
    else
    {
      this->Datasets.emplace_back(vtkDataSet::SafeDownCast(dobj));
    }
  }

  void AllReduce(std::set<std::string>& names, vtkMultiProcessController* controller)
  {
    std::ostringstream str;
    for (auto iter = names.begin(); iter != names.end(); ++iter)
    {
      str << (*iter) << '\n';
    }

    int slen = static_cast<int>(str.str().size()) + 1; // includes room for null terminator.
    int maxslen = 0;
    controller->AllReduce(&slen, &maxslen, 1, vtkCommunicator::MAX_OP);

    int numRanks = controller->GetNumberOfProcesses();
    std::vector<char> buffer(numRanks * maxslen, '\0');
    std::vector<char> sbuffer(maxslen, '\0');
    strcpy(sbuffer.data(), str.str().c_str());

    controller->AllGather(sbuffer.data(), buffer.data(), maxslen);
    names.clear();
    for (int cc = 0; cc < numRanks; ++cc)
    {
      std::vector<std::string> rnames;
      vtksys::SystemTools::Split(std::string(&buffer[cc * maxslen]), rnames, '\n');
      names.insert(rnames.begin(), rnames.end());
    }
  }
};
}

VTK_ABI_NAMESPACE_BEGIN
//----------------------------------------------------------------------------
int vtkCGNSFileSeriesReader::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // iterate over all files in the active set and collect the data.
  ANode hierarchy;

  int success = 1;
  for (size_t cc = 0, max = this->ActiveFiles.size(); cc < max; ++cc)
  {
    this->ChooseActiveFile(static_cast<int>(cc));
    if (this->Reader->ProcessRequest(request, inputVector, outputVector))
    {
      vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
      assert(output);
      hierarchy.Add(output);
      output->Initialize();
    }
    else
    {
      vtkErrorMacro("Failed to read '" << this->GetCurrentFileName() << "'");
      success = 0;
      break;
    }
  }

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    int allSuccess = 0;
    this->Controller->AllReduce(&success, &allSuccess, 1, vtkCommunicator::MIN_OP);
    if (allSuccess == 0)
    {
      return 0;
    }
  }

  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    // Ensure all ranks have same meta-data about the number of bases and zones.
    hierarchy.SyncMetadata(this->Controller);
  }

  vtkMultiBlockDataSet* output = vtkMultiBlockDataSet::GetData(outputVector, 0);
  output->Initialize();
  output->CompositeShallowCopy(vtkMultiBlockDataSet::SafeDownCast(hierarchy.Get()));
  return 1;
}
VTK_ABI_NAMESPACE_END
