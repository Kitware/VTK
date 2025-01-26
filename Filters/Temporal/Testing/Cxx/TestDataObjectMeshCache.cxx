// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDataObject.h"
#include "vtkDataObjectMeshCache.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSetGet.h"
#include "vtkTable.h"
#include "vtkTestErrorObserver.h"
#include "vtkTestUtilities.h"

#include "MeshCacheMockAlgorithms.h"
#include "MeshCachePipeline.h"

#include <memory>

namespace details
{
std::array<int, 4> modifiedData = { 42, 43, 44, 45 };
}

namespace compositeDetails
{
std::array<int, 4> modifiedData = { 100, 101, 102, 103 };
std::array<int, 4> modifiedData2 = { 104, 105, 106, 107 };

void setupExpectedArray(
  vtkPartitionedDataSetCollection* pdc, int partition, const std::array<int, 4>& data)
{
  vtkNew<vtkIntArray> expectedArray;
  expectedArray->SetName(mockArraysName::pointData.c_str());
  for (int value : data)
  {
    expectedArray->InsertNextValue(value);
  }
  pdc->GetPartition(partition, 0)->GetPointData()->AddArray(expectedArray);
}
};

//------------------------------------------------------------------------------
/**
 * Check status of a default constructed cache.
 */
bool TestDefault()
{
  vtkNew<vtkDataObjectMeshCache> cache;
  vtkDataObjectMeshCache::Status status = cache->GetStatus();

  vtkDataObjectMeshCache::Status expected;
  expected.OriginalDataDefined = false;
  expected.ConsumerDefined = false;
  expected.CacheDefined = false;
  expected.OriginalMeshUnmodified = false;
  expected.ConsumerUnmodified = false;
  expected.AttributesIdsExists = true;

  bool error = (status != expected) || status.enabled();
  vtkLogIf(ERROR, error, "Uninitialized cache should not be usable.");

  return !error;
}

//------------------------------------------------------------------------------
/**
 * Initialize a cache step by step.
 * Check status at each step.
 */
bool TestCacheInitialization(TestPipelineInterface* pipeline)
{
  vtkNew<vtkDataObjectMeshCache> cache;

  vtkDataObjectMeshCache::Status expected;
  expected.OriginalDataDefined = false;
  expected.ConsumerDefined = false;
  expected.CacheDefined = false;
  expected.OriginalMeshUnmodified = false;
  expected.ConsumerUnmodified = false;
  expected.AttributesIdsExists = true;

  // set input
  cache->SetOriginalDataObject(pipeline->GetFilterInputData());
  expected.OriginalDataDefined = true;
  vtkDataObjectMeshCache::Status status = cache->GetStatus();
  if ((status != expected) || status.enabled())
  {
    vtkLog(ERROR, "CacheInitialization: error with input setup.");
    return false;
  }

  // set consumer
  cache->SetConsumer(pipeline->ConsumerFilter);
  expected.ConsumerDefined = true;
  status = cache->GetStatus();
  if ((status != expected) || status.enabled())
  {
    vtkLog(ERROR, "CacheInitialization: error with consumer setup.");
    return false;
  }

  // update cached mesh and mtimes.
  cache->UpdateCache(pipeline->GetFilterOutputData());
  expected.CacheDefined = true;
  expected.OriginalMeshUnmodified = true;
  expected.ConsumerUnmodified = true;
  status = cache->GetStatus();
  if ((status != expected) || !status.enabled())
  {
    cache->PrintSelf(std::cout, vtkIndent());
    vtkLog(ERROR, "CacheInitialization: error while caching data.");
    return false;
  }

  // set attribute ids
  // filter forward input cell data to point data.
  cache->AddOriginalIds(vtkDataObject::POINT, mockArraysName::pointIds);
  status = cache->GetStatus();
  if (status != expected)
  {
    vtkLog(ERROR, "CacheInitialization: error with ids setup.");
    return false;
  }

  if (!status.enabled())
  {
    vtkLog(ERROR, "CacheInitialization: unexpected unusable cache.");
    return false;
  }

  return true;
}

