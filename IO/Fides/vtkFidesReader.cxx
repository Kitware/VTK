// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkFidesReader.h"

#include "vtkDataArraySelection.h"
#include "vtkFieldData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkStringArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkmDataSet.h"
#include "vtkmlib/ImageDataConverter.h"
#include "vtkmlib/UnstructuredGridConverter.h"
#include "vtksys/SystemTools.hxx"

#include <fides/DataSetReader.h>

#include <vtkm/filter/clean_grid/CleanGrid.h>

#include <numeric>
#include <utility>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkFidesReader);

struct vtkFidesReader::vtkFidesReaderImpl
{
  std::unique_ptr<fides::io::DataSetReader> Reader;
  std::unordered_map<std::string, std::string> Paths;
  bool HasParsedDataModel{ false };
  bool AllDataSourcesSet{ false };
  bool UsePresetModel{ false };
  bool SkipNextPrepareCall{ false };
  int NumberOfDataSources{ 0 };
  bool UseInlineEngine{ false };
  fides::Params AllParams;
  vtkNew<vtkStringArray> SourceNames;

  // Metadata of an individual group in ADIOS file
  // This metadata is populated in RequestInformation.
  // and subsequently used in RequestData
  struct GroupMetaData
  {
    std::size_t NumberOfBlocks;
    std::string Name;
    std::set<std::string> PointDataArrays;
    std::set<std::string> CellDataArrays;
    std::set<std::string> FieldDataArrays;
  };
  std::vector<GroupMetaData> GroupMetaDataCollection;

  // first -> source name, second -> address of IO object
  std::pair<std::string, std::string> IOObjectInfo;

  vtkStringArray* GetDataSourceNames()
  {
    if (this->Reader && this->SourceNames->GetNumberOfValues() == 0)
    {
      auto names = this->Reader->GetDataSourceNames();
      for (const auto& name : names)
      {
        this->SourceNames->InsertNextValue(name);
      }
    }
    return this->SourceNames;
  }

  void SetNumberOfDataSources()
  {
    if (this->Reader)
    {
      auto names = this->Reader->GetDataSourceNames();
      this->NumberOfDataSources = names.size();
    }
  }

  void SetupInlineEngine()
  {
    if (this->IOObjectInfo.first.empty() || this->IOObjectInfo.second.empty())
    {
      return;
    }

    // params has to be set before setting data source
    fides::DataSourceParams params;
    params["engine_type"] = "Inline";
    this->Reader->SetDataSourceParameters(this->IOObjectInfo.first, params);
    this->Reader->SetDataSourceIO(this->IOObjectInfo.first, this->IOObjectInfo.second);
    this->UseInlineEngine = true;
  }
};

vtkFidesReader::vtkFidesReader()
  : Impl(new vtkFidesReaderImpl())
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->FieldDataArraySelection = vtkDataArraySelection::New();
  this->ConvertToVTK = false;
  this->StreamSteps = false;
  this->NextStepStatus = static_cast<StepStatus>(fides::StepStatus::NotReady);
  this->CreateSharedPoints = true;
}

vtkFidesReader::~vtkFidesReader()
{
  this->PointDataArraySelection->Delete();
  this->CellDataArraySelection->Delete();
  this->FieldDataArraySelection->Delete();
}

int vtkFidesReader::CanReadFile(const std::string& name)
{
  if (!vtksys::SystemTools::FileExists(name))
  {
    return 0;
  }
  if (vtksys::SystemTools::StringEndsWith(name, ".bp") ||
    vtksys::SystemTools::StringEndsWith(name, ".bp4") ||
    vtksys::SystemTools::StringEndsWith(name, ".bp5"))
  {
    if (fides::io::DataSetReader::CheckForDataModelAttribute(name))
    {
      return 1;
    }
    return 0;
  }
  if (vtksys::SystemTools::StringEndsWith(name, ".json"))
  {
    return 1;
  }
  return 0;
}

void vtkFidesReader::SetFileName(const std::string& fname)
{
  this->FileName = fname;
  if (vtksys::SystemTools::StringEndsWith(fname, ".bp") ||
    vtksys::SystemTools::StringEndsWith(fname, ".bp4") ||
    vtksys::SystemTools::StringEndsWith(fname, ".bp5"))
  {
    if (fides::io::DataSetReader::CheckForDataModelAttribute(fname))
    {
      this->Impl->UsePresetModel = true;
      vtkDebugMacro(<< "Using a preset data model");
    }
  }
}

