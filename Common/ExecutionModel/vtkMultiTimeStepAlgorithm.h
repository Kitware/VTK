// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkMultiTimeStepAlgorithm
 * @brief Superclass for algorithms that would like to make multiple time requests
 *
 * This class can be inherited by any algorithm that wishes to make multiple
 * time requests upstream.
 *
 * A subclass should override `RequestUpdateExtent` and use
 * `vtkMultiTimeStepAlgorithm::UPDATE_TIME_STEPS` key to indicate which
 * timesteps are to be requested. This class will then take care of executing
 * the upstream pipeline to obtain the requested timesteps.
 *
 * Subclasses can then override `Execute` which is provided a vector of input
 * data objects corresponding to the requested timesteps.
 *
 * The REQUEST_DATA request is handled to aggregate the input data for each
 * timestep. On the last call, it forwards them to the `Execute` method.
 * Thus the usual RequestData is not defined here.
 */

#ifndef vtkMultiTimeStepAlgorithm_h
#define vtkMultiTimeStepAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro
#include "vtkDeprecation.h"                // For Deprecation macro
#include "vtkSmartPointer.h"               //needed for a private variable

#include "vtkDataObject.h" // needed for the smart pointer
#include <vector>          //needed for a private variable

VTK_ABI_NAMESPACE_BEGIN
class vtkInformationDoubleVectorKey;
class vtkMultiBlockDataSet;
class VTKCOMMONEXECUTIONMODEL_EXPORT vtkMultiTimeStepAlgorithm : public vtkAlgorithm
{
public:
  static vtkMultiTimeStepAlgorithm* New();
  vtkTypeMacro(vtkMultiTimeStepAlgorithm, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkMultiTimeStepAlgorithm();

  ~vtkMultiTimeStepAlgorithm() override = default;

  /**
   * This is filled by the child class to request multiple time steps
   */
  VTK_DEPRECATED_IN_9_6_0("Please use SetTimeSteps directly instead.")
  static vtkInformationDoubleVectorKey* UPDATE_TIME_STEPS();

  ///@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  ///@}

  ///@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }
  ///@}

  /**
   * Subclasses should override this method to do the actual execution.
   * For backwards compatibility, the default implementation returns -1. If -1
   * is returned, its assumed that this method is not overridden and the
   * `RequestData` must be called, if possible. However, `RequestData` is only
   * supported if input type is not vtkPartitionedDataSetCollection or
   * vtkPartitionedDataSet.
   */
  virtual int Execute(vtkInformation* vtkNotUsed(request),
    const std::vector<vtkSmartPointer<vtkDataObject>>& vtkNotUsed(inputs),
    vtkInformationVector* vtkNotUsed(outputVector))
  {
    return -1;
  }

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*)
  {
    return 1;
  }

  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Set the list of time steps values.
   */
  void SetTimeSteps(const std::vector<double>& values);

  bool CacheData;
  unsigned int NumberOfCacheEntries;

private:
  vtkMultiTimeStepAlgorithm(const vtkMultiTimeStepAlgorithm&) = delete;
  void operator=(const vtkMultiTimeStepAlgorithm&) = delete;
  int RequestUpdateIndex;              // keep track of the time looping index
  std::vector<double> UpdateTimeSteps; // store the requested time steps
  bool IsInCache(double time, size_t& idx);
  struct TimeCache
  {
    TimeCache(double time, vtkDataObject* data)
      : TimeValue(time)
      , Data(data)
    {
    }
    double TimeValue;
    vtkSmartPointer<vtkDataObject> Data;
  };
  std::vector<TimeCache> Cache;
};

VTK_ABI_NAMESPACE_END
#endif