//------------------------------------------------------------------------------
/**
 * Check impact of consumer and mesh time on the cache status.
 */
bool TestModifiedTime(TestPipelineInterface* pipeline)
{
  vtkNew<vtkDataObjectMeshCache> cache;

  pipeline->InitializeCache(cache);

  vtkDataObjectMeshCache::Status expected;
  expected.OriginalDataDefined = true;
  expected.ConsumerDefined = true;
  expected.CacheDefined = true;
  expected.OriginalMeshUnmodified = true;
  expected.ConsumerUnmodified = true;
  expected.AttributesIdsExists = true;

  vtkDataObjectMeshCache::Status status = cache->GetStatus();
  vtkLogIf(ERROR, !status.enabled(), "ModifiedTime: expect usable cache.");

  pipeline->UpdateInputData(details::modifiedData[0]);
  status = cache->GetStatus();
  vtkLogIf(ERROR, status != expected, "ModifiedTime: error data mtime should not interfere.");
  vtkLogIf(ERROR, !status.enabled(), "ModifedTime: expect valid cache.");

  pipeline->MarkConsumerModified();
  status = cache->GetStatus();
  expected.ConsumerUnmodified = false;
  vtkLogIf(ERROR, status != expected, "ModifiedTime: error with consumer mtime.");
  vtkLogIf(ERROR, status.enabled(), "ModifedTime: expect invalid cache.");

  cache->UpdateCache(pipeline->GetFilterOutputData());
  status = cache->GetStatus();
  expected.ConsumerUnmodified = true;
  vtkLogIf(ERROR, status != expected, "ModifiedTime: error when resetting consumer mtime.");
  vtkLogIf(ERROR, !status.enabled(), "ModifedTime: expect usable cache.");

  pipeline->MarkInputMeshModified();
  status = cache->GetStatus();
  expected.OriginalMeshUnmodified = false;
  vtkLogIf(ERROR, status != expected, "ModifiedTime: error with input mtime.");
  vtkLogIf(ERROR, status.enabled(), "ModifiedTime: expect invalid cache.");

  cache->UpdateCache(pipeline->GetFilterOutputData());
  status = cache->GetStatus();
  expected.OriginalMeshUnmodified = true;
  vtkLogIf(ERROR, status != expected, "ModifiedTime: error when resetting consumer mtime.");
  vtkLogIf(ERROR, !status.enabled(), "ModifedTime: expect usable cache.");

  return true;
}

//------------------------------------------------------------------------------
/**
 * Try different configuration of original attribute ids.
 */
bool TestAttributesIds(TestPipelineInterface* pipeline)
{
  vtkNew<vtkDataObjectMeshCache> cache;
  pipeline->InitializeCache(cache);

  vtkDataObjectMeshCache::Status expected;
  expected.OriginalDataDefined = true;
  expected.ConsumerDefined = true;
  expected.CacheDefined = true;
  expected.OriginalMeshUnmodified = true;
  expected.ConsumerUnmodified = true;
  expected.AttributesIdsExists = true;

  cache->RemoveOriginalIds(vtkDataObject::POINT);
  vtkDataObjectMeshCache::Status status = cache->GetStatus();
  vtkLogIf(ERROR, status != expected, "AttributesIds: error when resetting attributes ids.");
  vtkLogIf(ERROR, !status.enabled(), "AttributesIds: without attributes cache should be usable.");

  cache->ClearOriginalIds();
  expected.AttributesIdsExists = false;
  cache->AddOriginalIds(vtkDataObject::CELL, mockArraysName::pointIds);
  status = cache->GetStatus();
  vtkLogIf(
    ERROR, status != expected, "AttributesIds: error when adding attribute without global ids.");
  vtkLogIf(ERROR, status.enabled(), "AttributesIds: inexisting global ids should disable cache.");

  cache->ClearOriginalIds();
  cache->AddOriginalIds(vtkDataObject::VERTEX, mockArraysName::pointIds);
  status = cache->GetStatus();
  vtkLogIf(ERROR, status != expected,
    "AttributesIds: error when setting ids on inexisting attribute type.");
  vtkLogIf(ERROR, status.enabled(), "AttributesIds: inexisting id array should disable cache.");

  return true;
}

