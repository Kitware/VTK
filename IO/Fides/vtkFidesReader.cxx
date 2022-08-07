/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFidesReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

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

vtkInformationKeyMacro(vtkFidesReader, NUMBER_OF_BLOCKS, Integer);

vtkStandardNewMacro(vtkFidesReader);

struct vtkFidesReader::vtkFidesReaderImpl
{
  std::unique_ptr<fides::io::DataSetReader> Reader;
  std::unordered_map<std::string, std::string> Paths;
  bool HasParsedDataModel{ false };
  bool AllDataSourcesSet{ false };
  bool UsePresetModel{ false };
  int NumberOfDataSources{ 0 };
  vtkNew<vtkStringArray> SourceNames;

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
  }
};

vtkFidesReader::vtkFidesReader()
  : Impl(new vtkFidesReaderImpl())
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->PointDataArraySelection = vtkDataArraySelection::New();
  this->CellDataArraySelection = vtkDataArraySelection::New();
  this->ConvertToVTK = false;
  this->StreamSteps = false;
  this->NextStepStatus = static_cast<StepStatus>(fides::StepStatus::NotReady);
}

vtkFidesReader::~vtkFidesReader()
{
  this->PointDataArraySelection->Delete();
  this->CellDataArraySelection->Delete();
}

int vtkFidesReader::CanReadFile(const std::string& name)
{
  if (!vtksys::SystemTools::FileExists(name))
  {
    return 0;
  }
  if (vtksys::SystemTools::StringEndsWith(name, ".bp"))
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
  if (vtksys::SystemTools::StringEndsWith(fname, ".bp"))
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
  // can't call SetDataSourceIO in Fides yet, so just save the address for now
  this->Impl->IOObjectInfo = std::make_pair(name, ioAddress);
  this->StreamSteps = true;
  this->Modified();
}

// This version is used when a json file with the data model is provided
void vtkFidesReader::ParseDataModel(const std::string& fname)
{
  this->Impl->Reader.reset(new fides::io::DataSetReader(fname));
  this->Impl->HasParsedDataModel = true;
  this->Impl->SetupInlineEngine();
}

// This version is used when a pre-defined data model is being used
void vtkFidesReader::ParseDataModel()
{
  this->Impl->Reader.reset(
    new fides::io::DataSetReader(this->FileName, fides::io::DataSetReader::DataModelInput::BPFile));
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
  vtkPartitionedDataSet* output =
    vtkPartitionedDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));
  if (!output)
  {
    output = vtkPartitionedDataSet::New();
    outInfo->Set(vtkDataObject::DATA_OBJECT(), output);
    output->Delete();
  }
  return 1;
}

