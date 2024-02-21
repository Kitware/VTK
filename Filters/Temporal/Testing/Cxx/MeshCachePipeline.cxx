// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "MeshCachePipeline.h"
#include "MeshCacheMockAlgorithms.h"

#include "vtkDataObjectMeshCache.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPolyData.h"

VTK_ABI_NAMESPACE_BEGIN

/**
 * TestPipelineInterface
 */
//------------------------------------------------------------------------------
void TestPipelineInterface::InitializeCache(vtkDataObjectMeshCache* cache)
{
  cache->SetOriginalDataObject(this->GetFilterInputData());
  cache->SetConsumer(this->ConsumerFilter);
  cache->AddOriginalIds(vtkDataObject::POINT, mockArraysName::pointIds);
  cache->UpdateCache(this->GetFilterOutputData());
}

//------------------------------------------------------------------------------
void TestPipelineInterface::MarkConsumerModified()
{
  this->ConsumerFilter->Modified();
}

//------------------------------------------------------------------------------
vtkDataObject* TestPipelineInterface::GetFilterInputData()
{
  return this->ConsumerFilter->GetInput();
}

//------------------------------------------------------------------------------
vtkDataObject* TestPipelineInterface::GetFilterOutputData()
{
  return this->ConsumerFilter->GetOutput();
}

/**
 * TestMeshPipeline
 */
//------------------------------------------------------------------------------
TestMeshPipeline::TestMeshPipeline(bool useghosts)
{
  this->StaticMeshSource->SetGenerateGhosts(useghosts);
  this->StaticMeshSource->Update();
  this->ConsumerFilter->SetInputConnection(this->StaticMeshSource->GetOutputPort());
  this->ConsumerFilter->Update();
}

//------------------------------------------------------------------------------
void TestMeshPipeline::UpdateInputData(int startData)
{
  this->StaticMeshSource->SetStartData(startData);
  this->StaticMeshSource->Update();
}

//------------------------------------------------------------------------------
void TestMeshPipeline::MarkInputMeshModified()
{
  this->StaticMeshSource->MarkMeshModified();
  this->StaticMeshSource->Update();
}

//------------------------------------------------------------------------------
vtkMTimeType TestMeshPipeline::GetInputMeshMTime()
{
  auto polydata = vtkPolyData::SafeDownCast(this->GetFilterInputData());
  return polydata->GetMeshMTime();
}

//------------------------------------------------------------------------------
vtkMTimeType TestMeshPipeline::GetOutputMeshMTime()
{
  auto polydata = vtkPolyData::SafeDownCast(this->GetFilterOutputData());
  return polydata->GetMeshMTime();
}

//------------------------------------------------------------------------------
void TestMeshPipeline::MarkGhostsModified()
{
  this->StaticMeshSource->MarkGhostsModified();
  this->StaticMeshSource->Update();
}

//------------------------------------------------------------------------------
void TestMeshPipeline::SetUseGhosts(bool useghost)
{
  this->StaticMeshSource->SetGenerateGhosts(useghost);
  this->StaticMeshSource->Update();
}

/**
 * TestCompositePipeline
 */
//------------------------------------------------------------------------------
TestCompositePipeline::TestCompositePipeline()
{
  this->ConsumerFilter->SetInputConnection(this->StaticCompositeSource->GetOutputPort());
  this->ConsumerFilter->Update();
}

//------------------------------------------------------------------------------
void TestCompositePipeline::UpdateInputData(int start)
{
  this->StaticCompositeSource->SetStartData(start);
  this->StaticCompositeSource->Update();
  this->ConsumerFilter->Update();
}

//------------------------------------------------------------------------------
void TestCompositePipeline::MarkInputMeshModified()
{
  this->StaticCompositeSource->MarkMeshModified();
  this->StaticCompositeSource->Update();
}

//------------------------------------------------------------------------------
vtkMTimeType TestCompositePipeline::GetInputMeshMTime()
{
  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(this->GetFilterInputData());
  auto poly = vtkPolyData::SafeDownCast(pdc->GetPartition(1, 0));

  return poly->GetMeshMTime();
}

//------------------------------------------------------------------------------
vtkMTimeType TestCompositePipeline::GetOutputMeshMTime()
{
  auto pdc = vtkPartitionedDataSetCollection::SafeDownCast(this->GetFilterOutputData());
  auto poly = vtkPolyData::SafeDownCast(pdc->GetPartition(1, 0));

  return poly->GetMeshMTime();
}

VTK_ABI_NAMESPACE_END
