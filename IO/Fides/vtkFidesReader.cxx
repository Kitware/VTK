/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFidesReader.h

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

#include <vtkm/filter/CleanGrid.h>

#include <numeric>

vtkInformationKeyMacro(vtkFidesReader, NUMBER_OF_BLOCKS, Integer);

vtkStandardNewMacro(vtkFidesReader);

struct vtkFidesReader::vtkFidesReaderImpl
{
  std::unique_ptr<fides::io::DataSetReader> Reader;
  std::unordered_map<std::string, std::string> Paths;
  bool HasParsedDataModel{ false };
  bool AllDataSourcesSet{ false };
  bool UsePresetModel{ false };
  bool HasReadMetadata{ false };
  int NumberOfDataSources{ 0 };
  vtkNew<vtkStringArray> SourceNames;

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

// This version is used when a json file with the data model is provided
void vtkFidesReader::ParseDataModel(const std::string& fname)
{
  this->Impl->Reader.reset(new fides::io::DataSetReader(fname));
  this->Impl->HasParsedDataModel = true;
}

// This version is used when a pre-defined data model is being used
void vtkFidesReader::ParseDataModel()
{
  this->Impl->Reader.reset(
    new fides::io::DataSetReader(this->FileName, fides::io::DataSetReader::DataModelInput::BPFile));
  this->Impl->HasParsedDataModel = true;
}

void vtkFidesReader::SetDataSourcePath(const std::string& name, const std::string& path)
{
  if (this->Impl->NumberOfDataSources <= 0)
  {
    this->Impl->SetNumberOfDataSources();
  }
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
  os << indent << "Has read metadata: " << this->Impl->HasReadMetadata << "\n";
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

  // If we're using a preset model, we'll have to do the call to ParseDataModel here
  if (this->Impl->UsePresetModel && !this->Impl->HasParsedDataModel)
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

  if (this->Impl->HasReadMetadata)
  {
    return 1;
  }
  auto metaData = this->Impl->Reader->ReadMetaData(this->Impl->Paths);
  this->Impl->HasReadMetadata = true;
  vtkDebugMacro(<< "MetaData has been read by Fides");

  auto nBlocks = metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_BLOCKS());
  outInfo->Set(NUMBER_OF_BLOCKS(), nBlocks.NumberOfItems);

  if (metaData.Has(fides::keys::FIELDS()))
  {
    vtkDebugMacro(<< "Metadata has fields info");
    auto fields = metaData.Get<fides::metadata::Vector<fides::metadata::FieldInformation>>(
      fides::keys::FIELDS());
    for (auto& field : fields.Data)
    {
      if (field.Association == fides::Association::POINTS)
      {
        this->PointDataArraySelection->AddArray(field.Name.c_str());
      }
      else if (field.Association == fides::Association::CELL_SET)
      {
        this->CellDataArraySelection->AddArray(field.Name.c_str());
      }
      else if (field.Association == fides::Association::FIELD_DATA)
      {
        this->FieldDataArraySelection->AddArray(field.Name.c_str());
      }
    }
  }

  if (!this->StreamSteps && metaData.Has(fides::keys::NUMBER_OF_STEPS()))
  {
    vtkDebugMacro(<< "We are not streaming steps and metadata contains number of steps info");
    size_t nSteps =
      metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_STEPS()).NumberOfItems;

    std::vector<double> times(nSteps);
    std::iota(times.begin(), times.end(), 0);

    double timeRange[2];
    timeRange[0] = times[0];
    timeRange[1] = times[nSteps - 1];

    outInfo->Set(vtkStreamingDemandDrivenPipeline::TIME_STEPS(), &times[0], (int)nSteps);
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
      endPiece = startPiece;
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
  vtkm::filter::CleanGrid filter;
  filter.SetCompactPointFields(false);
  auto result = filter.Execute(ds);
  return ConvertDataSet(result);
}

