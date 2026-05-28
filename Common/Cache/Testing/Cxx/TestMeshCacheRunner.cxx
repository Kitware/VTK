// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAlgorithm.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataObjectMeshCache.h"
#include "vtkDoubleArray.h"
#include "vtkLogger.h"
#include "vtkMeshCacheRunner.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTestUtilities.h"

#include <cstdlib>

namespace
{
/**
 * Without the automatic update, leaving scope lets the cache empty.
 */
bool FirstStepNoUpdate(vtkDataObjectMeshCache* cache, vtkPolyData* input, vtkPolyData* output)
{
  vtkLogScopeFunction(INFO);
  {
    vtkMeshCacheRunner runner{ cache, input, output, false };
    auto status = cache->GetStatus();
    if (status.enabled())
    {
      vtkLog(ERROR, "ERROR: cache should not have been initialized.");
      return false;
    }
    if (output->GetNumberOfPoints() != 0)
    {
      vtkLog(ERROR, "ERROR: cache is empty and output should be empty too.");
      return false;
    }
  }
  auto status = cache->GetStatus();
  if (status.enabled() || status.CacheDefined)
  {
    vtkLog(ERROR, "ERROR: cache should not have been init.");
    return false;
  }

  return true;
}

/**
 * The cache is initialized from the runner destructor, when is leaving its scope.
 */
bool SecondStepAutomaticUpdate(
  vtkDataObjectMeshCache* cache, vtkPolyData* input, vtkPolyData* output)
{
  vtkLogScopeFunction(INFO);
  {
    vtkMeshCacheRunner runner{ cache, input, output, true };
    if (output->GetNumberOfPoints() != 0)
    {
      vtkLog(ERROR, "Output should still be empty");
      return false;
    }
    // cache will be init from output: fill it like a passthrough filter
    output->ShallowCopy(input);
  }
  auto status = cache->GetStatus();
  if (!status.enabled())
  {
    vtkLog(ERROR, "runner with auto update should have initialized cache.");
    return false;
  }
  return true;
}

/**
 * Output is finally updated using cache.
 */
bool ThirdStepInitOutput(vtkDataObjectMeshCache* cache, vtkPolyData* input, vtkPolyData* output)
{
  vtkLogScopeFunction(INFO);
  // cleanup to be able to check the content after cache usage.
  output->Initialize();
  {
    vtkMeshCacheRunner runner{ cache, input, output, true };
    if (output->GetNumberOfPoints() == 0)
    {
      vtkLog(ERROR, "Output should not be empty");
      return false;
    }
  }
  if (!vtkTestUtilities::CompareDataObjects(input, output))
  {
    vtkLog(ERROR, "Output should have been initialized from input");
    return false;
  }

  return true;
}

vtkSmartPointer<vtkPolyData> CreateData()
{
  vtkNew<vtkPolyData> mesh;
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> lines;
  mesh->SetPoints(points);
  mesh->SetLines(lines);
  vtkNew<vtkDoubleArray> pointArray;
  pointArray->SetName("point_array");
  vtkNew<vtkDoubleArray> cellArray;
  cellArray->SetName("cell_array");
  mesh->GetPointData()->AddArray(pointArray);
  mesh->GetCellData()->AddArray(cellArray);
  const int nbOfElements = 5;
  for (int i = 0; i < nbOfElements; i++)
  {
    points->InsertNextPoint(i, 0, 0);
    // line with next point. Last cell goes from last point to first.
    lines->InsertNextCell({ i, (i + 1) % nbOfElements });
    pointArray->InsertNextValue(i);
    cellArray->InsertNextValue(i);
  }

  return mesh;
}
}

int TestMeshCacheRunner(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  vtkNew<vtkDataObjectMeshCache> cache;
  vtkNew<vtkAlgorithm> consumer;
  cache->SetConsumer(consumer);

  auto mesh = ::CreateData();
  cache->SetOriginalDataObject(mesh);
  cache->PreservedInputAllAttributes();
  cache->ForwardAttribute(vtkDataObject::POINT);
  cache->ForwardAttribute(vtkDataObject::CELL);

  vtkNew<vtkPolyData> output;
  if (!::FirstStepNoUpdate(cache, mesh, output))
  {
    return EXIT_FAILURE;
  }

  output->Initialize();
  if (!::SecondStepAutomaticUpdate(cache, mesh, output))
  {
    return EXIT_FAILURE;
  }

  output->Initialize();
  if (!::ThirdStepInitOutput(cache, mesh, output))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
