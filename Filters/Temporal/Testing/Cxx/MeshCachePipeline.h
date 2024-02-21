// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#ifndef MeshCachePipeline_h
#define MeshCachePipeline_h

#include "vtkNew.h"
#include "vtkType.h"

#include <array>

VTK_ABI_NAMESPACE_BEGIN

class vtkConsumerDataFilter;
class vtkDataObject;
class vtkDataObjectMeshCache;
class vtkStaticCompositeSource;
class vtkStaticDataSource;

//------------------------------------------------------------------------------
/**
 * Interface to provide a pipeline utility:
 *  - define accessor to different elements of this pipeline.
 *  - initialize a vtkDataObjectMeshCache object
 */
class TestPipelineInterface
{
public:
  void InitializeCache(vtkDataObjectMeshCache* cache);
  vtkDataObject* GetFilterInputData();
  vtkDataObject* GetFilterOutputData();

  virtual ~TestPipelineInterface() = default;

  /**
   * Mark filter as modified.
   */
  void MarkConsumerModified();

  virtual vtkMTimeType GetOutputMeshMTime() = 0;
  virtual vtkMTimeType GetInputMeshMTime() = 0;

  /**
   * Change data in input data array. Mesh stay unmodified.
   */
  virtual void UpdateInputData(int start) = 0;

  /**
   * Mark input mesh as modified.
   */
  virtual void MarkInputMeshModified() = 0;

  vtkNew<vtkConsumerDataFilter> ConsumerFilter;
};

/**
 * Implement TestPipelineInterface.
 * Construct a pipeline with the static mesh source and the consumer filter.
 */
class TestMeshPipeline : public TestPipelineInterface
{
  vtkNew<vtkStaticDataSource> StaticMeshSource;

public:
  TestMeshPipeline(bool useGhost = false);
  ~TestMeshPipeline() override = default;

  void UpdateInputData(int start) override;
  void MarkInputMeshModified() override;
  vtkMTimeType GetOutputMeshMTime() override;
  vtkMTimeType GetInputMeshMTime() override;
  void SetUseGhosts(bool useghost);
  void MarkGhostsModified();
};

/**
 * Implement TestPipelineInterface.
 * Construct a pipeline with the static composite source and the consumer filter.
 */
class TestCompositePipeline : public TestPipelineInterface
{
  vtkNew<vtkStaticCompositeSource> StaticCompositeSource;

public:
  TestCompositePipeline();
  ~TestCompositePipeline() override = default;

  void UpdateInputData(int start) override;
  void MarkInputMeshModified() override;
  vtkMTimeType GetOutputMeshMTime() override;
  vtkMTimeType GetInputMeshMTime() override;
};
VTK_ABI_NAMESPACE_END

#endif
