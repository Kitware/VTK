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
 * In VTK 9.1 and earlier, subclasses overrode `RequestData` instead of
 * `Execute`. RequestData was passed a `vtkMultiBlockDataSet` with blocks corresponding
 * to the input timesteps. However, with addition of vtkPartitionedDataSet and
 * vtkPartitionedDataSetCollection in VTK 9.2, it is not possible to package all
 * input data types into a multiblock dataset. Hence, the method is deprecated
 * and only used when `Execute` is not overridden.
 */

#ifndef vtkMultiTimeStepAlgorithm_h
#define vtkMultiTimeStepAlgorithm_h

#include "vtkAlgorithm.h"
#include "vtkCommonExecutionModelModule.h" // For export macro
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
