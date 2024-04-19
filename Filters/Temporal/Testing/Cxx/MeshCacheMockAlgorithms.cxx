// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "MeshCacheMockAlgorithms.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataObjectMeshCache.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"

#include <numeric>

VTK_ABI_NAMESPACE_BEGIN

constexpr int NB_OF_POINTS = 4;

/************************************************************************
 * vtkStaticDataSource
 ***********************************************************************/
vtkStandardNewMacro(vtkStaticDataSource);

//------------------------------------------------------------------------------
vtkStaticDataSource::vtkStaticDataSource()
{
  this->SetNumberOfInputPorts(0);

  const int numberOfPoints = NB_OF_POINTS;
  vtkNew<vtkPoints> points;
  points->SetNumberOfPoints(numberOfPoints);
  points->SetPoint(0, 0.0, 0.0, 0.0);
  points->SetPoint(1, 1.0, 0.0, 0.0);
  points->SetPoint(2, 0.0, 1.0, 0.0);
  points->SetPoint(3, 1.0, 1.0, 1.0);

  vtkNew<vtkCellArray> cells;
  cells->InsertNextCell(3);
  cells->InsertCellPoint(0);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(2);

  cells->InsertNextCell(3);
  cells->InsertCellPoint(1);
  cells->InsertCellPoint(2);
  cells->InsertCellPoint(3);

  this->SourceOutput->SetPoints(points);
  this->SourceOutput->SetPolys(cells);

  vtkNew<vtkIdTypeArray> ids;
  ids->SetName(mockArraysName::pointIds.c_str());
  ids->SetNumberOfTuples(numberOfPoints);
  this->SourceOutput->GetPointData()->SetGlobalIds(ids);
  auto idsRange = vtk::DataArrayValueRange(ids);
  std::iota(idsRange.begin(), idsRange.end(), 0);

  vtkCellData* cd = this->SourceOutput->GetCellData();
  vtkNew<vtkUnsignedCharArray> ghostCells;
  ghostCells->SetName(vtkDataSetAttributes::GhostArrayName());
  ghostCells->InsertNextValue(0);
  ghostCells->InsertNextValue(vtkDataSetAttributes::HIDDENCELL);
  cd->AddArray(ghostCells);
}

//------------------------------------------------------------------------------
int vtkStaticDataSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkNew<vtkIntArray> data;
  data->SetName(mockArraysName::pointData.c_str());
  data->SetNumberOfTuples(NB_OF_POINTS);
  this->SourceOutput->GetPointData()->AddArray(data);
  auto dataRange = vtk::DataArrayValueRange(data);
  std::iota(dataRange.begin(), dataRange.end(), this->StartData);

  if (!this->GenerateGhosts)
  {
    this->SourceOutput->GetCellData()->Initialize();
  }

  vtkPolyData* output = vtkPolyData::GetData(outputVector);
  output->ShallowCopy(this->SourceOutput);

  return 1;
}

//------------------------------------------------------------------------------
void vtkStaticDataSource::MarkMeshModified()
{
  this->SourceOutput->GetPoints()->Modified();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkStaticDataSource::MarkGhostsModified()
{
  vtkCellData* cd = this->SourceOutput->GetCellData();
  auto ghostCells = cd->GetArray(vtkDataSetAttributes::GhostArrayName());
  if (ghostCells)
  {
    ghostCells->Modified();
  }
}

/************************************************************************
 * vtkStaticCompositeSource
 ***********************************************************************/
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkStaticCompositeSource);

//------------------------------------------------------------------------------
vtkStaticCompositeSource::vtkStaticCompositeSource()
{
  this->SetNumberOfInputPorts(0);

  this->FirstData->Update();
  this->SecondData->SetStartData(this->FirstData->GetOutput()->GetNumberOfPoints());
  this->SecondData->Update();

  this->SourceOutput->SetPartition(0, 0, this->FirstData->GetOutput());
  this->SourceOutput->SetPartition(1, 0, this->SecondData->GetOutput());
}

//------------------------------------------------------------------------------
int vtkStaticCompositeSource::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkCompositeDataSet* output = vtkCompositeDataSet::GetData(outputVector);
  output->ShallowCopy(this->SourceOutput);

  return 1;
}

//------------------------------------------------------------------------------
void vtkStaticCompositeSource::SetStartData(int start)
{
  this->FirstData->SetStartData(start);
  this->FirstData->Update();
  this->SecondData->SetStartData(NB_OF_POINTS + start);
  this->SecondData->Update();
  this->Modified();
}

//------------------------------------------------------------------------------
void vtkStaticCompositeSource::MarkMeshModified()
{
  this->FirstData->MarkMeshModified();
  this->FirstData->Update();
  this->SecondData->MarkMeshModified();
  this->SecondData->Update();
  this->Modified();
}

/************************************************************************
 * vtkConsumerDataFilter
 ***********************************************************************/
//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkConsumerDataFilter);

//------------------------------------------------------------------------------
int vtkConsumerDataFilter::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataObject* input = vtkDataObject::GetData(inputVector[0]);
  vtkDataObject* output = vtkDataObject::GetData(outputVector);
  output->ShallowCopy(input);
  return 1;
}

//------------------------------------------------------------------------------
vtkCompositeDataSet* vtkConsumerDataFilter::GetCompositeOutput()
{
  return vtkCompositeDataSet::SafeDownCast(this->GetOutput());
}

VTK_ABI_NAMESPACE_END