//------------------------------------------------------------------------------
bool TestInvalidateCache(TestPipelineInterface* pipeline)
{
  vtkNew<vtkDataObjectMeshCache> cache;
  vtkNew<vtkPolyData> output;

  pipeline->InitializeCache(cache);

  vtkDataObjectMeshCache::Status status = cache->GetStatus();
  vtkLogIf(ERROR, !status.enabled(), "InvalidateCache: expect usable cache.");
  cache->InvalidateCache();
  status = cache->GetStatus();
  vtkLogIf(ERROR, status.enabled(), "InvalidateCache: cache should have been invalidated.");

  return true;
}

//------------------------------------------------------------------------------
bool TestUseCache(TestMeshPipeline* pipeline)
{
  vtkNew<vtkDataObjectMeshCache> cache;

  vtkNew<vtkPolyData> expectedOutput;
  expectedOutput->DeepCopy(pipeline->GetFilterOutputData());
  // we forward only point data
  expectedOutput->GetCellData()->Initialize();
  assert(expectedOutput->GetCellData()->GetGhostArray() == nullptr);

  pipeline->InitializeCache(cache);
  pipeline->UpdateInputData(details::modifiedData[0]);

  vtkDataObjectMeshCache::Status status = cache->GetStatus();
  vtkLogIf(ERROR, !status.enabled(), "UseCache: expect usable cache.");
  vtkNew<vtkPolyData> cacheOutput;
  cache->CopyCacheToDataObject(cacheOutput);
  status = cache->GetStatus();
  vtkLogIf(ERROR, !status.enabled(), "UseCache: using cache should not invalidate it.");

  // Cell data arrays are not forwarded.
  // This concerns also ghostcells array that is used in GetMeshMTime. So cache output
  // may appears as older than pipeline data.
  bool sameMeshTime = pipeline->GetOutputMeshMTime() >= cacheOutput->GetMeshMTime();
  vtkLogIf(ERROR, !sameMeshTime,
    "UseCache: cache should have same mesh mtime than previous output. Expected");
  vtkLogIf(ERROR, !sameMeshTime,
    "expected: " << pipeline->GetInputMeshMTime() << " but has: " << cacheOutput->GetMeshMTime());

  vtkNew<vtkIntArray> expectedArray;
  expectedArray->SetName(mockArraysName::pointData.c_str());
  expectedArray->SetArray(details::modifiedData.data(), 4, 1);
  expectedOutput->GetPointData()->AddArray(expectedArray);

  bool sameData = vtkTestUtilities::CompareDataObjects(cacheOutput, expectedOutput);
  vtkLogIf(ERROR, !sameData, "UseCache: wrong cache output.");

  return true;
}

//------------------------------------------------------------------------------
bool TestMeshOnly(TestMeshPipeline* pipeline)
{
  vtkNew<vtkDataObjectMeshCache> cache;

  vtkNew<vtkPolyData> expectedOutput;
  expectedOutput->DeepCopy(pipeline->GetFilterOutputData());
  expectedOutput->GetPointData()->Initialize();
  expectedOutput->GetCellData()->Initialize();

  pipeline->InitializeCache(cache);
  cache->ClearOriginalIds();

  vtkDataObjectMeshCache::Status status = cache->GetStatus();
  vtkLogIf(ERROR, !status.enabled(), "MeshOnly: expect usable cache.");
  vtkNew<vtkPolyData> cacheOutput;
  cache->CopyCacheToDataObject(cacheOutput);
  status = cache->GetStatus();
  vtkLogIf(ERROR, !status.enabled(), "MeshOnly: using cache should not invalidate it.");

  bool sameData = vtkTestUtilities::CompareDataObjects(cacheOutput, expectedOutput);
  vtkLogIf(ERROR, !sameData, "MeshOnly: wrong cache output.");

  return true;
}

