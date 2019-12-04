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
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPartitionedDataSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkUnstructuredGrid.h"
#include "vtkmDataSet.h"
#include "vtkmlib/ImageDataConverter.h"
#include "vtkmlib/UnstructuredGridConverter.h"

#include <fides/DataSetReader.h>

#include <vtkm/filter/CleanGrid.h>

#include <numeric>

vtkInformationKeyMacro(vtkFidesReader, NUMBER_OF_BLOCKS, Integer);

vtkStandardNewMacro(vtkFidesReader);

struct vtkFidesReader::vtkFidesReaderImpl
{
  std::unique_ptr<fides::io::DataSetReader> Reader;
  std::unordered_map<std::string, std::string> Paths;
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
}

vtkFidesReader::~vtkFidesReader()
{
  this->PointDataArraySelection->Delete();
  this->CellDataArraySelection->Delete();
}

void vtkFidesReader::ParseDataModel(const std::string& fname)
{
  this->Impl->Reader.reset(new fides::io::DataSetReader(fname));
}

void vtkFidesReader::SetDataSourcePath(const std::string& name, const std::string& path)
{
  this->Impl->Paths[name] = path;
}

void vtkFidesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
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

  auto metaData = this->Impl->Reader->ReadMetaData(this->Impl->Paths);

  auto nBlocks = metaData.Get<fides::metadata::Size>(fides::keys::NUMBER_OF_BLOCKS());
  outInfo->Set(NUMBER_OF_BLOCKS(), nBlocks.NumberOfItems);

  if (metaData.Has(fides::keys::FIELDS()))
  {
    auto fields = metaData.Get<fides::metadata::Vector<fides::metadata::FieldInformation>>(
      fides::keys::FIELDS());
    for (auto& field : fields.Data)
    {
      if (field.Association == vtkm::cont::Field::Association::POINTS)
      {
        this->PointDataArraySelection->AddArray(field.Name.c_str());
      }
      else if (field.Association == vtkm::cont::Field::Association::CELL_SET)
      {
        this->CellDataArraySelection->AddArray(field.Name.c_str());
      }
    }
  }

  if (!this->StreamSteps && metaData.Has(fides::keys::NUMBER_OF_STEPS()))
  {
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

}

void vtkFidesReader::PrepareNextStep()
{
  this->Impl->Reader->PrepareNextStep(this->Impl->Paths);
  this->StreamSteps = true;
  this->Modified();
}

int vtkFidesReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkPartitionedDataSet* output = vtkPartitionedDataSet::GetData(outputVector);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);
  int nBlocks = outInfo->Get(NUMBER_OF_BLOCKS());

  int nPieces = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  int piece = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());

  fides::metadata::Vector<size_t> blocksToRead = DetermineBlocksToRead(nBlocks, nPieces, piece);

  fides::metadata::MetaData selections;
  selections.Set(fides::keys::BLOCK_SELECTION(), blocksToRead);

  if (!this->StreamSteps && outInfo->Has(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()))
  {
    int step = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
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
  selections.Set(fides::keys::FIELDS(), arraySelection);

  vtkm::cont::PartitionedDataSet datasets;
  try
  {
    if (this->StreamSteps)
    {
      datasets = this->Impl->Reader->ReadStep(this->Impl->Paths, selections);
    }
    else
    {
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