vtkFieldData* GetVTKFieldData(
  const std::unordered_map<std::string, fides::datamodel::FieldData>& fidesFieldData,
  vtkm::Id partition)
{
  if (fidesFieldData.empty())
  {
    return nullptr;
  }

  vtkFieldData* fieldData = vtkFieldData::New();
  for (const auto& fd : fidesFieldData)
  {
    const std::vector<vtkm::cont::VariantArrayHandle>& fdBlocks = fd.second.GetData();
    if (static_cast<size_t>(partition) < fdBlocks.size())
    {
      auto& array = fdBlocks[partition];
      vtkDataArray* temp = fromvtkm::Convert(array, fd.second.GetName().c_str());
      fieldData->AddArray(temp);
      temp->Delete();
    }
  }
  return fieldData;
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

int vtkFidesReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  if (!this->Impl->HasParsedDataModel || !this->Impl->AllDataSourcesSet)
  {
    vtkErrorMacro("RequestData() DataModel must be parsed and all data sources "
                  "must be set before RequestData()");
    return 0;
  }
  vtkPartitionedDataSet* output = vtkPartitionedDataSet::GetData(outputVector);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int nBlocks = outInfo->Get(NUMBER_OF_BLOCKS());

  int nPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  fides::metadata::Vector<size_t> blocksToRead = DetermineBlocksToRead(nBlocks, nPieces, piece);

  fides::metadata::MetaData selections;
  if (blocksToRead.Data.empty())
  {
    // nothing to read on this rank
    output->SetNumberOfPartitions(0);
    return 1;
  }
  selections.Set(fides::keys::BLOCK_SELECTION(), blocksToRead);

  if (!this->StreamSteps && outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    int step = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    vtkDebugMacro(<< "RequestData() Not streaming and we have update time step request for step "
                  << step);
    fides::metadata::Index idx(step);
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
      arraySelection.Data.push_back(
        fides::metadata::FieldInformation(aname, vtkm::cont::Field::Association::POINTS));
    }
  }
  int nCArrays = this->CellDataArraySelection->GetNumberOfArrays();
  for (int i = 0; i < nCArrays; i++)
  {
    const char* aname = this->CellDataArraySelection->GetArrayName(i);
    if (this->CellDataArraySelection->ArrayIsEnabled(aname))
    {
      arraySelection.Data.push_back(
        fides::metadata::FieldInformation(aname, vtkm::cont::Field::Association::CELL_SET));
    }
  }
  int nFArrays = this->FieldDataArraySelection->GetNumberOfArrays();
  for (int i = 0; i < nFArrays; i++)
  {
    const char* aname = this->FieldDataArraySelection->GetArrayName(i);
    if (this->FieldDataArraySelection->ArrayIsEnabled(aname))
    {
      arraySelection.Data.push_back(
        fides::metadata::FieldInformation(aname, fides::Association::FIELD_DATA));
    }
  }
  selections.Set(fides::keys::FIELDS(), arraySelection);

  vtkm::cont::PartitionedDataSet datasets;
  try
  {
    if (this->StreamSteps)
    {
      vtkDebugMacro(<< "RequestData() calling ReadStep");
      datasets = this->Impl->Reader->ReadStep(this->Impl->Paths, selections);
      this->NextStepStatus = static_cast<StepStatus>(fides::StepStatus::NotReady);
    }
    else
    {
      vtkDebugMacro(<< "RequestData() calling ReadDataSet");
      datasets = this->Impl->Reader->ReadDataSet(this->Impl->Paths, selections);
    }
  }
  catch (std::invalid_argument& e)
  {
    vtkErrorMacro(<< e.what());
    return 0;
  }
  vtkm::Id nParts = datasets.GetNumberOfPartitions();
  output->SetNumberOfPartitions(nParts);

  // need to get field data from fides and save it into a vtkFieldData
  // and add it to the dataset so it can be used in filters.
  const auto& fidesFieldData = this->Impl->Reader->GetFieldData()->GetAllFields();
  vtkDebugMacro(<< "Fides FieldData contains " << fidesFieldData.size() << " fields");
  for (vtkm::Id i = 0; i < nParts; i++)
  {
    auto& ds = datasets.GetPartition(i);
    vtkFieldData* fieldData = GetVTKFieldData(fidesFieldData, i);
    if (this->ConvertToVTK)
    {
      vtkDataSet* vds = ConvertDataSet(ds);
      if (vds)
      {
        if (fieldData)
        {
          vds->SetFieldData(fieldData);
          fieldData->Delete();
        }
        output->SetPartition(i, vds);
        vds->Delete();
      }
    }
    else
    {
      vtkmDataSet* vds = vtkmDataSet::New();
      vds->SetVtkmDataSet(ds);
      if (fieldData)
      {
        vds->SetFieldData(fieldData);
        fieldData->Delete();
      }
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