void vtkFidesReader::SetDataSourceIO(const std::string& name, const std::string& ioAddress)
{
  if (name.empty() || ioAddress.empty())
  {
    return;
  }
  // can't call SetDataSourceIO in Fides yet, so just save the address for now
  this->Impl->IOObjectInfo = std::make_pair(name, ioAddress);
  this->StreamSteps = true;
  this->Impl->UseInlineEngine = true;
  this->Modified();
}

// This version is used when a json file with the data model is provided
void vtkFidesReader::ParseDataModel(const std::string& fname)
{
  // Should no longer be used; use ParseDataModel() instead
  this->FileName = fname;
  this->ParseDataModel();
}

// This version is used when a pre-defined data model is being used
void vtkFidesReader::ParseDataModel()
{
  // If we have the minimum required info (basically just FileName), then we'll
  // go ahead and create the reader.
  // This opens the reader in a random access mode.
  // If RequestInformation is called again, we may end up deleting it and making
  // a new reader because we have new information about how the reader should
  // actually be opened (e.g., with some type of streaming engine)
  fides::io::DataSetReader::DataModelInput inputType =
    fides::io::DataSetReader::DataModelInput::JSONFile;
  if (this->Impl->UsePresetModel)
  {
    inputType = fides::io::DataSetReader::DataModelInput::BPFile;
  }
  try
  {
    this->Impl->Reader.reset(new fides::io::DataSetReader(this->FileName, inputType,
      this->StreamSteps, this->Impl->AllParams, this->CreateSharedPoints));
  }
  catch (std::exception& e)
  {
    // In some cases it's expected that reading will fail (e.g., not all properties have been set
    // yet), so we don't always want to output the exception. We'll just put it in vtkDebugMacro,
    // so we can just turn it on when we're experiencing some issue.
    vtkDebugMacro(<< "Exception encountered when trying to set up Fides DataSetReader: "
                  << e.what());
    this->Impl->HasParsedDataModel = false;
    return;
  }
  this->Impl->HasParsedDataModel = true;
  this->Impl->SetupInlineEngine();
}

void vtkFidesReader::SetDataSourcePath(const std::string& name, const std::string& path)
{
  if (this->Impl->NumberOfDataSources <= 0)
  {
    this->Impl->SetNumberOfDataSources();
  }
  vtkDebugMacro(<< "Number of data sources: " << this->Impl->NumberOfDataSources);
  vtkDebugMacro(<< "source " << name << "'s path is " << path);
  this->Impl->Paths[name] = path;
  this->Modified();
  if (this->Impl->Paths.size() == static_cast<size_t>(this->Impl->NumberOfDataSources))
  {
    vtkDebugMacro(<< "All data sources have now been set");
    this->Impl->AllDataSourcesSet = true;
  }
}

void vtkFidesReader::SetDataSourceEngine(const std::string& name, const std::string& engine)
{
  if (name.empty() || engine.empty())
  {
    return;
  }
  fides::DataSourceParams params;
  params["engine_type"] = engine;
  vtkDebugMacro(<< "for data source " << name << ", setting ADIOS engine to " << engine);
  this->Impl->AllParams.insert(std::make_pair(name, params));
  this->Modified();
}

void vtkFidesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Filename: " << this->FileName << "\n";
  os << indent << "Convert To VTK: " << this->ConvertToVTK << "\n";
  os << indent << "Stream Steps: " << this->StreamSteps << "\n";
  os << indent << "Next step status: " << this->NextStepStatus << "\n";
  os << indent << "Use Preset model: " << this->Impl->UsePresetModel << "\n";
  os << indent << "Has parsed data model: " << this->Impl->HasParsedDataModel << "\n";
  os << indent << "All data sources set: " << this->Impl->AllDataSourcesSet << "\n";
  os << indent << "Number of data sources: " << this->Impl->NumberOfDataSources << "\n";
  os << indent << "Create shared points: " << this->CreateSharedPoints << "\n";
}

int vtkFidesReader::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    return this->RequestData(request, inputVector, outputVector);
  }

  // execute information
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_INFORMATION()))
  {
    return this->RequestInformation(request, inputVector, outputVector);
  }

  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

int vtkFidesReader::RequestDataObject(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  vtkPartitionedDataSetCollection* output =
    vtkPartitionedDataSetCollection::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    output = vtkPartitionedDataSetCollection::New();
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    output->Delete();
  }
  return 1;
}

int vtkFidesReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (this->StreamSteps && this->NextStepStatus != StepStatus::NotReady)
  {
    // if we're in StreamSteps mode, updating the step status could cause
    // RequestInformation to be called again. In this case, we'll assume
    // that if NextStepStatus is good, that we'll just return here instead
    // of resetting the DataSetReader
    return 1;
  }
  if (this->Impl->UseInlineEngine && this->Impl->HasParsedDataModel)
  {
    // If we're using the Inline engine, we may get unnecessary
    // RequestInformation calls, but we don't want to actually reset the
    // reader
    return 1;
  }

  if (this->Impl->UseInlineEngine)
  {
    // ranks can only access their own data with the inline engine, so
    // we have to set CreateSharedPoints to false. GhostCellsGenerator
    // is currently having a feature added to it, that will fix the gap
    // without Fides needing to do it, that should work for the inline case
    this->CreateSharedPoints = false;
  }

  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // okay so basically we will reset our reader on any call to RequestInfo
  // (except for the situations described above)
  // because we may later get new metadata that determines how we should actually
  // have created the reader, configured adios, etc
  // at bare minimum, FileName has been set, so we can go ahead and call this
  this->ParseDataModel();

  if (!this->Impl->HasParsedDataModel)
  {
    // for some reason we weren't able to set up the fides reader, so just return
    return 1;
  }

  // reset the number of data sources
  this->Impl->SetNumberOfDataSources();
  if (!this->Impl->Paths.empty() &&
    this->Impl->Paths.size() == static_cast<size_t>(this->Impl->NumberOfDataSources))
  {
    vtkDebugMacro(<< "All data sources have now been set");
    this->Impl->AllDataSourcesSet = true;
  }

  // for generated data model, we have to set the paths for sources
  if (this->Impl->UsePresetModel)
  {
    vtkStringArray* sourceNames = this->Impl->GetDataSourceNames();
    vtkDebugMacro(<< this->Impl->NumberOfDataSources << " data sources were found");
    for (int i = 0; i < this->Impl->NumberOfDataSources; ++i)
    {
      // Currently, if there are multiple data sources and we are using a predefined
      // data model, then we'll assume this is XGC. All other predefined data models
      // have only a single data source, and file name is not specified in the data
      // model, so in this case, we need to set the data source path to be the
      // full file name. For XGC, this->FileName is actually to a file containing only
      // attributes, so we just need to grab the directory the attribute file is in
      // to set for each data source.
      std::string path;
      if (this->Impl->NumberOfDataSources == 1)
      {
        path = this->FileName;
      }
      else
      {
        path = vtksys::SystemTools::GetFilenamePath(this->FileName) + "/";
      }
      this->SetDataSourcePath(sourceNames->GetValue(i), path);
    }
  }

  if (this->Impl->NumberOfDataSources == 0 || this->Impl->Paths.empty())
  {
    // no reason to keep going
    // this can happen when using a JSON file instead of a BP file
    return 1;
  }

  if (this->StreamSteps)
  {
    // This isn't as relevant for earlier BP versions, but for BP5 streaming
    // as well as staging engines, we need to do the BeginStep() call before
    // we can read anything, or even just inquire variables/attributes.
    // Fides does need to get some information about variables/attributes
    // in ReadMetaData()
    this->PrepareNextStep();
    this->Impl->SkipNextPrepareCall = true;
  }

  // collection of group metadata will be rebuilt
  this->Impl->GroupMetaDataCollection.clear();
  // get all group names
  auto groupNames = this->Impl->Reader->GetGroupNames(this->Impl->Paths);
  if (groupNames.empty())
  {
    // this is fine. there are no groups in the file.
    // insert a placeholder empty group name, so that the for loop runs once.
    groupNames.insert("");
  }
  for (const auto& groupName : groupNames)
  {
    fides::metadata::MetaData metaData;
    try
    {
      metaData = this->Impl->Reader->ReadMetaData(this->Impl->Paths, groupName);
    }
    catch (...)
    {
      // it's possible that we were able to set Fides up, but reading metadata
      // failed, indicating that not all properties have been set before this
      // RequestInformation call.
      return 1;
    }
    vtkDebugMacro(<< "MetaData has been read by Fides " << (groupName.empty() ? "for group " : "")
                  << groupName);

    vtkFidesReaderImpl::GroupMetaData groupMetaData;
    groupMetaData.Name = groupName;
    groupMetaData.NumberOfBlocks =
      metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_BLOCKS()).NumberOfItems;
    vtkDebugMacro(<< "Number of blocks found in metadata: " << groupMetaData.NumberOfBlocks);

    if (metaData.Has(fides::keys::FIELDS()))
    {
      vtkDebugMacro(<< "Metadata has fields info");
      auto fields = metaData.Get<fides::metadata::Vector<fides::metadata::FieldInformation>>(
        fides::keys::FIELDS());
      for (auto& field : fields.Data)
      {
        if (field.Association == vtkm::cont::Field::Association::Points)
        {
          groupMetaData.PointDataArrays.insert(field.Name);
          this->PointDataArraySelection->AddArray(field.Name.c_str());
        }
        else if (field.Association == vtkm::cont::Field::Association::Cells)
        {
          groupMetaData.CellDataArrays.insert(field.Name);
          this->CellDataArraySelection->AddArray(field.Name.c_str());
        }
        else if (field.Association == vtkm::cont::Field::Association::WholeDataSet)
        {
          groupMetaData.FieldDataArrays.insert(field.Name);
          this->FieldDataArraySelection->AddArray(field.Name.c_str());
        }
      }
    }
    this->Impl->GroupMetaDataCollection.emplace_back(std::move(groupMetaData));
  } // for groupName in groupNames

  fides::metadata::MetaData metaData;
  try
  {
    metaData = this->Impl->Reader->ReadMetaData(this->Impl->Paths);
  }
  catch (...)
  {
    // shouldn't happen, cheap insurance.
    return 1;
  }
  if (!this->StreamSteps && metaData.Has(fides::keys::NUMBER_OF_STEPS()))
  {
    // If there's a time array provided, we'll use that, otherwise, just create an array
    // with consecutive integers for the time
    std::vector<double> times;
    int nSteps;
    if (metaData.Has(fides::keys::TIME_ARRAY()))
    {
      times = metaData.Get<fides::metadata::Vector<double>>(fides::keys::TIME_ARRAY()).Data;
      nSteps = static_cast<int>(times.size());
    }
    else
    {
      nSteps = metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_STEPS()).NumberOfItems;

      times.resize(nSteps);
      std::iota(times.begin(), times.end(), 0);
    }

    double timeRange[2];
    timeRange[0] = times[0];
    timeRange[1] = times[nSteps - 1];
    vtkDebugMacro(<< "time min: " << timeRange[0] << ", time max: " << timeRange[1]);

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), times.data(), nSteps);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }
  outInfo->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);

  return 1;
}