//------------------------------------------------------------------------------
bool TestGhostCells(TestMeshPipeline* pipeline)
{
  vtkNew<vtkDataObjectMeshCache> cache;

  vtkNew<vtkPolyData> expectedOutput;
  expectedOutput->DeepCopy(pipeline->GetFilterOutputData());
  expectedOutput->GetPointData()->Initialize();

  pipeline->InitializeCache(cache);

  vtkDataObjectMeshCache::Status status = cache->GetStatus();
  vtkLogIf(ERROR, !status.enabled(), "GhostCells: expect usable cache.");

  pipeline->SetUseGhosts(false);
  status = cache->GetStatus();
  vtkLogIf(ERROR, status.enabled(), "GhostCells: removing ghosts should invalidate cache.");

  pipeline->SetUseGhosts(true);
  status = cache->GetStatus();
  vtkLogIf(ERROR, status.enabled(), "GhostCells: adding ghosts should invalidate cache.");

  pipeline->MarkGhostsModified();
  status = cache->GetStatus();
  vtkLogIf(ERROR, status.enabled(), "GhostCells: modified ghosts should invalidate cache.");

  vtkNew<vtkPolyData> cacheOutput;
  cache->CopyCacheToDataObject(cacheOutput);
  bool hasGhosts = cacheOutput->GetCellData()->GetGhostArray() != nullptr;
  vtkLogIf(ERROR, hasGhosts, "GhostCells: cache output should not have ghost array");

  return true;
}

//------------------------------------------------------------------------------
bool TestUseCompositeCache(TestCompositePipeline* pipeline)
{
  vtkNew<vtkDataObjectMeshCache> cache;

  vtkNew<vtkPartitionedDataSetCollection> expectedOutput;
  expectedOutput->DeepCopy(pipeline->GetFilterOutputData());

  pipeline->InitializeCache(cache);
  vtkDataObjectMeshCache::Status status = cache->GetStatus();
  vtkLogIf(ERROR, !status.enabled(), "UseCompositeCache: expect usable cache.");

  pipeline->UpdateInputData(compositeDetails::modifiedData[0]);
  status = cache->GetStatus();
  vtkLogIf(ERROR, !status.enabled(), "UseCompositeCache: expect usable cache.");

  vtkNew<vtkPartitionedDataSetCollection> cacheOutput;
  cache->CopyCacheToDataObject(cacheOutput);

  compositeDetails::setupExpectedArray(expectedOutput, 0, compositeDetails::modifiedData);
  compositeDetails::setupExpectedArray(expectedOutput, 1, compositeDetails::modifiedData2);

  bool sameData = vtkTestUtilities::CompareDataObjects(cacheOutput, expectedOutput);
  vtkLogIf(ERROR, !sameData, "UseCompositeCache: using cache has unexpected content.");

  return true;
}

