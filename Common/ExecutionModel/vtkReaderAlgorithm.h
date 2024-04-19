// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkReaderAlgorithm
 * @brief   Superclass for readers that implement a simplified API.
 *
 * This class and associated subclasses were created to make it easier to
 * develop readers. When directly subclassing from other algorithm classes
 * one has to learn a general purpose API that somewhat obfuscates pipeline
 * functionality behind information keys. One has to know how to find
 * time and pieces requests using keys for example. Furthermore, these
 * classes together with specialized executives can implement common
 * reader functionality for things such as file series (for time and/or
 * partitions), caching, mapping time requests to indices etc.
 * This class implements the most basic API which is specialized as
 * needed by subclasses (for file series for example).
 */

#ifndef vtkReaderAlgorithm_h
#define vtkReaderAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkReaderAlgorithm : public vtkAlgorithm
{
public:
  vtkTypeMacro(vtkReaderAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Overridden to call appropriate handle pipeline request from executive.
   */
  vtkTypeBool ProcessRequest(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* outInfo) override;

  /**
   * This can be overridden by a subclass to create an output that
   * is determined by the file being read. If the output is known at
   * compile time, it is easier to override FillOutputPortInformation()
   * to set vtkDataObject::DATA_TYPE_NAME(). The subclass should compare
   * the new output type with the type of the currentOutput argument and
   * return currentOutput without changing its reference count if the
   * types are same.
   */
  virtual vtkDataObject* CreateOutput(vtkDataObject* currentOutput) { return currentOutput; }

  /**
   * Provide meta-data for the pipeline. This meta-data cannot vary over
   * time as this method will not be called when only a request is changed.
   * These include things like time steps. Subclasses may have specialized
   * interfaces making this simpler.
   */
  virtual int ReadMetaData(vtkInformation* metadata) = 0;

  /**
   * Provide meta-data for the pipeline. This meta-data can vary over time
   * as this method will be called after a request is changed (such as time)
   * These include things like whole extent. Subclasses may have specialized
   * interfaces making this simpler.
   */
  virtual int ReadTimeDependentMetaData(int /*timestep*/, vtkInformation* /*metadata*/)
  {
    return 1;
  }

  /**
   * Read the mesh (connectivity) for a given set of data partitioning,
   * number of ghost levels and time step (index). The reader populates
   * the data object passed in as the last argument. It is OK to read
   * more than the mesh (points, arrays etc.). However, this may interfere
   * with any caching implemented by the executive (i.e. cause more reads).
   */
  virtual int ReadMesh(
    int piece, int npieces, int nghosts, int timestep, vtkDataObject* output) = 0;

  /**
   * Read the points. The reader populates the input data object. This is
   * called after ReadMesh() so the data object should already contain the
   * mesh.
   */
  virtual int ReadPoints(
    int piece, int npieces, int nghosts, int timestep, vtkDataObject* output) = 0;

  /**
   * Read all the arrays (point, cell, field etc.). This is called after
   * ReadPoints() so the data object should already contain the mesh and
   * points.
   */
  virtual int ReadArrays(
    int piece, int npieces, int nghosts, int timestep, vtkDataObject* output) = 0;

protected:
  vtkReaderAlgorithm();
  ~vtkReaderAlgorithm() override;

private:
  vtkReaderAlgorithm(const vtkReaderAlgorithm&) = delete;
  void operator=(const vtkReaderAlgorithm&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