namespace
{
fides::metadata::Vector<size_t> DetermineBlocksToRead(int nBlocks, int nPieces, int piece)
{
  int startPiece, endPiece;
  if (nBlocks > nPieces)
  {
    int nLocalPieces = nBlocks / nPieces;
    startPiece = nLocalPieces * piece;
    endPiece = startPiece + nLocalPieces;
    int remain = nBlocks % nPieces;
    if (remain)
    {
      if (piece < remain)
      {
        startPiece += piece;
        endPiece += piece + 1;
      }
      else
      {
        startPiece += remain;
        endPiece += remain;
      }
    }
  }
  else
  {
    if (piece < nBlocks)
    {
      startPiece = piece;
      endPiece = piece + 1;
    }
    else
    {
      startPiece = nBlocks - 1;
      endPiece = startPiece + 1;
    }
  }

  fides::metadata::Vector<size_t> blocksToRead;
  size_t nBlocksToRead = endPiece - startPiece;
  if (nBlocksToRead > 0)
  {
    blocksToRead.Data.resize(nBlocksToRead);
    std::iota(blocksToRead.Data.begin(), blocksToRead.Data.end(), startPiece);
  }
  return blocksToRead;
}

vtkDataSet* ConvertDataSet(const vtkm::cont::DataSet& ds)
{
  vtkNew<vtkUnstructuredGrid> dstmp;
  const auto& cs = ds.GetCellSet();
  if (cs.IsType<vtkm::cont::CellSetSingleType<>>() || cs.IsType<vtkm::cont::CellSetExplicit<>>())
  {
    vtkUnstructuredGrid* ug = vtkUnstructuredGrid::New();
    fromvtkm::Convert(ds, ug, dstmp);
    return ug;
  }
  else if (cs.IsType<vtkm::cont::CellSetStructured<2>>() ||
    cs.IsType<vtkm::cont::CellSetStructured<3>>())
  {
    const auto& coords = ds.GetCoordinateSystem();
    auto array = coords.GetData();
    if (array.IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>())
    {
      vtkImageData* image = vtkImageData::New();
      fromvtkm::Convert(ds, image, dstmp);
      return image;
    }
  }
  vtkm::filter::clean_grid::CleanGrid filter;
  filter.SetCompactPointFields(false);
  auto result = filter.Execute(ds);
  return ConvertDataSet(result);
}

} // end anon namespace