//------------------------------------------------------------------------------
bool TestUnsupportedInputs()
{
  vtkNew<vtkDataObjectMeshCache> cache;
  vtkNew<vtkTest::ErrorObserver> observer;
  cache->AddObserver(vtkCommand::WarningEvent, observer);

  // image data is supported, but does not make use of the cache (for now)
  vtkNew<vtkImageData> image;
  cache->SetOriginalDataObject(image);

  vtkLogIf(ERROR, !cache->IsSupportedData(image), "ImageData is expected to be supported.");
  vtkLogIf(ERROR, observer->GetWarning() || observer->GetError(),
    "Using ImageData shouldn't raise errors or warnings.");

  // non dataset: vtkTable
  vtkNew<vtkTable> table;
  cache->SetOriginalDataObject(table);
  int nbOfFailures = observer->CheckWarningMessage("Unsupported input type: vtkTable");

  // wrong composite: htg
  vtkNew<vtkHyperTreeGrid> htg;
  cache->SetOriginalDataObject(htg);
  nbOfFailures += observer->CheckWarningMessage("Unsupported input type: vtkHyperTreeGrid");

  observer->Clear();
  // composite of wrong dataset: image data
  vtkNew<vtkPartitionedDataSetCollection> pdc;
  pdc->SetPartition(0, 0, image);
  cache->SetOriginalDataObject(pdc);
  vtkLogIf(ERROR, !cache->IsSupportedData(pdc),
    "Composite dataset with ImageData is expected to be supported.");
  vtkLogIf(ERROR, observer->GetWarning() || observer->GetError(),
    "Composite dataset with ImageData is expected to not raise errors or warnings.");

  // composite with a mix of unsupported/supported leaves
  vtkNew<vtkPolyData> polydata;
  pdc->SetPartition(1, 0, polydata);
  cache->SetOriginalDataObject(pdc);
  vtkLogIf(ERROR, !cache->IsSupportedData(pdc),
    "Composite dataset with ImageData and PolyData is expected to be supported.");
  vtkLogIf(ERROR, observer->GetWarning() || observer->GetError(),
    "Composite dataset with ImageData and PolyData is expected to not raise errors or warnings.");

  pdc->SetPartition(0, 0, table);
  pdc->SetPartition(1, 0, polydata);
  cache->SetOriginalDataObject(pdc);
  nbOfFailures += observer->CheckWarningMessage(
    "Composite vtkPartitionedDataSetCollection has unsupported block(s).");

  vtkNew<vtkConsumerDataFilter> consumer;
  cache->SetConsumer(consumer);
  vtkDataObjectMeshCache::Status status = cache->GetStatus();
  vtkLogIf(ERROR, status.enabled() || status.OriginalDataDefined,
    "Cache status OriginalDataDefined should be false without a correct OriginalDataObject.");

  return nbOfFailures == 0;
}

//------------------------------------------------------------------------------
bool TestUnsupportedCalls(TestPipelineInterface* pipeline)
{
  vtkNew<vtkTest::ErrorObserver> observer;
  vtkNew<vtkDataObjectMeshCache> cache;
  cache->AddObserver(vtkCommand::WarningEvent, observer);

  pipeline->InitializeCache(cache);

  cache->UpdateCache(nullptr);
  int nbOfFailures = observer->CheckWarningMessage("Cannot update from nullptr");
  cache->GetStatus();

  cache->CopyCacheToDataObject(nullptr);
  nbOfFailures = observer->CheckWarningMessage("Cannot copy to nullptr");
  cache->GetStatus();

  cache->SetOriginalDataObject(nullptr);
  nbOfFailures += observer->CheckWarningMessage("Invalid original dataobject: nullptr");
  cache->GetStatus();

  pipeline->InitializeCache(cache);
  vtkNew<vtkTable> table;
  cache->UpdateCache(table);
  nbOfFailures +=
    observer->CheckWarningMessage("Cannot update from unsupported data type: vtkTable");
  cache->GetStatus();

  cache->CopyCacheToDataObject(table);
  cache->GetStatus();

  cache->AddOriginalIds(-1, "ids");
  nbOfFailures += observer->CheckWarningMessage("Invalid attribute type: -1");
  cache->GetStatus();

  cache->CopyCacheToDataObject(table);
  nbOfFailures += observer->CheckWarningMessage("Cannot copy to unsupported data type: vtkTable");

  cache->InvalidateCache();
  cache->CopyCacheToDataObject(table);
  nbOfFailures += observer->CheckWarningMessage("Cannot copy from nullptr");

  return nbOfFailures == 0;
}

