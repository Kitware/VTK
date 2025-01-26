// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef MeshCacheMockAlgorithms_h
#define MeshCacheMockAlgorithms_h

#include "vtkNew.h"
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkPassInputTypeAlgorithm.h"
#include "vtkPolyDataAlgorithm.h"
#include "vtkSetGet.h"

#include <array>

VTK_ABI_NAMESPACE_BEGIN

class vtkCompositeDataSet;
class vtkDataObjectMeshCache;
class vtkPartitionedDataSetCollection;

//------------------------------------------------------------------------------
namespace mockArraysName
{
const std::string pointIds = "pointIds";
const std::string pointData = "pointData";
};

//------------------------------------------------------------------------------
/**
 * Simple source that create a polydata (triangle)
 * with point data arrays:
 *  * pointIds: the ids
 *  * pointData: incremental array starting at StartData (default 0)
 */
class vtkStaticDataSource : public vtkPolyDataAlgorithm
{
  vtkNew<vtkPolyData> SourceOutput;
  int StartData = 0;
  bool GenerateGhosts = false;

public:
  vtkTypeMacro(vtkStaticDataSource, vtkPolyDataAlgorithm);
  static vtkStaticDataSource* New();

  vtkStaticDataSource();
  ~vtkStaticDataSource() override = default;

  vtkSetMacro(StartData, int);

  vtkSetMacro(GenerateGhosts, bool);

  void MarkGhostsModified();

  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override;

  // mark polydata points as modified
  void MarkMeshModified();
};

//------------------------------------------------------------------------------
/**
 * Simple source that create a composite (PartitionedDataSetCollection) of 2 polydata.
 * internally use vtkStaticDataSource to generate polydata with data.
 */
class vtkStaticCompositeSource : public vtkPartitionedDataSetCollectionAlgorithm
{
  vtkNew<vtkPartitionedDataSetCollection> SourceOutput;
  vtkNew<vtkStaticDataSource> FirstData;
  vtkNew<vtkStaticDataSource> SecondData;

public:
  vtkTypeMacro(vtkStaticCompositeSource, vtkPartitionedDataSetCollectionAlgorithm);
  static vtkStaticCompositeSource* New();

  vtkStaticCompositeSource();
  ~vtkStaticCompositeSource() override = default;

  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override;

  /**
   * Forward to internal vtkStaticDataSource.
   * Second source has an offset to avoid values overlap.
   */
  void SetStartData(int start);

  /**
   * Forward to internal vtkStaticDataSource to mark points as modified.
   */
  void MarkMeshModified();
};

//------------------------------------------------------------------------------
/**
 * Simple filter that will be our cache consumer.
 * Only ShallowCopy input to output.
 */
class vtkConsumerDataFilter : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkConsumerDataFilter, vtkPassInputTypeAlgorithm);
  static vtkConsumerDataFilter* New();

  int RequestData(vtkInformation*, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Get output data as composite dataset.
   */
  vtkCompositeDataSet* GetCompositeOutput();
};

VTK_ABI_NAMESPACE_END

#endif