void vtkFidesReader::PrepareNextStep()
{
  if (!this->Impl->Reader)
  {
    vtkErrorMacro(<< "vtkFidesReader::PrepareNextStep() has been called,"
                     " but Fides has not been set up yet");
    this->NextStepStatus = static_cast<StepStatus>(fides::StepStatus::NotReady);
    return;
  }
  if (this->Impl->SkipNextPrepareCall)
  {
    this->Impl->SkipNextPrepareCall = false;
    return;
  }
  try
  {
    this->NextStepStatus =
      static_cast<StepStatus>(this->Impl->Reader->PrepareNextStep(this->Impl->Paths));
  }
  catch (...)
  {
    return;
  }
  vtkDebugMacro(<< "PrepareNextStep() NextStepStatus = " << this->NextStepStatus);
  this->StreamSteps = true;
  this->Modified();
}

int vtkFidesReader::GetNextStepStatus()
{
  vtkDebugMacro(<< "GetNextStepStatus = " << this->NextStepStatus);
  return this->NextStepStatus;
}

double vtkFidesReader::GetTimeOfCurrentStep()
{
  if (!this->StreamSteps)
  {
    vtkErrorMacro("GetTimeOfCurrentStep() can only be called in streaming mode");
    return 0.0;
  }

  if (this->Impl->NumberOfDataSources == 0)
  {
    this->Impl->SetNumberOfDataSources();
    if (this->Impl->Paths.size() == static_cast<size_t>(this->Impl->NumberOfDataSources))
    {
      vtkDebugMacro(<< "All data sources have now been set");
      this->Impl->AllDataSourcesSet = true;
    }
  }

  if (!this->Impl->HasParsedDataModel || !this->Impl->AllDataSourcesSet)
  {
    vtkErrorMacro(<< "data model has not been parsed or all data sources have not been set");
    return 0.0;
  }

  auto metaData = this->Impl->Reader->ReadMetaData(this->Impl->Paths);
  if (metaData.Has(fides::keys::TIME_VALUE()))
  {
    return metaData.Get<fides::metadata::Time>(fides::keys::TIME_VALUE()).Data;
  }

  vtkErrorMacro(<< "Couldn't grab the time from the Fides metadata");
  return 0.0;
}

int vtkFidesReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->Impl->HasParsedDataModel || !this->Impl->AllDataSourcesSet)
  {
    vtkErrorMacro("RequestData() DataModel must be parsed and all data sources "
                  "must be set before RequestData()");
    return 0;
  }

  if (this->StreamSteps && this->NextStepStatus != StepStatus::OK)
  {
    // This doesn't usually happen, but when using Catalyst Live with
    // Fides, sometimes there's a situation where Catalyst gets updated
    // state from Live and it has NextStepStatus == NotReady. In that case
    // (usually only when running with MPI), one rank will think it needs
    // to call RequestData(), in vtkLiveInsituLink::InsituPostProcess().
    // But PrepareNextStep() will not be called, and so ADIOS will throw
    // an error because EndStep() was called without BeginStep().
    return 1;
  }

  vtkPartitionedDataSetCollection* output = vtkPartitionedDataSetCollection::GetData(outputVector);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  output->SetNumberOfPartitionedDataSets(0);

  fides::metadata::MetaData selections;
  // Select time step if downstream requested a specific time step.
  if (!this->StreamSteps && outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    auto step = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    int index = -1;
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      auto nSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      std::vector<double> allSteps(nSteps);
      outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), allSteps.data());

      double minDiff = VTK_DOUBLE_MAX;
      for (unsigned int i = 0; i < allSteps.size(); i++)
      {
        double diff = std::fabs(allSteps[i] - step);
        if (diff < minDiff)
        {
          minDiff = diff;
          index = i;
        }
      }
    }
    if (index == -1)
    {
      vtkErrorMacro(<< "Couldn't find index of time value " << step);
      index = static_cast<int>(0);
    }
    vtkDebugMacro(<< "RequestData() Not streaming and we have update time step request for step "
                  << step << " with index " << index);
    fides::metadata::Index idx(index);
    selections.Set(fides::keys::STEP_SELECTION(), idx);
  }

  unsigned int pdsIdx = 0;
  for (const auto& groupMetaData : this->Impl->GroupMetaDataCollection)
  {
    int nBlocks = groupMetaData.NumberOfBlocks;
    int nPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
    int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
    vtkDebugMacro(<< "nBlocks: " << nBlocks << ", nPieces: " << nPieces << ", piece: " << piece
                  << (groupMetaData.Name.empty() ? "" : ", groupName: ") << groupMetaData.Name);

    fides::metadata::Vector<size_t> blocksToRead = DetermineBlocksToRead(nBlocks, nPieces, piece);
    if (blocksToRead.Data.empty())
    {
      // nothing to read on this rank
      output->SetNumberOfPartitions(pdsIdx, 0);
      vtkDebugMacro(<< "No blocks to read on this rank; returning");
      continue;
    }
    // Select blocks to read.
    selections.Set(fides::keys::BLOCK_SELECTION(), blocksToRead);
    // Select group.
    selections.Set(fides::keys::GROUP_SELECTION(), fides::metadata::String(groupMetaData.Name));

    using FieldInfoType = fides::metadata::Vector<fides::metadata::FieldInformation>;
    FieldInfoType arraySelection;
    // pick selected arrays from the global data array selection instances.
    for (const auto& aname : groupMetaData.PointDataArrays)
    {
      if (this->PointDataArraySelection->ArrayIsEnabled(aname.c_str()))
      {
        // if this array was enabled on the global point data array selection.
        arraySelection.Data.emplace_back(aname, vtkm::cont::Field::Association::Points);
      }
    }
    for (const auto& aname : groupMetaData.CellDataArrays)
    {
      if (this->CellDataArraySelection->ArrayIsEnabled(aname.c_str()))
      {
        // if this array was enabled on the global cell data array selection.
        arraySelection.Data.emplace_back(aname, vtkm::cont::Field::Association::Cells);
      }
    }
    for (const auto& aname : groupMetaData.FieldDataArrays)
    {
      if (this->FieldDataArraySelection->ArrayIsEnabled(aname.c_str()))
      {
        // if this array was enabled on the global field data array selection.
        arraySelection.Data.emplace_back(aname, vtkm::cont::Field::Association::WholeDataSet);
      }
    }
    selections.Set(fides::keys::FIELDS(), arraySelection);

    vtkm::cont::PartitionedDataSet datasets;
    try
    {
      vtkDebugMacro(<< "RequestData() calling ReadDataSet");
      datasets = this->Impl->Reader->ReadDataSet(this->Impl->Paths, selections);
      if (this->StreamSteps)
      {
        this->NextStepStatus = static_cast<StepStatus>(fides::StepStatus::NotReady);
      }
    }
    catch (std::invalid_argument& e)
    {
      vtkErrorMacro(<< e.what());
      return 0;
    }
    vtkm::Id nParts = datasets.GetNumberOfPartitions();
    output->SetNumberOfPartitions(pdsIdx, nParts);
    std::string datasetName;
    {
      const auto parts = vtksys::SystemTools::SplitString(groupMetaData.Name);
      datasetName = parts.empty() ? "mesh" : parts.back();
    }
    output->GetMetaData(pdsIdx)->Set(vtkCompositeDataSet::NAME(), datasetName.c_str());

    for (vtkm::Id i = 0; i < nParts; i++)
    {
      auto& ds = datasets.GetPartition(i);
      if (this->ConvertToVTK)
      {
        vtkDataSet* vds = ConvertDataSet(ds);
        if (vds)
        {
          output->SetPartition(pdsIdx, i, vds);
          vds->Delete();
        }
      }
      else
      {
        vtkmDataSet* vds = vtkmDataSet::New();
        vds->SetVtkmDataSet(ds);
        output->SetPartition(pdsIdx, i, vds);
        vds->Delete();
      }
    }
    pdsIdx++;
  }

  return 1;
}

int vtkFidesReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSetCollection");
  return 1;
}
VTK_ABI_NAMESPACE_END