//------------------------------------------------------------------------------
int TestDataObjectMeshCache(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkLog(INFO, "Test default cache");
  bool success = TestDefault();

  // simple dataset
  if (success)
  {
    vtkLogScopeF(INFO, "Test polydata");
    std::unique_ptr<TestMeshPipeline> initPipeline(new TestMeshPipeline());
    success = success && TestCacheInitialization(initPipeline.get());
    std::unique_ptr<TestMeshPipeline> timePipeline(new TestMeshPipeline());
    success = success && TestModifiedTime(timePipeline.get());
    std::unique_ptr<TestMeshPipeline> idsPipeline(new TestMeshPipeline());
    success = success && TestAttributesIds(idsPipeline.get());
    std::unique_ptr<TestMeshPipeline> invalidatePipeline(new TestMeshPipeline());
    success = success && TestInvalidateCache(invalidatePipeline.get());
    std::unique_ptr<TestMeshPipeline> usePipeline(new TestMeshPipeline());
    success = success && TestUseCache(usePipeline.get());
    std::unique_ptr<TestMeshPipeline> meshOnlyPipeline(new TestMeshPipeline());
    success = success && TestMeshOnly(meshOnlyPipeline.get());
  }

  // simple dataset with ghosts
  if (success)
  {
    vtkLogScopeF(INFO, "Test ghost cells");
    std::unique_ptr<TestMeshPipeline> initPipeline(new TestMeshPipeline(true));
    success = success && TestCacheInitialization(initPipeline.get());
    std::unique_ptr<TestMeshPipeline> timePipeline(new TestMeshPipeline(true));
    success = success && TestModifiedTime(timePipeline.get());
    std::unique_ptr<TestMeshPipeline> idsPipeline(new TestMeshPipeline(true));
    success = success && TestAttributesIds(idsPipeline.get());
    std::unique_ptr<TestMeshPipeline> invalidatePipeline(new TestMeshPipeline(true));
    success = success && TestInvalidateCache(invalidatePipeline.get());
    std::unique_ptr<TestMeshPipeline> usePipeline(new TestMeshPipeline(true));
    success = success && TestUseCache(usePipeline.get());
    std::unique_ptr<TestMeshPipeline> meshOnlyPipeline(new TestMeshPipeline(true));
    success = success && TestMeshOnly(meshOnlyPipeline.get());
    std::unique_ptr<TestMeshPipeline> ghostCellsPipeline(new TestMeshPipeline(true));
    success = success && TestGhostCells(ghostCellsPipeline.get());
  }

  // composite
  if (success)
  {
    vtkLogScopeF(INFO, "Test composite");
    std::unique_ptr<TestCompositePipeline> initPipeline(new TestCompositePipeline());
    success = success && TestCacheInitialization(initPipeline.get());
    std::unique_ptr<TestCompositePipeline> timePipeline(new TestCompositePipeline());
    success = success && TestModifiedTime(timePipeline.get());
    std::unique_ptr<TestCompositePipeline> idsPipeline(new TestCompositePipeline());
    success = success && TestAttributesIds(idsPipeline.get());
    std::unique_ptr<TestCompositePipeline> invalidatePipeline(new TestCompositePipeline());
    success = success && TestInvalidateCache(invalidatePipeline.get());
    std::unique_ptr<TestCompositePipeline> usePipeline(new TestCompositePipeline());
    success = success && TestUseCompositeCache(usePipeline.get());
  }

  // unsupported cases
  if (success)
  {
    vtkLogScopeF(INFO, "Test unsupported cases");
    success = success && TestUnsupportedInputs();
    std::unique_ptr<TestMeshPipeline> simplePipeline(new TestMeshPipeline());
    success = success && TestUnsupportedCalls(simplePipeline.get());
    std::unique_ptr<TestCompositePipeline> compositePipeline(new TestCompositePipeline());
    success = success && TestUnsupportedCalls(compositePipeline.get());
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