int vtkFidesReader::RequestInformation(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (this->Impl->NumberOfDataSources == 0)
  {
    this->Impl->SetNumberOfDataSources();
    if (this->Impl->Paths.size() == static_cast<size_t>(this->Impl->NumberOfDataSources))
    {
      vtkDebugMacro(<< "All data sources have now been set");
      this->Impl->AllDataSourcesSet = true;
    }
  }

  if (!this->Impl->UsePresetModel && !this->Impl->HasParsedDataModel)
  {
    this->ParseDataModel(this->FileName);
    if (this->StreamSteps)
    {
      // when streaming UpdateInformation() should be called to get Fides set
      // up, but don't read metadata yet.
      return 1;
    }
  }
  else if (this->Impl->UsePresetModel && !this->Impl->HasParsedDataModel)
  {
    vtkDebugMacro(<< "using preset model but hasn't been parsed yet");
    this->ParseDataModel();
    vtkStringArray* sourceNames = this->Impl->GetDataSourceNames();
    this->Impl->NumberOfDataSources = sourceNames->GetNumberOfValues();
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
  else if (!this->Impl->HasParsedDataModel || !this->Impl->AllDataSourcesSet)
  {
    vtkErrorMacro(
      << "RequestInfo() has not parsed data model and all data sources have not been set");
    return 1;
  }

  auto metaData = this->Impl->Reader->ReadMetaData(this->Impl->Paths);
  vtkDebugMacro(<< "MetaData has been read by Fides");

  auto nBlocks = metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_BLOCKS());
  outInfo->Set(NUMBER_OF_BLOCKS(), nBlocks.NumberOfItems);
  vtkDebugMacro(<< "Number of blocks found in metadata: " << nBlocks.NumberOfItems);
  outInfo->Set(vtkAlgorithm::CAN_HANDLE_PIECE_REQUEST(), 1);

  if (metaData.Has(fides::keys::FIELDS()))
  {
    vtkDebugMacro(<< "Metadata has fields info");
    auto fields = metaData.Get<fides::metadata::Vector<fides::metadata::FieldInformation>>(
      fides::keys::FIELDS());
    for (auto& field : fields.Data)
    {
      if (field.Association == vtkm::cont::Field::Association::Points)
      {
        this->PointDataArraySelection->AddArray(field.Name.c_str());
      }
      else if (field.Association == vtkm::cont::Field::Association::Cells)
      {
        this->CellDataArraySelection->AddArray(field.Name.c_str());
      }
    }
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

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), times.data(), nSteps);
    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

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
  this->NextStepStatus =
    static_cast<StepStatus>(this->Impl->Reader->PrepareNextStep(this->Impl->Paths));
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

  vtkPartitionedDataSet* output = vtkPartitionedDataSet::GetData(outputVector);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int nBlocks = outInfo->Get(NUMBER_OF_BLOCKS());

  int nPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  vtkDebugMacro(<< "nBlocks: " << nBlocks << ", nPieces: " << nPieces << ", piece: " << piece);

  fides::metadata::Vector<size_t> blocksToRead = DetermineBlocksToRead(nBlocks, nPieces, piece);

  fides::metadata::MetaData selections;
  if (blocksToRead.Data.empty())
  {
    // nothing to read on this rank
    output->SetNumberOfPartitions(0);
    vtkDebugMacro(<< "No blocks to read on this rank; returning");
    return 1;
  }
  selections.Set(fides::keys::BLOCK_SELECTION(), blocksToRead);

  if (!this->StreamSteps && outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    auto step = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    int index = -1;
    if (outInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_STEPS()))
    {
      auto nSteps = outInfo->Length(vtkStreamingDemandDrivenPipeline::TIME_STEPS());
      std::vector<double> allSteps(nSteps);
      outInfo->Get(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), allSteps.data());
      auto it = std::find(allSteps.begin(), allSteps.end(), step);
      if (it != allSteps.end())
      {
        index = it - allSteps.begin();
      }
    }
    if (index == -1)
    {
      vtkErrorMacro(<< "Couldn't find index of time value " << step);
      index = static_cast<int>(step);
    }
    vtkDebugMacro(<< "RequestData() Not streaming and we have update time step request for step "
                  << step);
    fides::metadata::Index idx(index);
    selections.Set(fides::keys::STEP_SELECTION(), idx);
  }

  using FieldInfoType = fides::metadata::Vector<fides::metadata::FieldInformation>;
  FieldInfoType arraySelection;
  int nArrays = this->PointDataArraySelection->GetNumberOfArrays();
  for (int i = 0; i < nArrays; i++)
  {
    const char* aname = this->PointDataArraySelection->GetArrayName(i);
    if (this->PointDataArraySelection->ArrayIsEnabled(aname))
    {
      arraySelection.Data.emplace_back(aname, vtkm::cont::Field::Association::Points);
    }
  }
  int nCArrays = this->CellDataArraySelection->GetNumberOfArrays();
  for (int i = 0; i < nCArrays; i++)
  {
    const char* aname = this->CellDataArraySelection->GetArrayName(i);
    if (this->CellDataArraySelection->ArrayIsEnabled(aname))
    {
      arraySelection.Data.emplace_back(aname, vtkm::cont::Field::Association::Cells);
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
  output->SetNumberOfPartitions(nParts);

  for (vtkm::Id i = 0; i < nParts; i++)
  {
    auto& ds = datasets.GetPartition(i);
    if (this->ConvertToVTK)
    {
      vtkDataSet* vds = ConvertDataSet(ds);
      if (vds)
      {
        output->SetPartition(i, vds);
        vds->Delete();
      }
    }
    else
    {
      vtkmDataSet* vds = vtkmDataSet::New();
      vds->SetVtkmDataSet(ds);
      output->SetPartition(i, vds);
      vds->Delete();
    }
  }

  return 1;
}

int vtkFidesReader::FillOutputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // now add our info
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkPartitionedDataSet");
  return 1;
}
